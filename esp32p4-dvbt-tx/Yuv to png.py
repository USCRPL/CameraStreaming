#!/usr/bin/env python3
"""
yuv_to_png.py - reconstruct a captured frame from an ESP32-P4 serial log.

    python yuv_to_png.py serial_log.txt frame

The dump block carries a fmt tag (rgb565 / i420). For rgb565 it writes frame.png
directly. For YUV it writes a luma image plus I420/NV12/NV21 color candidates so
you can identify the real chroma layout.

Requires: pip install numpy pillow
Capture the log with a raw serial terminal, e.g.:
    python -m serial.tools.miniterm COM5 115200 > serial_log.txt
"""
import sys, re, base64
import numpy as np
from PIL import Image

def yuv_to_rgb(Y, U, V):
    U = np.repeat(np.repeat(U, 2, 0), 2, 1).astype(np.float32)
    V = np.repeat(np.repeat(V, 2, 0), 2, 1).astype(np.float32)
    h, w = Y.shape
    U = U[:h, :w]; V = V[:h, :w]
    R = Y + 1.402 * (V - 128)
    G = Y - 0.344136 * (U - 128) - 0.714136 * (V - 128)
    B = Y + 1.772 * (U - 128)
    return np.clip(np.stack([R, G, B], -1), 0, 255).astype(np.uint8)

def main():
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(1)
    log = open(sys.argv[1], "r", errors="ignore").read()
    stem = sys.argv[2] if len(sys.argv) > 2 else "frame"

    m = re.search(
        r"---FRAME_DUMP_BEGIN w=(\d+) h=(\d+) len=(\d+) fmt=(\w+)---(.*?)---FRAME_DUMP_END---",
        log, re.S)
    if not m:
        print("No frame dump block found"); sys.exit(2)

    w, h, n, fmt = int(m.group(1)), int(m.group(2)), int(m.group(3)), m.group(4)
    b64 = re.sub(r"[^A-Za-z0-9+/=]", "", m.group(5))
    d = np.frombuffer(base64.b64decode(b64)[:n], np.uint8)
    print(f"{w}x{h} fmt={fmt}, {len(d)} bytes (expected {n})")

    if fmt == "rgb565":
        px = d[:w * h * 2].view(np.uint16).reshape(h, w)   # little-endian
        R = ((px >> 11) & 0x1F).astype(np.uint16) * 255 // 31
        G = ((px >> 5)  & 0x3F).astype(np.uint16) * 255 // 63
        B = (px         & 0x1F).astype(np.uint16) * 255 // 31
        Image.fromarray(np.stack([R, G, B], -1).astype(np.uint8)).save(stem + ".png")
        pxs = px.byteswap()
        Rs = ((pxs >> 11) & 0x1F).astype(np.uint16) * 255 // 31
        Gs = ((pxs >> 5)  & 0x3F).astype(np.uint16) * 255 // 63
        Bs = (pxs         & 0x1F).astype(np.uint16) * 255 // 31
        Image.fromarray(np.stack([Rs, Gs, Bs], -1).astype(np.uint8)).save(stem + "_swapped.png")
        print(f"wrote {stem}.png (and {stem}_swapped.png for big-endian)")
        return

    ysize = w * h
    Y = d[:ysize].reshape(h, w).astype(np.float32)
    Image.fromarray(Y.astype(np.uint8)).save(stem + "_luma.png")
    cw, ch = w // 2, h // 2
    csize = cw * ch
    chroma = d[ysize:]
    if len(chroma) >= 2 * csize:
        U = chroma[:csize].reshape(ch, cw).astype(np.float32)
        V = chroma[csize:2 * csize].reshape(ch, cw).astype(np.float32)
        Image.fromarray(yuv_to_rgb(Y, U, V)).save(stem + "_i420.png")
        uv = chroma[:2 * csize].reshape(ch, cw, 2).astype(np.float32)
        Image.fromarray(yuv_to_rgb(Y, uv[:, :, 0], uv[:, :, 1])).save(stem + "_nv12.png")
        Image.fromarray(yuv_to_rgb(Y, uv[:, :, 1], uv[:, :, 0])).save(stem + "_nv21.png")
    print(f"wrote {stem}_luma.png + I420/NV12/NV21 color candidates")

if __name__ == "__main__":
    main()