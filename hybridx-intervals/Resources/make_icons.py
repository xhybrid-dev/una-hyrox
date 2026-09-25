#!/usr/bin/env python3
"""Draw the Intervals Probe's launcher icons: a plain teal "P" on a dark tile.

Writes icon_60x60.png and icon_30x30.png next to this script. The watch build's
app_merging.py requires exactly those sizes, square, RGBA, and converts them to
ABGR2222 (two bits per channel, alpha included). This probe is throwaway (P0,
never released), so the icon is placeholder artwork only -- legible enough to
tell it apart on the watch's app list, nothing more.

Drawn at 4x and reduced with a box filter, then snapped to the display's four
levels per channel so what you see here is what the watch shows.

    python3 make_icons.py        (needs Pillow)
"""

import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))

# SDK::GUI::Color values (SDK/GUI/Color.hpp), as the simulator shows them.
TEAL = (0, 170, 170, 255)       # TEAL      0x008080
TEAL_DARK = (0, 85, 85, 255)    # TEAL_DARK 0x004040
WHITE = (255, 255, 255, 255)    # WHITE     0xC0C0C0


def draw(size):
    s = size * 4
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def p(x, y):
        return (x * s / 60.0, y * s / 60.0)

    # A rounded dark-teal tile, with a plain white "P" for "Probe".
    d.rounded_rectangle([p(4, 4), p(56, 56)], radius=s * 0.12, fill=TEAL_DARK, outline=TEAL, width=max(1, s // 30))
    d.rectangle([p(20, 14), p(26, 46)], fill=WHITE)
    d.rectangle([p(20, 14), p(38, 20)], fill=WHITE)
    d.rectangle([p(32, 14), p(38, 30)], fill=WHITE)
    d.rectangle([p(20, 24), p(38, 30)], fill=WHITE)

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
