#!/usr/bin/env python3
"""Draw HybridX Trail's launcher icons: a winding trail with a start dot.

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
AMBER = (255, 170, 0, 255)     # YELLOW_DARK-ish 0xC08000, the breadcrumb line
LIME = (170, 255, 0, 255)      # LIME  0x80C000, the start
GRAY_DARK = (85, 85, 85, 255)  # GRAY_DARK 0x404040


def draw(size):
    s = size * 4
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def p(x, y):
        return (x * s / 60.0, y * s / 60.0)

    # A dark disc, like the watch face the route is drawn on.
    d.ellipse([p(2, 2), p(58, 58)], fill=GRAY_DARK)
    # The route: an S winding up the face.
    trail = [p(16, 50), p(24, 44), p(38, 42), p(44, 34), p(34, 26), p(22, 24), p(20, 16), p(32, 10), p(44, 12)]
    d.line(trail, fill=AMBER, width=max(2, s // 12), joint="curve")
    # The runner: a white dot on the line, and the start in lime.
    d.ellipse([p(12, 46), p(20, 54)], fill=LIME)
    d.ellipse([p(40, 30), p(48, 38)], fill=WHITE)

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
