#!/usr/bin/env python3
"""Draw HybridX Intervals' launcher icons: a workout profile in blocks.

Low grey warm-up, three tall lime work blocks with short grey rests between,
and a low grey cool-down: the shape of a structured workout as a coach draws
it. Writes icon_60x60.png and icon_30x30.png next to this script.

The watch build's app_merging.py requires exactly those sizes, square, RGBA,
and converts them to ABGR2222 (two bits per channel, alpha included), so the
icon is drawn the way it will be shown: flat colours from the display's 64,
hard edges, and a transparent background like the SDK's own app icons.
Drawn at 4x and reduced with a box filter, then snapped to the display's four
levels per channel so what you see here is what the watch shows.

    python3 make_icons.py        (needs Pillow)
"""

import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))

# SDK::GUI::Color values (SDK/GUI/Color.hpp), as the simulator shows them.
LIME = (170, 255, 0, 255)          # LIME 0x80C000: work
GRAY = (170, 170, 170, 255)        # GRAY 0x808080: warm-up, rest, cool-down
GRAY_DARK = (85, 85, 85, 255)      # GRAY_DARK 0x404040: the disc


def draw(size):
    s = size * 4
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def p(x, y):
        return (x * s / 60.0, y * s / 60.0)

    d.ellipse([p(2, 2), p(58, 58)], fill=GRAY_DARK)
    base = 42
    # (x0, x1, top, colour): warm-up, work, rest, work, rest, work, cool-down.
    blocks = [
        (10, 15, 34, GRAY),
        (16, 21, 16, LIME),
        (22, 25, 34, GRAY),
        (26, 31, 16, LIME),
        (32, 35, 34, GRAY),
        (36, 41, 16, LIME),
        (42, 50, 34, GRAY),
    ]
    for x0, x1, top, colour in blocks:
        d.rectangle([p(x0, top), p(x1, base)], fill=colour)

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
