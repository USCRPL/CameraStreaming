idf_component_register(
    SRCS
        "app_main.c"
        "ts_mux.c"
        "cam_capture.c"
        "h264_encoder.c"
        "overlay.c"
        "frame_dump.c"
        "rgb_convert.c"
    INCLUDE_DIRS "."
    REQUIRES
        it9507        # the IT9507 DVB-T modulator driver (../it9507)
        esp_video     # V4L2 capture + H.264/JPEG M2M devices, provides linux/videodev2.h
        esp_h264      # hardware H.264 codec (pulled in by esp_video, listed for clarity)
        esp_timer     # esp_timer_get_time() used in h264_encoder.c
)
