/*
 * h264_encoder.h - thin wrapper around the ESP32-P4 hardware H.264 encoder,
 *                  exposed by esp_video as a V4L2 M2M device (/dev/video11).
 *
 * Step (2) of the pipeline: YUV420 frame in -> Annex-B H.264 access unit out.
 *
 * M2M model: you queue a raw frame on the OUTPUT queue and dequeue the coded
 * frame from the CAPTURE queue. Input is V4L2_PIX_FMT_YUV420, output is
 * V4L2_PIX_FMT_H264. The hardware emits Constrained-Baseline Annex-B with
 * start codes - exactly what ts_mux wants.
 *
 * NOTE: the M2M encoder's bitrate control clamps to [25 kbps, 2.5 Mbps]. That
 * upper bound happens to sit right in DVB-T narrow-channel territory, so it is
 * a natural fit for this project.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t width, height;  /* must match the capture geometry            */
    uint32_t bitrate_bps;    /* CBR target; clamped to <= 2.5 Mbps          */
    uint32_t gop;            /* I-frame period in frames (e.g. fps -> 1s)   */
    uint32_t fps;            /* for rate control bookkeeping                */
    uint8_t  qp_min, qp_max; /* 0..51; e.g. 20..40                          */
    uint32_t out_buf_count;  /* OUTPUT(raw) buffers, >=1 (2 is plenty)      */
    uint32_t cap_buf_count;  /* CAPTURE(h264) buffers, >=1 (2 is plenty)    */
} h264_encoder_cfg_t;

#define H264_ENCODER_CFG_DEFAULT() (h264_encoder_cfg_t){ \
    .width = 800, .height = 640, .bitrate_bps = 800000, \
    .gop = 20, .fps = 20, .qp_min = 20, .qp_max = 40, \
    .out_buf_count = 2, .cap_buf_count = 2 }

typedef struct h264_encoder_s *h264_encoder_handle_t;

/* Called once per encoded access unit while its buffer is still mapped.
 * Copy/consume `au` before returning; the buffer is recycled afterwards. */
typedef esp_err_t (*h264_au_cb)(void *ctx, const uint8_t *au, size_t len,
                                bool keyframe, int64_t pts_90k);

esp_err_t h264_encoder_start(const h264_encoder_cfg_t *cfg, h264_encoder_handle_t *out);

/* Encode one YUV420 frame (w*h*3/2 bytes). Drives the M2M dance and invokes
 * `cb` with the resulting access unit. Synchronous (one in, one out). */
esp_err_t h264_encoder_encode(h264_encoder_handle_t h, const uint8_t *yuv, size_t len,
                              int64_t pts_90k, h264_au_cb cb, void *ctx);

/* Zero-copy path: get the next input buffer, build the frame directly into it,
 * then submit with filled = that pointer. Saves a full-frame memcpy per frame
 * (768 KB here -- on PSRAM that cost more time than the encode itself). */
uint8_t *h264_encoder_input_buffer(h264_encoder_handle_t h, size_t *cap);
esp_err_t h264_encoder_encode_filled(h264_encoder_handle_t h, uint8_t *filled,
                                     const uint8_t *yuv, size_t len,
                                     int64_t pts_90k, h264_au_cb cb, void *ctx);

/* Adjust CBR target at runtime (e.g. if the DVB-T channel changes). */
esp_err_t h264_encoder_set_bitrate(h264_encoder_handle_t h, uint32_t bitrate_bps);

esp_err_t h264_encoder_stop(h264_encoder_handle_t h);

#define MAP_FAILED (void *)-1
#ifdef __cplusplus
}
#endif
