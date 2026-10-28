# esp32p4-dvbt-tx — OV5647 → H.264 → MPEG2-TS → IT9507 DVB-T transmitter

Single-ESP32-P4 wireless video: capture from an OV5647, hardware H.264 encode,
burn in an FCC station-ID callsign, mux to a constant-bit-rate MPEG2-TS, and
modulate it to DVB-T RF through an IT9507 USB stick (P4 acts as USB host).

```
OV5647 ─CSI─► ISP ─YUV420─► overlay ─► H.264 HW ─► ts_mux (CBR TS) ─► IT9507 ─► RF
```

## Layout

```
esp32p4-dvbt-tx/
├── CMakeLists.txt              project file
├── sdkconfig.defaults          target + key options
├── main/                       the application (V4L2 capture, encode, overlay, mux, glue)
│   ├── app_main.c              pipeline wiring
│   ├── cam_capture.{c,h}       OV5647 V4L2 capture  (esp_video/esp_cam_sensor)
│   ├── h264_encoder.{c,h}      HW H.264 via V4L2 M2M (/dev/video11)
│   ├── overlay.{c,h}           callsign burn-in (luma-plane text)
│   ├── overlay_font.h          generated 8x16 bitmap font
│   ├── ts_mux.{c,h}            CBR MPEG2-TS muxer (PCR/PSI/PES + null pad)
│   ├── CMakeLists.txt / idf_component.yml
│   └── README.md               pipeline + throughput notes
└── components/
    └── it9507/                 reusable DVB-T modulator driver (USB host)
        ├── include/            public API:  it9507.h, it9507_regs.h
        ├── priv_include/       internal:    it9507_private.h, it9507_firmware.h
        ├── src/                it9507_core.c, it9507_usb.c
        ├── CMakeLists.txt / idf_component.yml
        └── README.md           wiring, firmware blob, caveats
```

## Build

```sh
idf.py set-target esp32p4
# 1) drop the 4560-byte IT9507 firmware blob into
#    components/it9507/priv_include/it9507_firmware.h  (see that file)
# 2) enable it in components/it9507/CMakeLists.txt:
#    target_compile_definitions(${COMPONENT_LIB} PRIVATE IT9507_HAVE_FIRMWARE=1)
# 3) set your callsign in main/app_main.c (CALLSIGN)
idf.py build flash monitor
```

## Status

The two self-contained units (`ts_mux`, `overlay`) are host-compiled and the TS
output is validated as well-formed CBR (sync/continuity/PCR/CRC all clean). The
ESP-IDF-dependent units are syntax-checked against the documented APIs. The
remaining on-target items — firmware blob, `esp_video` init field names for your
pinned version, YUV420 availability from the ISP, and the IT9507 `set_ts_interface`
tail — are called out in the per-component READMEs.
