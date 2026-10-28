/*
 * rgb_convert.c - see rgb_convert.h.
 *
 * Processes one 2x2 pixel block per iteration: four Y samples plus one
 * subsampled chroma pair, with no per-pixel branch. Y needs no clamp because the
 * BT.601 luma coefficients sum to exactly 256 (77+150+29), so the result is
 * always 0..255. Pointers are restrict-qualified and rows are walked
 * sequentially to keep the PSRAM prefetcher happy.
 */
#include "rgb_convert.h"
#include <stddef.h>
#include "esp_log.h"

/* RGB565 5/6-bit channel -> 8-bit. */
#define R5(p) (((( (p) >> 11) & 0x1F) * 527 + 23) >> 6)
#define G6(p) ((((( (p) >> 5) & 0x3F)) * 259 + 33) >> 6)
#define B5(p) ((((  (p)       & 0x1F)) * 527 + 23) >> 6)
#define SWAP16(p) ((uint16_t)(((p) >> 8) | ((p) << 8)))

/* The ISP emits BGR565, not RGB565: a red apple decoded as blue, and skin tones
 * came out blue-teal -- both are R and B exchanged. Note this is NOT the same as
 * RGB565_BYTESWAP (which reverses byte order); here the byte order is right but
 * the red and blue FIELDS are the other way round within the 16-bit pixel.
 * Set to 0 if a red object ever decodes as red without it. */
#define RGB565_RB_SWAP 1
#if RGB565_RB_SWAP
#define CH_R(p) B5(p)
#define CH_B(p) R5(p)
#else
#define CH_R(p) R5(p)
#define CH_B(p) B5(p)
#endif

/* Per-channel white-balance gains, Q8 (256 = 1.0x). The OV5647 ISP (RAW8 Bayer,
 * "Default" IPA) can emit a green-dominant frame -- if so, boost R and B (or cut
 * G) here until whites look neutral. Flash once at 256/256/256 and read the
 * "avg R/G/B" log line to see the imbalance, then set these to correct it
 * (e.g. if G is ~2x R and B, use ~460/256/460). Neutral by default = no change. */
#ifndef WB_R_GAIN_Q8
#define WB_R_GAIN_Q8 256
#endif
#ifndef WB_G_GAIN_Q8
#define WB_G_GAIN_Q8 256
#endif
#ifndef WB_B_GAIN_Q8
#define WB_B_GAIN_Q8 256
#endif

static inline uint8_t clamp8(int v)
{
    return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
}

/* white-balanced channel extraction */
#define WR(p) clamp8((CH_R(p) * WB_R_GAIN_Q8) >> 8)
#define WG(p) clamp8((G6(p) * WB_G_GAIN_Q8) >> 8)
#define WB(p) clamp8((CH_B(p) * WB_B_GAIN_Q8) >> 8)

void rgb565_to_i420(const uint8_t *restrict src, uint8_t *restrict dst,
                    int w, int h, bool byteswap)
{
    const uint16_t *restrict s = (const uint16_t *restrict)src;

    /* One-time channel-average probe: quantifies any color cast (e.g. a green-
     * dominant ISP frame shows G far above R and B). Sampled cheaply. */
    static int probed = 0;
    if (!probed) {
        probed = 1;
        uint64_t sr = 0, sg = 0, sb = 0; int n = 0;
        for (int i = 0; i < w * h; i += 37) {
            uint16_t p = byteswap ? SWAP16(s[i]) : s[i];
            sr += CH_R(p); sg += G6(p); sb += CH_B(p); n++;
        }
        if (n) ESP_LOGW("rgbconv", "avg R/G/B = %u / %u / %u  (green cast if G >> R,B)",
                        (unsigned)(sr / n), (unsigned)(sg / n), (unsigned)(sb / n));
    }
    uint8_t *restrict Yp = dst;
    uint8_t *restrict Up = dst + (size_t)w * h;
    uint8_t *restrict Vp = Up + (size_t)(w / 2) * (h / 2);
    const int cw = w / 2;

    for (int y = 0; y < h; y += 2) {
        const uint16_t *restrict r0 = s + (size_t)y * w;
        const uint16_t *restrict r1 = r0 + w;
        uint8_t *restrict y0 = Yp + (size_t)y * w;
        uint8_t *restrict y1 = y0 + w;
        uint8_t *restrict up = Up + (size_t)(y / 2) * cw;
        uint8_t *restrict vp = Vp + (size_t)(y / 2) * cw;

        for (int x = 0; x < w; x += 2) {
            uint16_t p00 = r0[x], p01 = r0[x + 1];
            uint16_t p10 = r1[x], p11 = r1[x + 1];
            if (byteswap) { p00 = SWAP16(p00); p01 = SWAP16(p01);
                            p10 = SWAP16(p10); p11 = SWAP16(p11); }

            /* top-left pixel: luma + the block's chroma */
            int r = WR(p00), g = WG(p00), b = WB(p00);
            y0[x]     = (uint8_t)((77 * r + 150 * g + 29 * b) >> 8);
            up[x >> 1] = clamp8(((-43 * r - 85 * g + 128 * b) >> 8) + 128);
            vp[x >> 1] = clamp8((( 128 * r - 107 * g - 21 * b) >> 8) + 128);

            /* other three pixels: luma only */
            r = WR(p01); g = WG(p01); b = WB(p01);
            y0[x + 1] = (uint8_t)((77 * r + 150 * g + 29 * b) >> 8);
            r = WR(p10); g = WG(p10); b = WB(p10);
            y1[x]     = (uint8_t)((77 * r + 150 * g + 29 * b) >> 8);
            r = WR(p11); g = WG(p11); b = WB(p11);
            y1[x + 1] = (uint8_t)((77 * r + 150 * g + 29 * b) >> 8);
        }
    }
}

/* I420 -> O_UYY_E_VYY (ESP32-P4 encoder native packed 4:2:0). See header. */
void i420_to_uyy_vyy(const uint8_t *restrict src, uint8_t *restrict dst,
                     int w, int h)
{
    const uint8_t *restrict Y = src;
    const uint8_t *restrict U = src + (size_t)w * h;
    const uint8_t *restrict V = U + (size_t)(w / 2) * (h / 2);
    const int cw = w / 2;
    const size_t orow = (size_t)w * 3 / 2;

    for (int y = 0; y < h; y++) {
        const uint8_t *restrict yr = Y + (size_t)y * w;
        /* even rows carry V, odd rows carry U (flip via UYY_VYY_SWAP_PARITY) */
        int use_u = (y & 1);
#if UYY_VYY_SWAP_PARITY
        use_u = !use_u;
#endif
        const uint8_t *restrict cr =
            (use_u ? U : V) + (size_t)(y / 2) * cw;
        uint8_t *restrict o = dst + (size_t)y * orow;
        for (int i = 0; i < cw; i++) {
            *o++ = cr[i];
            *o++ = yr[2 * i];
            *o++ = yr[2 * i + 1];
        }
    }
}

/* RGB565 -> O_UYY_E_VYY in ONE pass, writing straight into the encoder buffer.
 * The two-step route (RGB->I420, then I420->packed) made three full passes over
 * PSRAM per frame (~3.3 MB of traffic) and dominated the frame time. This does
 * 1 MB in, 768 KB out, and nothing else. */
void rgb565_to_uyy_vyy(const uint8_t *restrict src, uint8_t *restrict dst,
                       int sw, int sh, bool byteswap, int ds)
{
    const uint16_t *restrict s = (const uint16_t *restrict)src;
    if (ds < 1) ds = 1;
    const int w = sw / ds, h = sh / ds;      /* encoder-side dimensions */
    const size_t orow = (size_t)w * 3 / 2;

    for (int y = 0; y < h; y++) {
        /* point-sample: take every ds-th row and every ds-th pixel. Skipping
         * rows outright is what saves read bandwidth, which is the limiter. */
        const uint16_t *restrict sr = s + (size_t)(y * ds) * sw;
        uint8_t *restrict o = dst + (size_t)y * orow;
        /* even rows carry V, odd rows carry U (same parity as i420_to_uyy_vyy) */
        int want_u = (y & 1);
#if UYY_VYY_SWAP_PARITY
        want_u = !want_u;
#endif
        for (int i = 0; i < w / 2; i++) {
            uint16_t p0 = sr[(2 * i) * ds], p1 = sr[(2 * i + 1) * ds];
            if (byteswap) { p0 = SWAP16(p0); p1 = SWAP16(p1); }

            int r0 = WR(p0), g0 = WG(p0), b0 = WB(p0);
            int r1 = WR(p1), g1 = WG(p1), b1 = WB(p1);

            /* chroma point-sampled from the first pixel of the pair */
            int c = want_u
                  ? clamp8(((-43 * r0 - 85 * g0 + 128 * b0) >> 8) + 128)
                  : clamp8((( 128 * r0 - 107 * g0 - 21 * b0) >> 8) + 128);

            *o++ = (uint8_t)c;
            *o++ = (uint8_t)((77 * r0 + 150 * g0 + 29 * b0) >> 8);
            *o++ = (uint8_t)((77 * r1 + 150 * g1 + 29 * b1) >> 8);
        }
    }
}