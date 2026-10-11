#!/usr/bin/env python3
"""Draw HybridX Run's launcher icons: a gauge reading high.

A grey dial arc whose upper end is lime, with a white needle pointing into
the lime: an engine's capacity, which is what VO2max measures. Writes
icon_60x60.png and icon_30x30.png next to this script.

The watch build's app_merging.py requires exactly those sizes, square, RGBA,
and converts them to ABGR2222 (two bits per channel, alpha included), so the
icon is drawn the way it will be shown: flat colours from the display's 64,
hard edges, and a transparent background like the SDK's own app icons.
Drawn at 4x and reduced with a box filter, then snapped to the display's four
levels per channel so what you see here is what the watch shows. (Same
method as HybridX Intervals' make_icons.py.)

    python3 make_icons.py        (needs Pillow)
"""

import math
import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))

# SDK::GUI::Color values (SDK/GUI/Color.hpp), as the simulator shows them.
LIME = (170, 255, 0, 255)          # LIME 0x80C000: the high end
GRAY = (170, 170, 170, 255)        # GRAY 0x808080: the rest of the dial
WHITE = (255, 255, 255, 255)       # the needle
GRAY_DARK = (85, 85, 85, 255)      # GRAY_DARK 0x404040: the disc


def draw(size):
    s = size * 4
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def p(x, y):
        return (x * s / 60.0, y * s / 60.0)

    d.ellipse([p(2, 2), p(58, 58)], fill=GRAY_DARK)
    # Dial: PIL angles run clockwise from 3 o'clock. 150..390 is a 240 degree
    # arc open at the bottom; the last third is lime.
    box = [p(12, 12), p(48, 48)]
    width = int(round(6 * s / 60.0))
    d.arc(box, 150, 310, fill=GRAY, width=width)
    d.arc(box, 310, 390, fill=LIME, width=width)
    # Needle from the centre towards 340 degrees (into the lime).
    a = math.radians(340)
    cx, cy = 30, 30
    tip = (cx + 15 * math.cos(a), cy + 15 * math.sin(a))
    d.line([p(cx, cy), p(*tip)], fill=WHITE, width=int(round(4 * s / 60.0)))
    d.ellipse([p(cx - 4, cy - 4), p(cx + 4, cy + 4)], fill=WHITE)

    img = img.resize((size, size), Image.BOX)
    px = img.load()
    for y in range(size):
        for x in range(size):
            r, g, b, a = px[x, y]
            px[x, y] = tuple((c >> 6) * 85 for c in (r, g, b, a))
    return img


for n in (60, 30):
    out = os.path.join(HERE, "icon_%dx%d.png" % (n, n))
    draw(n).save(out)
    print("wrote", out)
