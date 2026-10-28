/*
 * cam_capture.h - thin V4L2 wrapper around esp_video / esp_cam_sensor for the
 *                 OV5647 on the ESP32-P4 (MIPI-CSI -> ISP -> YUV420 capture).
 *
 * This is step (1) of the pipeline: pull raw YUV420 frames out of the sensor.
 * The frames it yields are exactly what the H.264 M2M encoder wants
 * (V4L2_PIX_FMT_YUV420), so no pixel juggling is needed between the two.
 *
 * Capture device is the CSI/ISP node, conventionally /dev/video0
 * (ESP_VIDEO_MIPI_CSI_DEVICE_NAME).
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CAM_FMT_YUV420 = 0, /* planar YUV 4:2:0 — required by the H.264 M2M encoder */
    CAM_FMT_RGB565,     /* packed RGB565 — more reliable on some ISP revisions   */
} cam_fmt_t;

typedef struct {
    int        i2c_port;     /* SCCB/I2C port the OV5647 sits on            */
    int        sccb_scl_pin; /* board-specific; -1 if app already inited I2C*/
    int        sccb_sda_pin;
    int        reset_pin;    /* sensor RESET, -1 if unused                  */
    int        pwdn_pin;     /* sensor PWDN,  -1 if unused                  */
    uint32_t   width;        /* desired capture width  (e.g. 1280)          */
    uint32_t   height;       /* desired capture height (e.g. 720)           */
    uint32_t   buf_count;    /* number of capture buffers (>=2, 3-4 typical)*/
    bool       init_video;   /* true: call esp_video_init() here.           */
    cam_fmt_t  format;       /* pixel format; default YUV420 for H.264 path */
} cam_capture_cfg_t;

#define CAM_CAPTURE_CFG_DEFAULT() (cam_capture_cfg_t){ \
    .i2c_port = 0, .sccb_scl_pin = 8, .sccb_sda_pin = 7, \
    .reset_pin = -1, .pwdn_pin = -1, \
    .width = 1280, .height = 720, .buf_count = 4, \
    .init_video = true, .format = CAM_FMT_YUV420 }

typedef struct cam_capture_s *cam_capture_handle_t;

/* One dequeued frame. `data` stays valid until cam_capture_put() is called
 * with the same `index`. `len` is the filled YUV420 size (w*h*3/2). */
typedef struct {
    const uint8_t *data;
    size_t         len;
    uint32_t       index;     /* opaque buffer index, pass back to _put() */
    int64_t        ts_us;     /* driver timestamp in microseconds         */
} cam_frame_t;

/* Bring up CSI+ISP+sensor, negotiate YUV420 at cfg->width x height, allocate
 * and queue buffers, and start streaming. */
esp_err_t cam_capture_start(const cam_capture_cfg_t *cfg, cam_capture_handle_t *out);

/* Block until the next frame is ready (or timeout). On success fills *frame. */
esp_err_t cam_capture_get(cam_capture_handle_t h, cam_frame_t *frame, int timeout_ms);

/* Return a frame's buffer to the driver so it can be filled again. */
esp_err_t cam_capture_put(cam_capture_handle_t h, uint32_t index);

/* Negotiated geometry (handy for configuring the encoder). */
void cam_capture_get_format(cam_capture_handle_t h, uint32_t *w, uint32_t *h_, uint32_t *yuv_bytes);

esp_err_t cam_capture_stop(cam_capture_handle_t h);

#define MAP_FAILED (void *)-1

#ifdef __cplusplus
}
#endif
