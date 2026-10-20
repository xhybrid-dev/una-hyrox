#!/usr/bin/env python3
"""Draw X Sauna's launcher icons: steam rising off a heap of hot stones.

Three white steam curls over three orange stones on a dark grey disc. Writes
icon_60x60.png and icon_30x30.png next to this script. A placeholder until the
app has its own art (SA4).

The watch build's app_merging.py requires exactly those sizes, square, RGBA,
and converts them to ABGR2222, so the icon is drawn the way it will be shown:
flat colours from the display's 64, hard edges, a transparent background.
Drawn at 4x, reduced with a box filter, then snapped to the display's four
levels per channel. (Same method as Knitting's and X Weigh-in's make_icons.py.)

    python3 make_icons.py        (needs Pillow)
"""

import math
import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))

ORANGE = (255, 85, 0, 255)         # near SDK::GUI::Color::ORANGE_DARK
ORANGE_DARK = (170, 85, 0, 255)
WHITE = (255, 255, 255, 255)
GRAY_DARK = (85, 85, 85, 255)


def draw(size):
    s = size * 4
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def p(x, y):
        return (x * s / 60.0, y * s / 60.0)

    w = int(round(3 * s / 60.0))
    d.ellipse([p(2, 2), p(58, 58)], fill=GRAY_DARK)

    # Three stones, the middle one highest.
    d.ellipse([p(11, 40), p(29, 53)], fill=ORANGE_DARK)
    d.ellipse([p(31, 40), p(49, 53)], fill=ORANGE_DARK)
    d.ellipse([p(21, 35), p(39, 49)], fill=ORANGE)

    # Three steam curls, a sine wave each, rising from the stones.
    for x0, top in ((19, 12), (30, 8), (41, 12)):
        pts = []
        for i in range(0, 41):
            t = i / 40.0
            y = 33 - t * (33 - top)
            x = x0 + 3.2 * math.sin(t * 2.0 * math.pi * 1.25)
            pts.append(p(x, y))
        d.line(pts, fill=WHITE, width=w, joint="curve")
        for e in (pts[0], pts[-1]):
            r = w / 2.0
            d.ellipse([e[0] - r, e[1] - r, e[0] + r, e[1] + r], fill=WHITE)

    img = img.resize((size, size), Image.BOX)
    px = img.load()
    for y in range(size):
        for x in range(size):
            px[x, y] = tuple((c >> 6) * 85 for c in px[x, y])
    return img


for n in (60, 30):
    out = os.path.join(HERE, "icon_%dx%d.png" % (n, n))
    draw(n).save(out)
    print("wrote", out)
