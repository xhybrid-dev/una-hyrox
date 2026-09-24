#!/usr/bin/env python3
"""Draw HybridX Streak's launcher icons: a mountain with a lime summit flag.

Writes icon_60x60.png and icon_30x30.png next to this script. The watch build's
app_merging.py requires exactly those sizes, square, RGBA, and converts them to
ABGR2222 (two bits per channel, alpha included), so the icon is drawn the way
it will be shown: flat colours from the display's 64, hard edges, and a
transparent background like the SDK's own app icons.

Drawn at 4x and reduced with a box filter, then snapped to the display's four
levels per channel so what you see here is what the watch shows.

    python3 make_icons.py        (needs Pillow)
"""

import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))

# SDK::GUI::Color values (SDK/GUI/Color.hpp), as the simulator shows them.
WHITE = (255, 255, 255, 255)   # WHITE 0xC0C0C0
GRAY = (170, 170, 170, 255)    # GRAY  0x808080
TEAL = (0, 170, 170, 255)      # TEAL  0x008080
TEAL_DARK = (0, 85, 85, 255)   # TEAL_DARK 0x004040
LIME = (170, 255, 0, 255)      # LIME  0x80C000
GRAY_DARK = (85, 85, 85, 255)  # GRAY_DARK 0x404040


def draw(size):
    s = size * 4
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def p(x, y):
        return (x * s / 60.0, y * s / 60.0)

    # A back peak, then the main peak lit on the left and shaded on the right,
    # then its snow: the home screen's mountain in miniature.
    d.polygon([p(40, 26), p(20, 56), p(59, 56)], fill=GRAY_DARK)
    d.polygon([p(26, 14), p(3, 56), p(26, 56)], fill=TEAL)
    d.polygon([p(26, 14), p(26, 56), p(49, 56)], fill=TEAL_DARK)
    d.polygon([p(26, 14), p(19.5, 26.5), p(26, 29)], fill=WHITE)
    d.polygon([p(26, 14), p(26, 29), p(32.5, 26.5)], fill=GRAY)
    # The summit flag.
    d.rectangle([p(25, 2), p(27.4, 15)], fill=WHITE)
    d.polygon([p(27.4, 2), p(42, 6.5), p(27.4, 11)], fill=LIME)

    img = img.resize((size, size), Image.BOX)
    # Snap to the display's levels, alpha included (ABGR2222 keeps 2 bits).
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
