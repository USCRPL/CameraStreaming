/*
 * rgb_convert.h - RGB565 -> I420 (planar YUV420) color conversion.
 *
 * The ESP32-P4 ISP streams RGB565 reliably from the OV5647, but the H.264
 * encoder wants planar YUV420 (I420). This does the per-frame conversion
 * (BT.601), subsampling chroma 2x1/2x1 by point sampling each 2x2 block.
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* src: w*h*2 bytes RGB565.  dst: w*h*3/2 bytes I420 (Y plane, U plane, V plane).
 * byteswap=true reads the RGB565 as big-endian (V4L2_PIX_FMT_RGB565X); use it if
 * colors come out wrong (green/purple cast) with byteswap=false. w,h must be even. */
void rgb565_to_i420(const uint8_t *src, uint8_t *dst, int w, int h, bool byteswap);

/* Repack planar I420 into the ESP32-P4 encoder's NATIVE packed 4:2:0 layout,
 * O_UYY_E_VYY: each row is (w * 3 / 2) bytes of [C, Y, Y, C, Y, Y, ...], where C
 * is a V sample on even rows and a U sample on odd rows -- so each 2x2 block gets
 * one U and one V (proper 4:2:0). Total size is w*h*3/2, same as I420.
 *
 * Why: the V4L2 driver advertises YU12, but the hardware consumes THIS layout.
 * Feeding planar I420 makes it read 1200-byte rows from our 800-byte rows, which
 * repeats the picture horizontally and renders the U/V planes as image data.
 * Measured on-air: content ended at row 428 (= 512000/1200) with the remainder a
 * flat grey at luma 128 -- exactly our chroma planes shown as picture.
 *
 * If colours come out swapped (green<->magenta), flip UYY_VYY_SWAP_PARITY. */
#define UYY_VYY_SWAP_PARITY 0
void i420_to_uyy_vyy(const uint8_t *src, uint8_t *dst, int w, int h);

/* Fused RGB565 -> O_UYY_E_VYY (one pass, straight into the encoder's buffer).
 * Replaces rgb565_to_i420() + i420_to_uyy_vyy(): same result, but one pass over
 * PSRAM per frame instead of three. */
/* sw,sh = SOURCE (capture) dimensions. ds = downscale factor (1 = none, 2 = half).
 * Output is (sw/ds) x (sh/ds) in packed O_UYY_E_VYY. Capture must stay at a size
 * the ISP actually supports -- downscale here rather than asking for an
 * unsupported capture format, which makes S_FMT fail and hangs the pipeline. */
void rgb565_to_uyy_vyy(const uint8_t *src, uint8_t *dst, int sw, int sh,
                       bool byteswap, int ds);

#ifdef __cplusplus
}
#endif
