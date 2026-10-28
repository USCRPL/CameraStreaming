/*
 * overlay.c - YUV420 in-place text overlay. See overlay.h.
 */
#include "overlay.h"
#include "overlay_font.h"
#include <string.h>

static inline uint8_t upcase(uint8_t ch)
{
    return (ch >= 'a' && ch <= 'z') ? (uint8_t)(ch - 'a' + 'A') : ch;
}

/* Fill a luma rectangle, clipped to the frame. */

/* ---- packed O_UYY_E_VYY support -------------------------------------------
 * The encoder consumes packed rows of [C, Y, Y] (w*3/2 bytes per row) rather
 * than planar I420. Rather than duplicate the glyph-layout code, the two fill
 * helpers below switch on this flag, so overlay_draw_text() works on either
 * layout. Luma for pixel x sits at (x/2)*3 + 1 + (x&1); the chroma byte for the
 * pair sits at (x/2)*3. */
static int s_packed_mode = 0;

static void fill_y(uint8_t *Y, int w, int h, int x0, int y0, int bw, int bh, uint8_t v)
{
    int x1 = x0 + bw, y1 = y0 + bh;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > w) x1 = w;
    if (y1 > h) y1 = h;
    if (s_packed_mode) {
        size_t orow = (size_t)w * 3 / 2;
        for (int y = y0; y < y1; y++) {
            uint8_t *row = Y + (size_t)y * orow;
            for (int x = x0; x < x1; x++)
                row[(size_t)(x >> 1) * 3 + 1 + (x & 1)] = v;
        }
        return;
    }
    for (int y = y0; y < y1; y++)
        memset(&Y[(size_t)y * w + x0], v, (size_t)(x1 - x0));
}

/* Neutralise chroma (U=V=128) over a luma-space rectangle, clipped. */
static void neutralise_chroma(uint8_t *yuv, int w, int h, int x0, int y0, int bw, int bh)
{
    if (s_packed_mode) {
        size_t orow = (size_t)w * 3 / 2;
        int px1 = x0 + bw, py1 = y0 + bh;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (px1 > w) px1 = w;
        if (py1 > h) py1 = h;
        for (int y = y0; y < py1; y++) {
            uint8_t *row = yuv + (size_t)y * orow;
            for (int x = x0; x < px1; x += 2)
                row[(size_t)(x >> 1) * 3] = 128;
        }
        return;
    }
    uint8_t *U = yuv + (size_t)w * h;
    uint8_t *V = U + (size_t)(w / 2) * (h / 2);
    int cw = w / 2, ch = h / 2;
    int cx0 = x0 / 2, cy0 = y0 / 2;
    int cx1 = (x0 + bw + 1) / 2, cy1 = (y0 + bh + 1) / 2;
    if (cx0 < 0) cx0 = 0;
    if (cy0 < 0) cy0 = 0;
    if (cx1 > cw) cx1 = cw;
    if (cy1 > ch) cy1 = ch;
    for (int y = cy0; y < cy1; y++) {
        memset(&U[(size_t)y * cw + cx0], 128, (size_t)(cx1 - cx0));
        memset(&V[(size_t)y * cw + cx0], 128, (size_t)(cx1 - cx0));
    }
}

void overlay_draw_text(uint8_t *yuv, int w, int h, const char *text,
                       const overlay_style_t *st)
{
    if (!yuv || !text || !st) return;
    int scale = st->scale > 0 ? st->scale : 1;
    int n = (int)strlen(text);
    if (n == 0) return;

    int gw = OVL_FONT_W * scale;            /* per-glyph pixel width  */
    int gh = OVL_FONT_H * scale;            /* per-glyph pixel height */
    int pad = 2 * scale;                    /* box padding            */
    int text_w = n * gw;
    int box_w = text_w + 2 * pad;
    int box_h = gh + 2 * pad;

    /* place the box from the chosen corner */
    int bx, by;
    switch (st->anchor) {
        case OVL_TOP_LEFT:     bx = st->margin;            by = st->margin;            break;
        case OVL_TOP_RIGHT:    bx = w - box_w - st->margin; by = st->margin;            break;
        case OVL_BOTTOM_LEFT:  bx = st->margin;            by = h - box_h - st->margin; break;
        default:               bx = w - box_w - st->margin; by = h - box_h - st->margin; break;
    }
    int tx = bx + pad, ty = by + pad;       /* text origin inside box */

    if (st->box) {
        fill_y(yuv, w, h, bx, by, box_w, box_h, st->box_y);
        neutralise_chroma(yuv, w, h, bx, by, box_w, box_h);
    }

    /* draw glyphs into the Y plane */
    for (int ci = 0; ci < n; ci++) {
        uint8_t ch = upcase((uint8_t)text[ci]);
        if (ch < OVL_FONT_LO || ch > OVL_FONT_HI) ch = ' ';
        const uint8_t *gl = OVL_FONT[ch - OVL_FONT_LO];
        int ox = tx + ci * gw;
        for (int row = 0; row < OVL_FONT_H; row++) {
            uint8_t bits = gl[row];
            if (!bits) continue;
            for (int col = 0; col < OVL_FONT_W; col++) {
                if (!(bits & (1 << (7 - col)))) continue;
                fill_y(yuv, w, h, ox + col * scale, ty + row * scale,
                       scale, scale, st->fg_y);
            }
        }
    }
}

/* Same as overlay_draw_text(), but the buffer is packed O_UYY_E_VYY. */
void overlay_draw_text_packed(uint8_t *buf, int w, int h, const char *text,
                              const overlay_style_t *st)
{
    s_packed_mode = 1;
    overlay_draw_text(buf, w, h, text, st);
    s_packed_mode = 0;
}
