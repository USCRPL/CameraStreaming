/*
 * h264_encoder.c - ESP32-P4 HW H.264 via the esp_video V4L2 M2M device.
 *
 * Setup: open /dev/video11 -> S_FMT OUTPUT(YUV420) + CAPTURE(H264) ->
 *        S_EXT_CTRLS(bitrate,GOP,QP) -> REQBUFS/QUERYBUF/mmap on both queues
 *        -> queue all CAPTURE buffers -> STREAMON both.
 * Per frame: copy YUV into an OUTPUT buffer, QBUF it, DQBUF it back;
 *            DQBUF a CAPTURE buffer (the coded AU) -> callback -> QBUF it back.
 */
#include "h264_encoder.h"

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/select.h>

#include "linux/videodev2.h"
#include "esp_video_device.h"   /* ESP_VIDEO_H264_DEVICE_NAME ("/dev/video11") */
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_check.h"

#define TAG "h264_enc"
#ifndef ESP_VIDEO_H264_DEVICE_NAME
#define ESP_VIDEO_H264_DEVICE_NAME "/dev/video11"
#endif
#define H264_MAX_BUFS 4

/* Poll the encoder in short slices rather than one long wait: esp_video's
 * select() returns at the timeout boundary, so a single 100 ms timeout cost
 * 100 ms on every frame regardless of how fast the hardware finished. */
#define ENC_POLL_SLICE_MS  15
#define ENC_POLL_SLICES     8
#define H264_BR_MIN   25000u
#define H264_BR_MAX   2500000u

struct h264_encoder_s {
    int      fd;
    uint32_t width, height, yuv_bytes;
    uint32_t n_out, n_cap;
    void    *out_buf[H264_MAX_BUFS]; size_t out_len[H264_MAX_BUFS];
    void    *cap_buf[H264_MAX_BUFS]; size_t cap_len[H264_MAX_BUFS];
    uint32_t out_rr;   /* round-robin index into OUTPUT buffers */
};

static esp_err_t set_ctrl_named(int fd, uint32_t id, int32_t val, const char *name)
{
    struct v4l2_ext_control c = { .id = id, .value = val };
    struct v4l2_ext_controls cs = {
        .ctrl_class = V4L2_CTRL_CLASS_MPEG, .count = 1, .controls = &c,
    };
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &cs) == 0) {
        ESP_LOGI(TAG, "  ctrl %-12s = %-8d OK", name, (int)val);
        return ESP_OK;
    }
    /* Worth knowing: a rejected bitrate/QP silently leaves the encoder running
     * at its own defaults, which is how frames ended up many times over budget. */
    ESP_LOGW(TAG, "  ctrl %-12s = %-8d REJECTED", name, (int)val);
    return ESP_FAIL;
}

static esp_err_t set_ctrl(int fd, uint32_t id, int32_t val)
{
    struct v4l2_ext_control c = { .id = id, .value = val };
    struct v4l2_ext_controls cs = {
        .ctrl_class = V4L2_CTRL_CLASS_MPEG, .count = 1, .controls = &c,
    };
    return ioctl(fd, VIDIOC_S_EXT_CTRLS, &cs) == 0 ? ESP_OK : ESP_FAIL;
}

static esp_err_t map_queue(struct h264_encoder_s *e, int type, uint32_t count,
                           void **bufs, size_t *lens, uint32_t *n_out, bool queue_now)
{
    struct v4l2_requestbuffers req = { .count = count, .type = type,
                                       .memory = V4L2_MEMORY_MMAP };
    if (ioctl(e->fd, VIDIOC_REQBUFS, &req) != 0 || req.count < 1) return ESP_FAIL;
    *n_out = req.count;
    for (uint32_t i = 0; i < req.count; i++) {
        struct v4l2_buffer b = { .type = type, .memory = V4L2_MEMORY_MMAP, .index = i };
        if (ioctl(e->fd, VIDIOC_QUERYBUF, &b) != 0) return ESP_FAIL;
        lens[i] = b.length;
        bufs[i] = mmap(NULL, b.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                       e->fd, b.m.offset);
        if (bufs[i] == MAP_FAILED) return ESP_FAIL;
        if (queue_now && ioctl(e->fd, VIDIOC_QBUF, &b) != 0) return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t h264_encoder_start(const h264_encoder_cfg_t *cfg, h264_encoder_handle_t *out)
{
    ESP_RETURN_ON_FALSE(cfg && out, ESP_ERR_INVALID_ARG, TAG, "args");
    ESP_RETURN_ON_FALSE(cfg->out_buf_count <= H264_MAX_BUFS &&
                        cfg->cap_buf_count <= H264_MAX_BUFS,
                        ESP_ERR_INVALID_ARG, TAG, "too many buffers");

    struct h264_encoder_s *e = calloc(1, sizeof(*e));
    ESP_RETURN_ON_FALSE(e, ESP_ERR_NO_MEM, TAG, "oom");
    e->fd = -1;
    e->width = cfg->width; e->height = cfg->height;
    e->yuv_bytes = cfg->width * cfg->height * 3 / 2;

    esp_err_t ret = ESP_OK;
    e->fd = open(ESP_VIDEO_H264_DEVICE_NAME, O_RDWR);
    if (e->fd < 0) {
        ESP_LOGE(TAG, "open %s failed: errno %d (%s)",
                 ESP_VIDEO_H264_DEVICE_NAME, errno, strerror(errno));
        if (errno == ENOENT) {
            ESP_LOGE(TAG, "  -> the H.264 M2M device was not created. Enable it in");
            ESP_LOGE(TAG, "     menuconfig: Component config > Espressif Video >");
            ESP_LOGE(TAG, "     H.264 encoder video device (CONFIG_ESP_VIDEO_ENABLE_HW_H264_VIDEO_DEVICE=y)");
        }
        ret = ESP_FAIL; goto fail;
    }

    /* OUTPUT plane = raw YUV420 input to the encoder. */
    struct v4l2_format ofmt = { 0 };
    ofmt.type                = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ofmt.fmt.pix.width       = cfg->width;
    ofmt.fmt.pix.height      = cfg->height;
    ofmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    ofmt.fmt.pix.field       = V4L2_FIELD_NONE;
    ofmt.fmt.pix.bytesperline = cfg->width;   /* Y-plane stride. Driver reported 0;
                                   * with no stride the encoder reads our rows at the
                                   * wrong pitch, which repeats the image horizontally
                                   * even though the input buffer is correct I420. */
    ofmt.fmt.pix.sizeimage   = cfg->width * cfg->height * 3 / 2;  /* full I420; driver
                                   * reported 0 otherwise -> risk of a Y-only buffer
                                   * that drops U/V (green output). */
    /* Ground truth: ask the driver which INPUT (OUTPUT-queue) formats it actually
     * accepts. The ESP32-P4 hardware encoder does not necessarily consume planar
     * I420 -- if it wants a packed/interleaved layout, feeding I420 makes it walk
     * our rows at the wrong pitch, which repeats the image horizontally and
     * scrambles chroma exactly as observed. Whatever this prints is what we must
     * feed. */
    for (int i = 0; i < 8; i++) {
        struct v4l2_fmtdesc fd = { .index = (uint32_t)i,
                                   .type = V4L2_BUF_TYPE_VIDEO_OUTPUT };
        if (ioctl(e->fd, VIDIOC_ENUM_FMT, &fd) != 0) break;
        uint32_t pf = fd.pixelformat;
        ESP_LOGW(TAG, "encoder accepts INPUT fmt[%d]: %c%c%c%c  (%s)", i,
                 (char)(pf & 0xff), (char)((pf >> 8) & 0xff),
                 (char)((pf >> 16) & 0xff), (char)((pf >> 24) & 0xff),
                 fd.description);
    }

    ESP_GOTO_ON_FALSE(ioctl(e->fd, VIDIOC_S_FMT, &ofmt) == 0, ESP_FAIL, fail,
                      TAG, "S_FMT OUTPUT YUV420");

    /* Read back what the driver ACTUALLY negotiated. If fourcc isn't YU12 (planar
     * I420) -- e.g. NV12 (interleaved chroma) -- or sizeimage/bytesperline imply a
     * padded stride, then feeding tightly-packed I420 puts the chroma where the
     * encoder doesn't read it -> zero chroma -> solid green output. This log tells
     * us which. */
    {
        struct v4l2_format g = { .type = V4L2_BUF_TYPE_VIDEO_OUTPUT };
        if (ioctl(e->fd, VIDIOC_G_FMT, &g) == 0) {
            uint32_t pf = g.fmt.pix.pixelformat;
            ESP_LOGW(TAG, "OUTPUT G_FMT: %ux%u fourcc=%c%c%c%c bpl=%u sizeimage=%u (I420 wants %u)",
                     g.fmt.pix.width, g.fmt.pix.height,
                     (char)(pf & 0xff), (char)((pf >> 8) & 0xff),
                     (char)((pf >> 16) & 0xff), (char)((pf >> 24) & 0xff),
                     g.fmt.pix.bytesperline, g.fmt.pix.sizeimage,
                     (unsigned)(cfg->width * cfg->height * 3 / 2));
        }
    }

    /* CAPTURE plane = coded H.264 output. sizeimage bounds the per-frame
     * coded buffer the driver allocates; w*h is comfortably above a sub-2Mbit
     * access unit. */
    struct v4l2_format cfmt = { 0 };
    cfmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cfmt.fmt.pix.width       = cfg->width;
    cfmt.fmt.pix.height      = cfg->height;
    cfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
    cfmt.fmt.pix.sizeimage   = cfg->width * cfg->height;
    ESP_GOTO_ON_FALSE(ioctl(e->fd, VIDIOC_S_FMT, &cfmt) == 0, ESP_FAIL, fail,
                      TAG, "S_FMT CAPTURE H264");

    /* Rate control. Clamp bitrate to the encoder's supported window. */
    uint32_t br = cfg->bitrate_bps;
    if (br < H264_BR_MIN) br = H264_BR_MIN;
    if (br > H264_BR_MAX) br = H264_BR_MAX;
    ESP_LOGI(TAG, "applying rate control:");
    set_ctrl_named(e->fd, V4L2_CID_MPEG_VIDEO_BITRATE,        (int32_t)br,          "BITRATE");
    set_ctrl_named(e->fd, V4L2_CID_MPEG_VIDEO_H264_I_PERIOD,  (int32_t)cfg->gop,    "I_PERIOD");
    set_ctrl_named(e->fd, V4L2_CID_MPEG_VIDEO_H264_MIN_QP,    cfg->qp_min,          "MIN_QP");
    set_ctrl_named(e->fd, V4L2_CID_MPEG_VIDEO_H264_MAX_QP,    cfg->qp_max,          "MAX_QP");

    /* Buffers: OUTPUT held by us until we feed a frame; CAPTURE pre-queued. */
    ESP_GOTO_ON_FALSE(map_queue(e, V4L2_BUF_TYPE_VIDEO_OUTPUT, cfg->out_buf_count,
                                e->out_buf, e->out_len, &e->n_out, false) == ESP_OK,
                      ESP_FAIL, fail, TAG, "map OUTPUT");
    ESP_LOGW(TAG, "OUTPUT buffer length = %u (need %u for full I420)",
             (unsigned)e->out_len[0], (unsigned)(cfg->width * cfg->height * 3 / 2));
    ESP_GOTO_ON_FALSE(map_queue(e, V4L2_BUF_TYPE_VIDEO_CAPTURE, cfg->cap_buf_count,
                                e->cap_buf, e->cap_len, &e->n_cap, true) == ESP_OK,
                      ESP_FAIL, fail, TAG, "map CAPTURE");

    int t1 = V4L2_BUF_TYPE_VIDEO_OUTPUT, t2 = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_GOTO_ON_FALSE(ioctl(e->fd, VIDIOC_STREAMON, &t1) == 0, ESP_FAIL, fail,
                      TAG, "STREAMON OUTPUT");
    ESP_GOTO_ON_FALSE(ioctl(e->fd, VIDIOC_STREAMON, &t2) == 0, ESP_FAIL, fail,
                      TAG, "STREAMON CAPTURE");

    /* esp_video's VIDIOC_DQBUF blocks forever by default; bound it so a stalled
     * encode surfaces as a timeout/errno instead of hanging the pipeline. */
#ifdef VIDIOC_S_DQBUF_TIMEOUT
    {
        struct timeval to = { .tv_sec = 2, .tv_usec = 0 };
        if (ioctl(e->fd, VIDIOC_S_DQBUF_TIMEOUT, &to) != 0)
            ESP_LOGW(TAG, "could not set DQBUF timeout (errno %d) - DQBUF will block", errno);
    }
#else
    ESP_LOGW(TAG, "VIDIOC_S_DQBUF_TIMEOUT not defined in this esp_video version; "
                  "DQBUF will block indefinitely on a stall");
#endif

    ESP_LOGI(TAG, "encoder up %ux%u @%ubps gop %u (%u/%u buffers)",
             e->width, e->height, br, cfg->gop, e->n_out, e->n_cap);
    *out = e;
    return ESP_OK;

fail:
    h264_encoder_stop(e);
    return ret;
}

esp_err_t h264_encoder_encode(h264_encoder_handle_t e, const uint8_t *yuv, size_t len,
                              int64_t pts_90k, h264_au_cb cb, void *ctx)
{
    ESP_RETURN_ON_FALSE(e && yuv && cb, ESP_ERR_INVALID_ARG, TAG, "args");
    ESP_RETURN_ON_FALSE(len <= e->yuv_bytes, ESP_ERR_INVALID_SIZE, TAG, "frame too big");
    return h264_encoder_encode_filled(e, NULL, yuv, len, pts_90k, cb, ctx);
}

/* Hand back the next OUTPUT buffer so the caller can build the frame straight
 * into it -- avoids a full frame memcpy per frame (768 KB here, which on PSRAM
 * cost more than the encode itself). Pair with h264_encoder_encode_filled(). */
uint8_t *h264_encoder_input_buffer(h264_encoder_handle_t e, size_t *cap)
{
    if (!e) return NULL;
    if (cap) *cap = e->yuv_bytes;
    return e->out_buf[e->out_rr];
}

/* If `filled` is non-NULL it must be the pointer returned by
 * h264_encoder_input_buffer() (already populated, no copy). Otherwise `yuv` is
 * copied in as before. */
esp_err_t h264_encoder_encode_filled(h264_encoder_handle_t e, uint8_t *filled,
                                     const uint8_t *yuv, size_t len,
                                     int64_t pts_90k, h264_au_cb cb, void *ctx)
{
    ESP_RETURN_ON_FALSE(e && cb, ESP_ERR_INVALID_ARG, TAG, "args");
    ESP_RETURN_ON_FALSE(len <= e->yuv_bytes, ESP_ERR_INVALID_SIZE, TAG, "frame too big");

    static int trace_n = 0;     /* log timing for the first frames */
    int64_t t_qbuf = esp_timer_get_time();

    /* 1) Copy the raw frame into a free OUTPUT buffer and queue it. */
    uint32_t oi = e->out_rr;
    e->out_rr = (e->out_rr + 1) % e->n_out;
    if (!filled) memcpy(e->out_buf[oi], yuv, len);

    struct v4l2_buffer ob = { .type = V4L2_BUF_TYPE_VIDEO_OUTPUT,
                              .memory = V4L2_MEMORY_MMAP, .index = oi,
                              .bytesused = (uint32_t)len };
    ob.timestamp.tv_sec  = pts_90k / 90000;
    ob.timestamp.tv_usec = (pts_90k % 90000) * 1000000 / 90000;
    if (ioctl(e->fd, VIDIOC_QBUF, &ob) != 0) {
        ESP_LOGE(TAG, "QBUF OUTPUT failed: errno %d (%s)", errno, strerror(errno));
        return ESP_FAIL;
    }

    /* 2) Pump the M2M scheduler and wait for the coded frame. esp_video's M2M
     *    encoder is poll-driven and its select() tends to return at the timeout
     *    boundary rather than early on readiness, so a single long timeout costs
     *    that much on EVERY frame. Poll in short slices instead: the hardware
     *    encode finishes in a few ms, so we normally return after one slice, and
     *    a slow frame can still take several before we give up. */
    fd_set rfds;
    int s = 0;
    for (int attempt = 0; attempt < ENC_POLL_SLICES; attempt++) {
        FD_ZERO(&rfds); FD_SET(e->fd, &rfds);
        struct timeval tv = { .tv_sec = 0, .tv_usec = ENC_POLL_SLICE_MS * 1000 };
        s = select(e->fd + 1, &rfds, NULL, NULL, &tv);
        if (s != 0) break;
    }
    if (s == 0) {
        ESP_LOGW(TAG, "[enc] no coded frame within %d ms -- skipping",
                 ENC_POLL_SLICE_MS * ENC_POLL_SLICES);
        return ESP_ERR_TIMEOUT;   /* leave OUTPUT queued; don't risk a blocking DQBUF */
    }
    if (s < 0) {
        ESP_LOGE(TAG, "[enc] select failed: errno %d (%s)", errno, strerror(errno));
        return ESP_FAIL;
    }
    int64_t wait_us = esp_timer_get_time() - t_qbuf;

    /* 3) Pull the coded access unit off the CAPTURE queue. */
    struct v4l2_buffer cbz = { .type = V4L2_BUF_TYPE_VIDEO_CAPTURE, .memory = V4L2_MEMORY_MMAP };
    if (ioctl(e->fd, VIDIOC_DQBUF, &cbz) != 0) {
        ESP_LOGE(TAG, "DQBUF CAPTURE failed: errno %d (%s)", errno, strerror(errno));
        return ESP_FAIL;
    }

    /* 4) Reclaim the consumed OUTPUT buffer (ready now that the job finished). */
    struct v4l2_buffer db = { .type = V4L2_BUF_TYPE_VIDEO_OUTPUT, .memory = V4L2_MEMORY_MMAP };
    if (ioctl(e->fd, VIDIOC_DQBUF, &db) != 0)
        ESP_LOGW(TAG, "DQBUF OUTPUT failed: errno %d (%s)", errno, strerror(errno));

    bool keyframe = (cbz.flags & V4L2_BUF_FLAG_KEYFRAME) != 0;
    if (trace_n < 10) {
        ESP_LOGI(TAG, "[enc] frame %d: wait %lld ms, %u bytes, %s",
                 trace_n, wait_us / 1000, cbz.bytesused, keyframe ? "KEY" : "non-key");
        trace_n++;
    }

    /* The encoder occasionally returns an empty AU (observed: "0 bytes"). Muxing
     * a zero-length access unit gives the decoder a PES with no payload, so skip
     * it and recycle the buffer. */
    esp_err_t r = (cbz.bytesused > 0)
                ? cb(ctx, e->cap_buf[cbz.index], cbz.bytesused, keyframe, pts_90k)
                : ESP_OK;

    /* 5) Recycle the CAPTURE buffer regardless of what the sink returned. */
    struct v4l2_buffer rq = { .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
                              .memory = V4L2_MEMORY_MMAP, .index = cbz.index };
    ioctl(e->fd, VIDIOC_QBUF, &rq);
    return r;
}

esp_err_t h264_encoder_set_bitrate(h264_encoder_handle_t e, uint32_t bitrate_bps)
{
    ESP_RETURN_ON_FALSE(e, ESP_ERR_INVALID_ARG, TAG, "args");
    if (bitrate_bps < H264_BR_MIN) bitrate_bps = H264_BR_MIN;
    if (bitrate_bps > H264_BR_MAX) bitrate_bps = H264_BR_MAX;
    return set_ctrl(e->fd, V4L2_CID_MPEG_VIDEO_BITRATE, (int32_t)bitrate_bps);
}

esp_err_t h264_encoder_stop(h264_encoder_handle_t e)
{
    if (!e) return ESP_OK;
    if (e->fd >= 0) {
        int t1 = V4L2_BUF_TYPE_VIDEO_OUTPUT, t2 = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(e->fd, VIDIOC_STREAMOFF, &t1);
        ioctl(e->fd, VIDIOC_STREAMOFF, &t2);
        for (uint32_t i = 0; i < e->n_out; i++)
            if (e->out_buf[i] && e->out_buf[i] != MAP_FAILED) munmap(e->out_buf[i], e->out_len[i]);
        for (uint32_t i = 0; i < e->n_cap; i++)
            if (e->cap_buf[i] && e->cap_buf[i] != MAP_FAILED) munmap(e->cap_buf[i], e->cap_len[i]);
        close(e->fd);
    }
    free(e);
    return ESP_OK;
}