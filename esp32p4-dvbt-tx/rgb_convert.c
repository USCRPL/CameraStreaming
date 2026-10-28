/*
 * rgb_convert.c - see rgb_convert.h.
 */
#include "rgb_convert.h"
#include <stddef.h>

void rgb565_to_i420(const uint8_t *src, uint8_t *dst, int w, int h, bool byteswap)
{
    const uint16_t *s = (const uint16_t *)src;
    uint8_t *Yp = dst;
    uint8_t *Up = dst + w * h;
    uint8_t *Vp = Up + (w / 2) * (h / 2);
    const int cw = w / 2;

    for (int y = 0; y < h; y++) {
        const uint16_t *row = s + (size_t)y * w;
        uint8_t *yrow = Yp + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            uint16_t p = row[x];
            if (byteswap) p = (uint16_t)((p >> 8) | (p << 8));

            int r = (p >> 11) & 0x1F;
            int g = (p >> 5)  & 0x3F;
            int b =  p        & 0x1F;
            /* 5/6-bit -> 8-bit */
            r = (r * 527 + 23) >> 6;
            g = (g * 259 + 33) >> 6;
            b = (b * 527 + 23) >> 6;

            int Y = (77 * r + 150 * g + 29 * b) >> 8;      /* BT.601 */
            yrow[x] = (uint8_t)(Y > 255 ? 255 : Y);

            /* Point-sample chroma at the top-left of each 2x2 block. */
            if ((y & 1) == 0 && (x & 1) == 0) {
                int U = (((-43 * r - 85 * g + 128 * b) >> 8) + 128);
                int V = (((128 * r - 107 * g - 21 * b) >> 8) + 128);
                U = U < 0 ? 0 : (U > 255 ? 255 : U);
                V = V < 0 ? 0 : (V > 255 ? 255 : V);
                Up[(y >> 1) * cw + (x >> 1)] = (uint8_t)U;
                Vp[(y >> 1) * cw + (x >> 1)] = (uint8_t)V;
            }
        }
    }
}
