/*
 * cam_capture.c - V4L2 capture from the OV5647 (CSI->ISP) yielding YUV420.
 *
 * Uses esp_video's V4L2 shim. The flow is the textbook one:
 *   esp_video_init -> open -> S_FMT -> REQBUFS(MMAP) -> QUERYBUF+mmap ->
 *   QBUF all -> STREAMON -> { DQBUF -> use -> QBUF } -> STREAMOFF.
 *
 * The ONE board/version-specific seam is esp_video_init()'s config struct -
 * field names track the esp_video release you pin. Cross-check against
 * esp_video_init.h if it doesn't compile; everything below it is stock V4L2.
 */
#include "cam_capture.h"

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>

#include "linux/videodev2.h"
#include "esp_video_init.h"
#include "esp_video_device.h"   /* ESP_VIDEO_MIPI_CSI_DEVICE_NAME ("/dev/video0") */
#include "esp_log.h"
#include "esp_check.h"

#define TAG "cam_capture"
#ifndef ESP_VIDEO_MIPI_CSI_DEVICE_NAME
#define ESP_VIDEO_MIPI_CSI_DEVICE_NAME "/dev/video0"
#endif
#define CAM_MAX_BUFS 8

struct cam_capture_s {
    int      fd;
    uint32_t width, height, yuv_bytes;
    uint32_t n_bufs;
    void    *bufs[CAM_MAX_BUFS];
    size_t   buf_len[CAM_MAX_BUFS];
};

/* Percentage of each control's range to use when we must set exposure/gain
 * manually (no auto available). Raise if the picture is still dark, lower if it
 * blows out / gets noisy. */
#define EXPOSURE_TARGET_PCT  70
#define GAIN_TARGET_PCT      60

static bool set_ctrl(int fd, uint32_t id, int32_t val, const char *what)
{
    struct v4l2_control c = { .id = id, .value = val };
    if (ioctl(fd, VIDIOC_S_CTRL, &c) == 0) {
        ESP_LOGI(TAG, "  set %s = %d  OK", what, (int)val);
        return true;
    }
    return false;
}

/* Set a control to a percentage of its own reported range. If QUERYCTRL is not
 * implemented (common on esp_video) we can't read the range, so fall back to a
 * caller-supplied absolute value -- S_CTRL frequently works even where
 * QUERYCTRL does not. */
static bool boost_ctrl(int fd, uint32_t id, int pct, const char *what,
                       int32_t fallback_abs)
{
    struct v4l2_queryctrl q = { .id = id };
    if (ioctl(fd, VIDIOC_QUERYCTRL, &q) == 0 &&
        !(q.flags & V4L2_CTRL_FLAG_DISABLED) && q.maximum > q.minimum) {
        int32_t v = q.minimum + (int32_t)(((int64_t)(q.maximum - q.minimum) * pct) / 100);
        if (v > q.maximum) v = q.maximum;
        return set_ctrl(fd, id, v, what);
    }
    return set_ctrl(fd, id, fallback_abs, what);
}

static void cam_apply_exposure(int fd)
{
    /* 1) Probe controls INDIVIDUALLY by id. esp_video does not implement the
     * V4L2_CTRL_FLAG_NEXT_CTRL iteration, so the "walk the list" method reports
     * nothing even when the controls exist -- probe each id directly instead. */
    static const struct { uint32_t id; const char *name; } probe[] = {
        { V4L2_CID_EXPOSURE,          "EXPOSURE"          },
        { V4L2_CID_EXPOSURE_ABSOLUTE, "EXPOSURE_ABSOLUTE" },
        { V4L2_CID_EXPOSURE_AUTO,     "EXPOSURE_AUTO"     },
        { V4L2_CID_GAIN,              "GAIN"              },
        { V4L2_CID_ANALOGUE_GAIN,     "ANALOGUE_GAIN"     },
        { V4L2_CID_AUTOGAIN,          "AUTOGAIN"          },
        { V4L2_CID_BRIGHTNESS,        "BRIGHTNESS"        },
        { V4L2_CID_CONTRAST,          "CONTRAST"          },
        { V4L2_CID_SATURATION,        "SATURATION"        },
        { V4L2_CID_GAMMA,             "GAMMA"             },
        { V4L2_CID_AUTO_WHITE_BALANCE,"AUTO_WHITE_BALANCE"},
        { V4L2_CID_RED_BALANCE,       "RED_BALANCE"       },
        { V4L2_CID_BLUE_BALANCE,      "BLUE_BALANCE"      },
    };
    ESP_LOGW(TAG, "--- probing sensor/ISP controls individually ---");
    int found = 0;
    for (size_t i = 0; i < sizeof(probe) / sizeof(probe[0]); i++) {
        struct v4l2_queryctrl q = { .id = probe[i].id };
        if (ioctl(fd, VIDIOC_QUERYCTRL, &q) == 0 &&
            !(q.flags & V4L2_CTRL_FLAG_DISABLED)) {
            struct v4l2_control cur = { .id = probe[i].id };
            int have = (ioctl(fd, VIDIOC_G_CTRL, &cur) == 0);
            ESP_LOGW(TAG, "  %-18s min=%d max=%d def=%d cur=%s%d", probe[i].name,
                     (int)q.minimum, (int)q.maximum, (int)q.default_value,
                     have ? "" : "?", have ? (int)cur.value : 0);
            found++;
        } else {
            /* QUERYCTRL may be unimplemented even where S_CTRL works: try a read */
            struct v4l2_control cur = { .id = probe[i].id };
            if (ioctl(fd, VIDIOC_G_CTRL, &cur) == 0) {
                ESP_LOGW(TAG, "  %-18s (no QUERYCTRL) cur=%d", probe[i].name,
                         (int)cur.value);
                found++;
            }
        }
    }
    if (!found) ESP_LOGW(TAG, "  none readable -- exposure lives in the ISP IPA "
                              "(set the OV5647 IPA JSON, not V4L2)");

    /* 2) Prefer automatic exposure / gain / white balance. AWB matters here too:
     * the picture carries a green-teal cast, i.e. unbalanced channel gain. */
    bool auto_exp = set_ctrl(fd, V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_AUTO,
                             "EXPOSURE_AUTO");
    bool auto_gain = set_ctrl(fd, V4L2_CID_AUTOGAIN, 1, "AUTOGAIN");
    set_ctrl(fd, V4L2_CID_AUTO_WHITE_BALANCE, 1, "AUTO_WHITE_BALANCE");

    /* 3) Whatever stayed manual, push up. The absolute fallbacks apply when
     * QUERYCTRL is missing: OV5647 exposure is in lines (several hundred is a
     * long exposure at 800x640) and gain is a register in the low hundreds. */
    if (!auto_exp) {
        if (!boost_ctrl(fd, V4L2_CID_EXPOSURE_ABSOLUTE, EXPOSURE_TARGET_PCT,
                        "EXPOSURE_ABSOLUTE", 600))
            boost_ctrl(fd, V4L2_CID_EXPOSURE, EXPOSURE_TARGET_PCT, "EXPOSURE", 600);
    }
    if (!auto_gain) {
        if (!boost_ctrl(fd, V4L2_CID_ANALOGUE_GAIN, GAIN_TARGET_PCT,
                        "ANALOGUE_GAIN", 200))
            boost_ctrl(fd, V4L2_CID_GAIN, GAIN_TARGET_PCT, "GAIN", 200);
    }
}


static esp_err_t do_video_init(const cam_capture_cfg_t *cfg)
{
    /* --- esp_video_init() seam ---------------------------------------------
     * The CSI sensor (OV5647) is selected/built via menuconfig
     * (CONFIG_CAMERA_OV5647=y, CONFIG_EXAMPLE_ENABLE_MIPI_CSI_CAM_SENSOR=y).
     * Here we only hand esp_video the SCCB (I2C) wiring + reset/pwdn so it can
     * probe the sensor. If your app already created the I2C master bus, set
     * init_sccb=false and pass the port instead.
     */
    esp_video_init_sccb_config_t sccb = {
        .init_sccb = true,
        .i2c_config = {
            .port     = cfg->i2c_port,
            .scl_pin  = cfg->sccb_scl_pin,
            .sda_pin  = cfg->sccb_sda_pin,
        },
        .freq = 100000,
    };
    esp_video_init_csi_config_t csi = {
        .sccb_config = sccb,
        .reset_pin   = cfg->reset_pin,
        .pwdn_pin    = cfg->pwdn_pin,
    };
    esp_video_init_config_t vcfg = { .csi = &csi };
    return esp_video_init(&vcfg);
}

esp_err_t cam_capture_start(const cam_capture_cfg_t *cfg, cam_capture_handle_t *out)
{
    ESP_RETURN_ON_FALSE(cfg && out, ESP_ERR_INVALID_ARG, TAG, "args");
    ESP_RETURN_ON_FALSE(cfg->buf_count >= 2 && cfg->buf_count <= CAM_MAX_BUFS,
                        ESP_ERR_INVALID_ARG, TAG, "buf_count 2..%d", CAM_MAX_BUFS);

    if (cfg->init_video) {
        ESP_RETURN_ON_ERROR(do_video_init(cfg), TAG, "esp_video_init failed");
    }

    struct cam_capture_s *c = calloc(1, sizeof(*c));
    ESP_RETURN_ON_FALSE(c, ESP_ERR_NO_MEM, TAG, "oom");
    c->fd = -1;

    esp_err_t ret = ESP_OK;
    c->fd = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDWR);
    ESP_GOTO_ON_FALSE(c->fd >= 0, ESP_FAIL, fail, TAG, "open %s",
                      ESP_VIDEO_MIPI_CSI_DEVICE_NAME);

    /* Pick the V4L2 fourcc + bytes/frame for the requested format. RGB565 is the
     * format the stock capture_stream example streams reliably on this sensor;
     * YUV420 (planar) goes through the ISP's interpolation path which can be
     * fragile. If S_FMT rejects a format, VIDIOC_ENUM_FMT lists what the ISP
     * actually offers. */
    uint32_t fourcc, bpp_num, bpp_den;
    switch (cfg->format) {
    case CAM_FMT_RGB565: fourcc = V4L2_PIX_FMT_RGB565; bpp_num = 2; bpp_den = 1; break;
    case CAM_FMT_YUV420:
    default:             fourcc = V4L2_PIX_FMT_YUV420; bpp_num = 3; bpp_den = 2; break;
    }

    struct v4l2_format fmt = { 0 };
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = cfg->width;
    fmt.fmt.pix.height      = cfg->height;
    fmt.fmt.pix.pixelformat = fourcc;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
    ESP_GOTO_ON_FALSE(ioctl(c->fd, VIDIOC_S_FMT, &fmt) == 0, ESP_FAIL, fail,
                      TAG, "S_FMT %ux%u failed", cfg->width, cfg->height);

    c->width     = fmt.fmt.pix.width;
    c->height    = fmt.fmt.pix.height;
    c->yuv_bytes = c->width * c->height * bpp_num / bpp_den;

    /* Log EXACTLY what the driver negotiated. If bytesperline != width*bpp the
     * lines are padded (stride > width) and reading the buffer as tightly packed
     * wraps/repeats -> torn image. If the fourcc isn't what we asked, the ISP
     * substituted a format. */
    {
        uint32_t pf = fmt.fmt.pix.pixelformat;
        ESP_LOGI(TAG, "G_FMT: %ux%u fourcc=%c%c%c%c bytesperline=%u sizeimage=%u (expected stride=%u)",
                 fmt.fmt.pix.width, fmt.fmt.pix.height,
                 (char)(pf & 0xff), (char)((pf >> 8) & 0xff),
                 (char)((pf >> 16) & 0xff), (char)((pf >> 24) & 0xff),
                 fmt.fmt.pix.bytesperline, fmt.fmt.pix.sizeimage,
                 c->width * bpp_num / bpp_den);
    }

    struct v4l2_requestbuffers req = { 0 };
    req.count  = cfg->buf_count;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ESP_GOTO_ON_FALSE(ioctl(c->fd, VIDIOC_REQBUFS, &req) == 0 && req.count >= 2,
                      ESP_FAIL, fail, TAG, "REQBUFS failed");
    c->n_bufs = req.count;

    for (uint32_t i = 0; i < c->n_bufs; i++) {
        struct v4l2_buffer b = { 0 };
        b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        b.memory = V4L2_MEMORY_MMAP;
        b.index = i;
        ESP_GOTO_ON_FALSE(ioctl(c->fd, VIDIOC_QUERYBUF, &b) == 0, ESP_FAIL, fail,
                          TAG, "QUERYBUF %u", i);
        c->buf_len[i] = b.length;
        c->bufs[i] = mmap(NULL, b.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                          c->fd, b.m.offset);
        ESP_GOTO_ON_FALSE(c->bufs[i] != MAP_FAILED, ESP_FAIL, fail, TAG,
                          "mmap %u", i);
        ESP_GOTO_ON_FALSE(ioctl(c->fd, VIDIOC_QBUF, &b) == 0, ESP_FAIL, fail,
                          TAG, "QBUF %u", i);
    }

    /* ---- exposure / gain -------------------------------------------------
     * The frame arrives far too dark (measured avg R/G/B ~16/12/8), which also
     * kills colour: at that level U,V land within +-3 of neutral 128, so the
     * picture is effectively greyscale. Brightness and colour are the SAME bug.
     * We don't know which controls this driver exposes, so enumerate them (the
     * log lists every one with its range), then prefer auto exposure/gain and
     * fall back to a manual boost. Tune EXPOSURE_TARGET_PCT if needed. */
    cam_apply_exposure(c->fd);

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ESP_GOTO_ON_FALSE(ioctl(c->fd, VIDIOC_STREAMON, &type) == 0, ESP_FAIL, fail,
                      TAG, "STREAMON");

    ESP_LOGI(TAG, "capture started %ux%u %s (%u bytes/frame), %u buffers",
             c->width, c->height,
             cfg->format == CAM_FMT_RGB565 ? "RGB565" : "YUV420",
             c->yuv_bytes, c->n_bufs);
    *out = c;
    return ESP_OK;

fail:
    cam_capture_stop(c);
    return ret;
}

esp_err_t cam_capture_get(cam_capture_handle_t c, cam_frame_t *frame, int timeout_ms)
{
    ESP_RETURN_ON_FALSE(c && frame, ESP_ERR_INVALID_ARG, TAG, "args");

    /* esp_video's DQBUF blocks; gate it with select() for a real timeout. */
    if (timeout_ms >= 0) {
        fd_set fds; FD_ZERO(&fds); FD_SET(c->fd, &fds);
        struct timeval tv = { .tv_sec = timeout_ms / 1000,
                              .tv_usec = (timeout_ms % 1000) * 1000 };
        int r = select(c->fd + 1, &fds, NULL, NULL, &tv);
        if (r == 0) return ESP_ERR_TIMEOUT;
        if (r < 0)  return ESP_FAIL;
    }

    struct v4l2_buffer b = { 0 };
    b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    b.memory = V4L2_MEMORY_MMAP;
    ESP_RETURN_ON_FALSE(ioctl(c->fd, VIDIOC_DQBUF, &b) == 0, ESP_FAIL, TAG, "DQBUF");

    frame->index = b.index;
    frame->data  = c->bufs[b.index];
    frame->len   = b.bytesused ? b.bytesused : c->yuv_bytes;
    frame->ts_us = (int64_t)b.timestamp.tv_sec * 1000000 + b.timestamp.tv_usec;
    return ESP_OK;
}

esp_err_t cam_capture_put(cam_capture_handle_t c, uint32_t index)
{
    ESP_RETURN_ON_FALSE(c && index < c->n_bufs, ESP_ERR_INVALID_ARG, TAG, "args");
    struct v4l2_buffer b = { 0 };
    b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    b.memory = V4L2_MEMORY_MMAP;
    b.index = index;
    ESP_RETURN_ON_FALSE(ioctl(c->fd, VIDIOC_QBUF, &b) == 0, ESP_FAIL, TAG, "QBUF");
    return ESP_OK;
}

void cam_capture_get_format(cam_capture_handle_t c, uint32_t *w, uint32_t *h_, uint32_t *yuv_bytes)
{
    if (!c) return;
    if (w) *w = c->width;
    if (h_) *h_ = c->height;
    if (yuv_bytes) *yuv_bytes = c->yuv_bytes;
}

esp_err_t cam_capture_stop(cam_capture_handle_t c)
{
    if (!c) return ESP_OK;
    if (c->fd >= 0) {
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(c->fd, VIDIOC_STREAMOFF, &type);
        for (uint32_t i = 0; i < c->n_bufs; i++) {
            if (c->bufs[i] && c->bufs[i] != MAP_FAILED) munmap(c->bufs[i], c->buf_len[i]);
        }
        close(c->fd);
    }
    free(c);
    return ESP_OK;
}