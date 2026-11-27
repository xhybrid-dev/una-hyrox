#!/usr/bin/env python3
"""The Clock Probe's app icons: a ring of seven day segments (graded dark, amber, yellow, as the face
draws them) round a white X, on black, in colours on the display's grid (each channel 0x00, 0x40,
0x80 or 0xC0).

    python3 x-clock/Tools/Probe/Resources/make_icons.py
"""
import math
import os
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
BLACK = (0x00, 0x00, 0x00, 255)
NONE = (0x40, 0x40, 0x40, 255)
LOW = (0xC0, 0x80, 0x00, 255)
HIGH = (0xC0, 0xC0, 0x00, 255)
WHITE = (0xC0, 0xC0, 0xC0, 255)
GRADES = [HIGH, LOW, NONE, HIGH, LOW, NONE, NONE]


def icon(size):
    s = 8  # draw large, then shrink, for smooth edges
    n = size * s
    im = Image.new("RGBA", (n, n), BLACK)
    d = ImageDraw.Draw(im)
    c = n / 2
    outer, inner = n * 0.47, n * 0.35
    for i, col in enumerate(GRADES):
        a0 = -90 + i * 360 / 7 + 4
        a1 = -90 + (i + 1) * 360 / 7 - 4
        # a thick arc as a polygon between two radii
        pts = []
        steps = 24
        for k in range(steps + 1):
            a = math.radians(a0 + (a1 - a0) * k / steps)
            pts.append((c + outer * math.cos(a), c + outer * math.sin(a)))
        for k in range(steps, -1, -1):
            a = math.radians(a0 + (a1 - a0) * k / steps)
            pts.append((c + inner * math.cos(a), c + inner * math.sin(a)))
        d.polygon(pts, fill=col)
    # The X: two crossing blades.
    r, t = n * 0.20, n * 0.065
    for sign in (1, -1):
        d.polygon([(c - r, c - sign * r - t), (c - r, c - sign * r + t), (c + r, c + sign * r + t),
                   (c + r, c + sign * r - t)] if sign == 1 else
                  [(c - r, c + r - t), (c - r, c + r + t), (c + r, c - r + t), (c + r, c - r - t)], fill=WHITE)
    return im.resize((size, size), Image.LANCZOS)


for size in (60, 30):
    icon(size).save(os.path.join(HERE, f"icon_{size}x{size}.png"))
    print(f"icon_{size}x{size}.png")
