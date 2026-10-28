/*
 * overlay.h - burn a callsign (or any text) into a YUV420 frame in place.
 *
 * Why this stage exists: an on-screen station ID has to be part of the picture
 * the receiver decodes, so it must be drawn on the RAW frame BEFORE the H.264
 * encoder - you cannot legibly add it after compression. It slots into the
 * pipeline as: capture -> [overlay] -> encode -> mux -> modulate.
 *
 * Implementation: text is drawn straight into the luma (Y) plane - white glyphs
 * over an optional darkened, chroma-neutralised box. Only a few thousand byte
 * writes per frame, so the CPU cost is negligible and no extra buffer/copy is
 * introduced (the capture buffer is mmap'd writable and reused by the encoder).
 *
 * For full-frame compositing (logos, alpha-blended bugs) the ESP32-P4 PPA can
 * do it in hardware (driver/ppa.h, ppa_do_blend); for a text callsign the
 * direct Y-plane write here is simpler and lower latency.
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Anchor corner for placement, so you don't have to compute pixel coords by
 * hand for different resolutions. */
typedef enum {
    OVL_TOP_LEFT, OVL_TOP_RIGHT, OVL_BOTTOM_LEFT, OVL_BOTTOM_RIGHT
} overlay_anchor_t;

typedef struct {
    overlay_anchor_t anchor;
    int      margin;     /* pixels from the chosen corner (e.g. 12)          */
    int      scale;      /* integer glyph scale: 1=8x16, 2=16x32, 3=24x48    */
    bool     box;        /* draw a darkened, chroma-neutral background box   */
    uint8_t  fg_y;       /* glyph luma (235 = broadcast white)               */
    uint8_t  box_y;      /* box luma   (16  = broadcast black)               */
} overlay_style_t;

#define OVERLAY_STYLE_DEFAULT() (overlay_style_t){ \
    .anchor = OVL_BOTTOM_RIGHT, .margin = 12, .scale = 2, \
    .box = true, .fg_y = 235, .box_y = 16 }

/* Draw `text` into the YUV420 frame `yuv` (w*h Y, then w/2*h/2 U, then V).
 * w and h must be even. Text is upper-cased to match the font; unknown glyphs
 * render as blanks. Clipped to the frame edges. */
void overlay_draw_text(uint8_t *yuv, int w, int h, const char *text,
                       const overlay_style_t *st);

/* Same, but the buffer is packed O_UYY_E_VYY (the encoder's native layout). */
void overlay_draw_text_packed(uint8_t *buf, int w, int h, const char *text,
                              const overlay_style_t *st);

#ifdef __cplusplus
}
#endif
