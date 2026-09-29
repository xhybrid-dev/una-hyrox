#!/usr/bin/env python3
"""Draw the Route Sender's launcher icon: Trail's winding route, for a phone.

The same picture as Trail's watch icon (hybridx-trail/Resources/make_icons.py),
drawn at phone size with full colour and smooth edges, since a phone screen has
none of the watch's four-levels-per-channel limit.

Writes app/res/mipmap-xxxhdpi/ic_launcher.png (192 x 192).

    python3 make_icon.py        (needs Pillow)
"""

import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "app", "res", "mipmap-xxxhdpi", "ic_launcher.png")

WHITE = (255, 255, 255, 255)
AMBER = (255, 170, 0, 255)
LIME = (170, 255, 0, 255)
GRAY_DARK = (60, 60, 60, 255)


def draw(size):
    s = size * 4
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def p(x, y):
        return (x * s / 60.0, y * s / 60.0)

    d.ellipse([p(2, 2), p(58, 58)], fill=GRAY_DARK)
    trail = [p(16, 50), p(24, 44), p(38, 42), p(44, 34), p(34, 26), p(22, 24), p(20, 16), p(32, 10), p(44, 12)]
    d.line(trail, fill=AMBER, width=max(2, s // 14), joint="curve")
    d.ellipse([p(12, 46), p(20, 54)], fill=LIME)
    d.ellipse([p(40, 30), p(48, 38)], fill=WHITE)
    return img.resize((size, size), Image.LANCZOS)


os.makedirs(os.path.dirname(OUT), exist_ok=True)
draw(192).save(OUT)
print("wrote", OUT)
