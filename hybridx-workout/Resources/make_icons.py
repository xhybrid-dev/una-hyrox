#!/usr/bin/env python3
"""Draw HybridX Workout's launcher icons: a dumbbell.

A white bar with teal plates on a dark grey disc. Writes icon_60x60.png and
icon_30x30.png next to this script. A placeholder until the app has its own
art. (From X Sauna's make_icons.py.)

The watch build's app_merging.py requires exactly those sizes, square, RGBA,
and converts them to ABGR2222, so the icon is drawn the way it will be shown:
flat colours from the display's 64, hard edges, a transparent background.
Drawn at 4x, reduced with a box filter, then snapped to the display's four
levels per channel. (Same method as Knitting's and X Weigh-in's make_icons.py.)

    python3 make_icons.py        (needs Pillow)
"""

import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))

TEAL = (0, 170, 170, 255)          # near the menus' SDK::GUI::Color::TEAL
TEAL_DARK = (0, 85, 85, 255)
WHITE = (255, 255, 255, 255)
GRAY_DARK = (85, 85, 85, 255)


def draw(size):
    s = size * 4
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def p(x, y):
        return (x * s / 60.0, y * s / 60.0)

    d.ellipse([p(2, 2), p(58, 58)], fill=GRAY_DARK)

    # The bar, then two plates a side (the inner ones larger), tilted flat.
    d.rounded_rectangle([p(10, 28), p(50, 32)], radius=s / 60.0, fill=WHITE)
    for x0, x1, h in ((16, 22, 13), (11, 15, 9), (38, 44, 13), (45, 49, 9)):
        d.rounded_rectangle([p(x0, 30 - h), p(x1, 30 + h)], radius=1.5 * s / 60.0,
                            fill=TEAL if h == 13 else TEAL_DARK)

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
