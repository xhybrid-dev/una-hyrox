#!/usr/bin/env python3
"""Draw X Weigh-in's launcher icons: a bathroom scale.

A white scale platform on a dark grey disc, with a dark dial window at the
top whose lime needle points just past centre. Writes icon_60x60.png and
icon_30x30.png next to this script.

The watch build's app_merging.py requires exactly those sizes, square, RGBA,
and converts them to ABGR2222 (two bits per channel, alpha included), so the
icon is drawn the way it will be shown: flat colours from the display's 64,
hard edges, and a transparent background like the SDK's own app icons.
Drawn at 4x and reduced with a box filter, then snapped to the display's four
levels per channel so what you see here is what the watch shows. (Same
method as HybridX Run's make_icons.py.)

    python3 make_icons.py        (needs Pillow)
"""

import math
import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))

# SDK::GUI::Color values (SDK/GUI/Color.hpp), as the simulator shows them.
LIME = (170, 255, 0, 255)          # LIME 0x80C000: the needle
WHITE = (255, 255, 255, 255)       # the platform
GRAY_DARK = (85, 85, 85, 255)      # GRAY_DARK 0x404040: the disc
BLACK = (0, 0, 0, 255)             # the dial window


def draw(size):
    s = size * 4
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def p(x, y):
        return (x * s / 60.0, y * s / 60.0)

    d.ellipse([p(2, 2), p(58, 58)], fill=GRAY_DARK)
    # The platform: a rounded square, seen from above.
    d.rounded_rectangle([p(14, 14), p(46, 46)], radius=6 * s / 60.0, fill=WHITE)
    # The dial window: the top half of a disc, flat side down.
    box = [p(21, 18), p(39, 36)]
    d.pieslice(box, 180, 360, fill=BLACK)
    # Needle from the dial's centre, a little right of upright.
    a = math.radians(-70)
    cx, cy = 30, 27
    tip = (cx + 7.5 * math.cos(a), cy + 7.5 * math.sin(a))
    d.line([p(cx, cy), p(*tip)], fill=LIME, width=int(round(3 * s / 60.0)))

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
