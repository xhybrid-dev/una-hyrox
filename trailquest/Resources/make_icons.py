#!/usr/bin/env python3
"""Draw Trail Quest's launcher icons: a mountain with a snow cap and a sun.

A green mountain with a white snow cap on a dark grey disc, a yellow sun at
the top right. Writes icon_60x60.png and icon_30x30.png next to this script.

The watch build's app_merging.py requires exactly those sizes, square, RGBA,
and converts them to ABGR2222 (two bits per channel, alpha included), so the
icon is drawn the way it will be shown: flat colours from the display's 64,
hard edges, and a transparent background like the SDK's own app icons.
Drawn at 4x and reduced with a box filter, then snapped to the display's four
levels per channel so what you see here is what the watch shows. (Same
method as X Weigh-in's and HybridX Run's make_icons.py.)

    python3 make_icons.py        (needs Pillow)
"""

import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))

GRAY_DARK = (85, 85, 85, 255)      # the disc
GREEN = (0, 170, 0, 255)           # the mountain
GREEN_DARK = (0, 85, 0, 255)       # its shaded side
WHITE = (255, 255, 255, 255)       # the snow cap
YELLOW = (255, 255, 0, 255)        # the sun


def draw(size):
    s = size * 4
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def p(x, y):
        return (x * s / 60.0, y * s / 60.0)

    d.ellipse([p(2, 2), p(58, 58)], fill=GRAY_DARK)
    d.ellipse([p(37, 11), p(47, 21)], fill=YELLOW)                       # sun
    d.polygon([p(8, 46), p(26, 16), p(44, 46)], fill=GREEN)              # mountain
    d.polygon([p(26, 16), p(44, 46), p(31, 46)], fill=GREEN_DARK)        # shaded side
    d.polygon([p(26, 16), p(20.5, 25.5), p(24, 23), p(26, 27), p(29, 23), p(32, 25.5)], fill=WHITE)  # snow cap
    d.rectangle([p(8, 46), p(44, 48)], fill=GREEN_DARK)                  # foot

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
