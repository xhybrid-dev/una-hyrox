#!/usr/bin/env python3
"""The Energy Probe's app icons: a crescent moon (the nights) and a bolt (energy) on black,
in colours on the display's grid (each channel 0x00, 0x40, 0x80 or 0xC0).

    python3 x-energy/Tools/Probe/Resources/make_icons.py
"""
import os
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
SKY = (0x00, 0x80, 0xC0, 255)
AMBER = (0xC0, 0x80, 0x00, 255)


def icon(size):
    s = 8  # draw large, then shrink, for smooth edges
    n = size * s
    im = Image.new("RGBA", (n, n), (0, 0, 0, 255))
    d = ImageDraw.Draw(im)
    # The moon: a disc with a disc cut out of it.
    r = n * 0.36
    cx, cy = n * 0.44, n * 0.5
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=SKY)
    off = n * 0.17
    d.ellipse([cx - r + off, cy - r - off * 0.35, cx + r + off, cy + r - off * 0.35], fill=(0, 0, 0, 255))
    # The bolt, to the right.
    pts = [(0.66, 0.16), (0.50, 0.53), (0.62, 0.53), (0.54, 0.86), (0.82, 0.42), (0.69, 0.42), (0.78, 0.16)]
    d.polygon([(x * n, y * n) for x, y in pts], fill=AMBER)
    return im.resize((size, size), Image.LANCZOS)


for size in (60, 30):
    icon(size).save(os.path.join(HERE, f"icon_{size}x{size}.png"))
    print(f"icon_{size}x{size}.png")
