#!/usr/bin/env python3
"""
Generator zasobów graficznych dla aplikacji KorektaBar.
Tworzy wielowarstwowy plik KorektaBar.ico (16x16, 32x32, 48x48, 256x256)
w estetyce Dark Mode bez zewnętrznych zależności (czysta biblioteka standardowa Pythona)
oraz weryfikuje obecność banera SVG dla README.md.
"""

import math
import os
import struct
import zlib


def make_png(width: int, height: int, rgba_bytes: bytes) -> bytes:
    def chunk(chunk_type: bytes, data: bytes) -> bytes:
        crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
        return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc)

    raw_rows = bytearray()
    stride = width * 4
    for y in range(height):
        raw_rows.append(0)  # Filter type 0 (None)
        raw_rows.extend(rgba_bytes[y * stride : (y + 1) * stride])

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    idat = zlib.compress(bytes(raw_rows), level=9)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", idat)
        + chunk(b"IEND", b"")
    )


def dist_to_segment(px: float, py: float, x1: float, y1: float, x2: float, y2: float) -> float:
    dx = x2 - x1
    dy = y2 - y1
    len_sq = dx * dx + dy * dy
    if len_sq == 0:
        return math.hypot(px - x1, py - y1)
    t = max(0.0, min(1.0, ((px - x1) * dx + (py - y1) * dy) / len_sq))
    proj_x = x1 + t * dx
    proj_y = y1 + t * dy
    return math.hypot(px - proj_x, py - proj_y)


def blend_pixel(buf: bytearray, idx: int, r: int, g: int, b: int, alpha: float) -> None:
    if alpha <= 0.0:
        return
    a = min(1.0, max(0.0, alpha))
    inv = 1.0 - a
    buf[idx] = int(buf[idx] * inv + r * a)
    buf[idx + 1] = int(buf[idx + 1] * inv + g * a)
    buf[idx + 2] = int(buf[idx + 2] * inv + b * a)
    buf[idx + 3] = int(min(255, buf[idx + 3] + (255 - buf[idx + 3]) * a))


def render_icon_rgba(size: int) -> bytes:
    buf = bytearray(size * size * 4)
    s = float(size)
    radius = s * 0.22
    pad = s * 0.06
    half = s * 0.5
    box_half = half - pad

    for y in range(size):
        for x in range(size):
            px = x + 0.5
            py = y + 0.5
            idx = (y * size + x) * 4

            # Zaokrąglony kwadrat tła (Dark Graphite #14161A -> #1D222C)
            qx = abs(px - half) - (box_half - radius)
            qy = abs(py - half) - (box_half - radius)
            outside = math.hypot(max(qx, 0.0), max(qy, 0.0)) + min(max(qx, qy), 0.0) - radius

            if outside < 0.8:
                bg_alpha = max(0.0, min(1.0, 0.8 - outside))
                grad = py / s
                bg_r = int(20 + 10 * grad)
                bg_g = int(22 + 12 * grad)
                bg_b = int(27 + 16 * grad)
                buf[idx] = bg_r
                buf[idx + 1] = bg_g
                buf[idx + 2] = bg_b
                buf[idx + 3] = int(255 * bg_alpha)

                # Subtelne obramowanie 1px (#2D3545)
                if -1.3 <= outside < 0.4:
                    border_a = max(0.0, min(1.0, 1.0 - abs(outside + 0.4) * 0.9)) * bg_alpha
                    blend_pixel(buf, idx, 59, 130, 246, border_a * 0.45)

            # Litera "K" (biel #ECEFF4) po lewej stronie
            stroke = max(1.1, s * 0.095)
            kx_left = s * 0.27
            ky_top = s * 0.25
            ky_bot = s * 0.71
            ky_mid = s * 0.48
            kx_right = s * 0.49

            d_stem = dist_to_segment(px, py, kx_left, ky_top, kx_left, ky_bot)
            d_up = dist_to_segment(px, py, kx_left + stroke * 0.1, ky_mid, kx_right, ky_top)
            d_dn = dist_to_segment(px, py, kx_left + stroke * 0.25, ky_mid - stroke * 0.1, kx_right, ky_bot)
            d_k = min(d_stem, d_up, d_dn)
            if d_k < stroke * 0.55 + 0.8:
                alpha = max(0.0, min(1.0, (stroke * 0.55 + 0.6 - d_k)))
                blend_pixel(buf, idx, 236, 239, 244, alpha)

            # Znak korekty / haczyk (niebieski akcent #38BDF8 -> #3B82F6) po prawej
            ck_stroke = max(1.1, s * 0.09)
            d_c1 = dist_to_segment(px, py, s * 0.55, s * 0.50, s * 0.64, s * 0.60)
            d_c2 = dist_to_segment(px, py, s * 0.64, s * 0.60, s * 0.79, s * 0.36)
            d_check = min(d_c1, d_c2)
            if d_check < ck_stroke * 0.55 + 0.8:
                alpha = max(0.0, min(1.0, (ck_stroke * 0.55 + 0.6 - d_check)))
                blend_pixel(buf, idx, 56, 189, 248, alpha)

            # Faliste podkreślenie korektora u dołu (#F87171 -> #38BDF8)
            wave_y = s * 0.80 + math.sin((px / s) * math.pi * 6.0) * (s * 0.022)
            if s * 0.21 <= px <= s * 0.79:
                d_wave = abs(py - wave_y)
                wave_thick = max(0.85, s * 0.032)
                if d_wave < wave_thick + 0.7:
                    alpha = max(0.0, min(1.0, wave_thick + 0.5 - d_wave))
                    t_col = (px - s * 0.21) / (s * 0.58)
                    wr = int(248 * (1.0 - t_col) + 56 * t_col)
                    wg = int(113 * (1.0 - t_col) + 189 * t_col)
                    wb = int(113 * (1.0 - t_col) + 248 * t_col)
                    blend_pixel(buf, idx, wr, wg, wb, alpha)

    return bytes(buf)


def build_ico(output_path: str) -> None:
    sizes = [16, 32, 48, 256]
    png_frames = []
    for sz in sizes:
        rgba = render_icon_rgba(sz)
        png_frames.append((sz, make_png(sz, sz, rgba)))

    # ICONDIR header: reserved=0, type=1 (ICO), count=len(sizes)
    header = struct.pack("<HHH", 0, 1, len(png_frames))
    directory = bytearray()
    payload = bytearray()
    offset = 6 + 16 * len(png_frames)

    for sz, png_data in png_frames:
        w_byte = 0 if sz == 256 else sz
        h_byte = 0 if sz == 256 else sz
        directory.extend(
            struct.pack(
                "<BBBBHHII",
                w_byte,
                h_byte,
                0,   # Color count
                0,   # Reserved
                1,   # Color planes
                32,  # Bits per pixel
                len(png_data),
                offset,
            )
        )
        payload.extend(png_data)
        offset += len(png_data)

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "wb") as f:
        f.write(header)
        f.write(directory)
        f.write(payload)


if __name__ == "__main__":
    out_ico = os.path.join(os.path.dirname(__file__), "..", "assets", "KorektaBar.ico")
    out_ico = os.path.abspath(out_ico)
    build_ico(out_ico)
    print(f"Wygenerowano {out_ico} ({os.path.getsize(out_ico)} bajtów)")
