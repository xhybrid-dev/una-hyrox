#!/usr/bin/env python3
"""Draw Knitting' launcher icons: two knitting needles through a ball of yarn.

Lime needles crossed over a white yarn ball on a dark grey disc. Writes
icon_60x60.png and icon_30x30.png next to this script.

The watch build's app_merging.py requires exactly those sizes, square, RGBA,
and converts them to ABGR2222, so the icon is drawn the way it will be shown:
flat colours from the display's 64, hard edges, a transparent background.
Drawn at 4x, reduced with a box filter, then snapped to the display's four
levels per channel. (Same method as X Weigh-in's make_icons.py.)

    python3 make_icons.py        (needs Pillow)
"""

import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))

LIME = (170, 255, 0, 255)          # SDK::GUI::Color::LIME
WHITE = (255, 255, 255, 255)
GRAY_DARK = (85, 85, 85, 255)
GRAY = (170, 170, 170, 255)


def draw(size):
    s = size * 4
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def p(x, y):
        return (x * s / 60.0, y * s / 60.0)

    w = int(round(3 * s / 60.0))
    d.ellipse([p(2, 2), p(58, 58)], fill=GRAY_DARK)
    # The yarn ball, with two strands wound round it.
    d.ellipse([p(14, 22), p(42, 50)], fill=WHITE)
    d.arc([p(14, 22), p(42, 50)], 200, 320, fill=GRAY, width=w)
    d.arc([p(18, 26), p(38, 46)], 20, 140, fill=GRAY, width=w)
    # Two needles crossing behind the top of the ball, tips as small knobs.
    d.line([p(14, 10), p(46, 38)], fill=LIME, width=w)
    d.line([p(46, 10), p(26, 30)], fill=LIME, width=w)
    for x, y in ((14, 10), (46, 10)):
        d.ellipse([p(x - 3, y - 3), p(x + 3, y + 3)], fill=LIME)

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
