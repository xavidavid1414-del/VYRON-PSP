#!/usr/bin/env python3
"""Generate simple, reproducible XAVRX PSP icon/banner assets for CIA packaging."""
import struct
import wave
import zlib

def png(path, width, height, pixel):
    raw = b"".join(b"\x00" + b"".join(bytes(pixel(x, y)) for x in range(width)) for y in range(height))
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)
    data = b"\x89PNG\r\n\x1a\n"
    data += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    data += chunk(b"IDAT", zlib.compress(raw, 9))
    data += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(data)

def art(x, y, w, h):
    # Dark navy background with cyan frame and a stylized white V.
    edge = min(x, y, w - 1 - x, h - 1 - y)
    bg = (10, 18, 38, 255)
    if edge < max(2, min(w, h) // 18):
        return (20, 210, 220, 255)
    cx = (w - 1) / 2
    nx = abs(x - cx) / max(1, cx)
    # Two diagonal strokes form a bold V in the central region.
    vline = y > h * 0.22 and y < h * 0.82 and abs(nx - (0.18 + 0.58 * (y / h))) < 0.075
    vline2 = y > h * 0.22 and y < h * 0.82 and abs(nx - (0.76 - 0.58 * (y / h))) < 0.075
    if vline or vline2:
        return (245, 250, 255, 255)
    # Cyan center accent.
    if abs(x - cx) < max(2, w * 0.018) and h * 0.70 < y < h * 0.86:
        return (20, 210, 220, 255)
    return bg

png("assets/xavrx_icon.png", 48, 48, lambda x,y: art(x,y,48,48))
png("assets/xavrx_banner.png", 256, 128, lambda x,y: art(x,y,256,128))
with wave.open("assets/silence.wav", "wb") as wav:
    wav.setnchannels(1)
    wav.setsampwidth(2)
    wav.setframerate(22050)
    wav.writeframes(b"\x00\x00" * (22050 * 2))
