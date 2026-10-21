#!/usr/bin/env python3
"""Draw Knitting's launcher icons: a ball of yarn with two knitting needles crossed above it.

A white ball with three black bands of yarn, two lime needles crossing in front of its top,
on a black disc. Writes icon_60x60.png and icon_30x30.png next to this script.

The watch build's app_merging.py requires exactly those sizes, square, RGBA, and converts
them to ABGR2222, so the icon is drawn the way it will be shown: flat colours from the
display's 64, hard edges, a transparent background. Drawn at 4x, reduced with a box filter,
then snapped to the display's four levels per channel. (Same method as X Weigh-in's
make_icons.py.)

The colours are chosen for the watch's launcher, which draws an icon in shades of blue by
brightness over its own blue (Jon's photo, 21 October 2026; docs/NOTES.md "Icon options"):
only bright colours and black, thick strokes, and a black edge round every shape, because a
mid-tone (grey, teal) lands on the launcher's blue and vanishes. icon-options/ has the other
designs that were considered; this is its "ball-A-black-disc".

    python3 make_icons.py        (needs Pillow)
"""

import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))

LIME = (170, 255, 0, 255)          # SDK::GUI::Color::LIME
WHITE = (255, 255, 255, 255)
BLACK = (0, 0, 0, 255)


def draw(size):
    s = size * 4
    k = s / 60.0
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def p(x, y):
        return (x * k, y * k)

    def w(width):
        return max(1, int(round(width * k)))

    def disc(cx, cy, r, fill, on=d):
        on.ellipse([p(cx - r, cy - r), p(cx + r, cy + r)], fill=fill)

    def line(a, b, width, fill, on=d):
        on.line([p(*a), p(*b)], fill=fill, width=w(width))
        disc(a[0], a[1], width / 2.0, fill, on)
        disc(b[0], b[1], width / 2.0, fill, on)

    def bezier(p0, p1, p2, n=24):
        return [((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0],
                 (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1])
                for t in (i / n for i in range(n + 1))]

    # The black disc: dark grey would sit on the launcher's blue.
    disc(30, 30, 29.5, BLACK)

    # The ball of yarn, white, edged in black, with three black bands of yarn clipped to it.
    cx, cy, r = 30, 39.5, 18
    disc(cx, cy, r + 2.0, BLACK)
    disc(cx, cy, r, WHITE)
    bands = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    bd = ImageDraw.Draw(bands)
    for o in (-7, 0, 7):
        pts = bezier((cx - 26 + o, cy + 26 + o), (cx + o + 5, cy + o + 5), (cx + 26 + o, cy - 26 + o))
        for a, b in zip(pts, pts[1:]):
            line(a, b, 2.8, BLACK, bd)
    clip = Image.new("L", (s, s), 0)
    disc(cx, cy, r - 0.5, 255, ImageDraw.Draw(clip))
    img.paste(bands, (0, 0), Image.composite(bands.split()[3], Image.new("L", (s, s), 0), clip))

    # Two needles crossing in front of the top of the ball, a black edge round each.
    n1 = ((10, 6), (37, 33))
    n2 = ((50, 6), (23, 33))
    for a, b in (n1, n2):
        line(a, b, 7.0, BLACK)
    for a, b in (n1, n2):
        line(a, b, 4.2, LIME)
    for x, y in ((10, 6), (50, 6)):
        disc(x, y, 6.6, BLACK)
    for x, y in ((10, 6), (50, 6)):
        disc(x, y, 5.0, LIME)

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
