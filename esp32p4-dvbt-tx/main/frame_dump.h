/*
 * frame_dump.h - dump one YUV420 frame over the serial console as base64,
 * for viewing a captured image without a display. Reconstruct on the PC with
 * tools/yuv_to_png.py.
 *
 * Debug aid: call once (it's slow - it streams the whole frame over UART).
 */
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Emit the frame between ---FRAME_DUMP_BEGIN--- / ---FRAME_DUMP_END--- markers,
 * tagged with `fmt` (e.g. "i420" or "rgb565") so the PC decoder knows the layout.
 * Other tasks' ESP_LOG output is silenced during the dump so it can't corrupt
 * the base64 stream. */
void frame_dump_base64(const uint8_t *data, size_t len, int w, int h, const char *fmt);

#ifdef __cplusplus
}
#endif
