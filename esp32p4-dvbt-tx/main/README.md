# Example pipeline: OV5647 → H.264 → MPEG2-TS → IT9507 (DVB-T RF)

This is the full wireless-video chain on a single ESP32-P4. Four stages, each
in its own file:

| Stage | File | What it does | Backed by |
|------|------|--------------|-----------|
| 1. Capture | `cam_capture.[ch]` | OV5647 → CSI → ISP → **YUV420** frames | `esp_video` + `esp_cam_sensor` (V4L2 `/dev/video0`) |
| 2. Encode | `h264_encoder.[ch]` | YUV420 → **Annex-B H.264** access units | `esp_h264` HW, via `esp_video` M2M `/dev/video11` |
| 3. Mux | `ts_mux.[ch]` | H.264 AUs → **CBR MPEG2-TS** | self-contained |
| 4. Modulate | `../it9507` | TS → **DVB-T RF** over USB | the driver component |

`app_main.c` wires them: capture a frame → encode it → mux the AU → `it9507_write_ts()`.

## The format chain is the whole game

The reason this is clean: every handoff uses a format the next stage already
wants, so there is **no manual pixel juggling**.

```
OV5647 RAW10 ──ISP──► V4L2_PIX_FMT_YUV420 ──► [H.264 M2M] ──► V4L2_PIX_FMT_H264 ──► TS
                      (capture output)          (same fmt in)    (Annex-B w/ start codes)
```

The ESP32-P4 hardware encoder has an awkward *native* input layout
(`ESP_H264_RAW_FMT_O_UYY_E_VYY`). By going through `esp_video`'s **M2M H.264
device** instead of calling `esp_h264` directly, the framework handles that
layout for us and we just feed standard planar `YUV420`. That is the single
most important design choice here — it removes the most error-prone part.

> If `cam_capture_start()` fails at `S_FMT YUV420`, your sensor/ISP profile may
> only expose `YUV422P`. Run `VIDIOC_ENUM_FMT` on `/dev/video0` to see what's
> offered; either switch the ISP output to YUV420 or add a 422→420 chroma
> decimation before the encoder.

## Throughput math (read this before tuning)

Two ceilings must both be satisfied:

1. **Encoder ceiling** — the M2M H.264 device clamps bitrate to **2.5 Mbps**.
2. **Channel ceiling** — DVB-T capacity = bandwidth × constellation × code-rate ×
   guard interval. A **2 MHz / QPSK / ½ / GI ¼** channel carries **~1.2 Mbps**
   usable; 8 MHz / QAM16 gets you several Mbps.

So pick `VID_BPS ≤ channel capacity`, set the TS CBR target a touch above it
for PSI/PCR overhead, and size `resolution × fps` to that budget. The defaults
in `app_main.c` (1280×720@20, 1.8 Mbps) target a mid-width channel; for a tight
2 MHz channel use 640×480 or drop to ~1.2 Mbps. When `it9507_write_ts()` starts
returning `ESP_ERR_TIMEOUT` (logged as dropped frames), the stream is over the
channel ceiling — lower `VID_BPS` or widen the channel.

## Verified vs. needs hardware validation

**Grounded in Espressif's documented APIs:** the V4L2 capture lifecycle, the
M2M H.264 device path (`/dev/video11`, YUV420 in / H264 out, bitrate+GOP+QP
controls), the `esp_video_init()` CSI/SCCB config struct, and 64-byte buffer
alignment all follow the current `esp-video-components` interfaces.

**Needs on-target validation:**
- **The one version-sensitive seam** is `esp_video_init()`'s config struct in
  `cam_capture.c`. Field names track the `esp_video` release you pin — cross-check
  against `esp_video_init.h` if it doesn't compile. The OV5647 itself is selected
  via `menuconfig` (`CONFIG_CAMERA_OV5647`, MIPI-CSI sensor enable) and board pins.
- **YUV420 availability** from the ISP (see the note above).
- **`select()` on the video fd** for the capture timeout — supported by
  `esp_video`'s VFS driver, but if your version lacks it, drop the `select()`
  gate and let `VIDIOC_DQBUF` block.
- The **muxer's PCR/CBR pacing** (validate against a TS analyser) and the
  **IT9507 firmware blob** (you supply it) — see `../it9507/README.md`.

## Wiring (Olimex/EV-board style, adjust to yours)

SCCB (I²C) for the OV5647 defaults to SCL=8 / SDA=7 in `CAM_CAPTURE_CFG_DEFAULT()`;
MIPI-CSI uses the P4's dedicated CSI pins. The IT9507 stick is on the USB 2.0 HS
host port. Keep the USB event task and the capture loop on different cores
(`app_main.c` pins USB to core 1).
