#!/usr/bin/env python3
"""Option 4 (stitches on a needle), redrawn to show on the watch's launcher.

Jon's photo of the launcher (21 October 2026) shows that it draws an icon in shades of
blue by brightness: a transparent pixel is the launcher's blue, a bright pixel is a light
blue, a dark one a darker blue. So the live icon's teal ball and dark grey disc fall almost
exactly on the background blue and vanish, and its thin lime needles are lost against the
ball. What works there is the Timer's and the scale's way: bright shapes, thick, on a dark
base or on nothing, with no mid-tones.

So these use only bright colours (white, lime, yellow) on black, strokes of 4 units or more
(2 px at 30 px), and a black edge around every shape so overlaps separate. The sheet shows
each icon in its real colours and as the launcher is simulated to show it. The simulation is
a rough fit to the photo (black to dark blue, white to light blue, nothing to the launcher
blue), not a measurement.

    python3 make_needle_icons.py

writes needle-<variant>_60x60.png / _30x30.png and launcher-sheet.png.
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import make_icon_options as base           # the drawing helpers and the older options
from make_icon_options import Canvas, curve, LIME, WHITE, BLACK, poppins

YELLOW = (255, 255, 0, 255)

# The launcher as the photo shows it, roughly.
LAUNCHER_BG = (45, 58, 180)
LAUNCHER_DARK = (12, 16, 96)            # a black icon pixel
LAUNCHER_LIGHT = (118, 160, 255)        # a white one


def simulate(img):
    """@p img as the launcher shows it: one blue, lighter where the icon is brighter."""
    out = Image.new("RGB", img.size, LAUNCHER_BG)
    px, op = img.load(), out.load()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            lum = (0.3 * r + 0.59 * g + 0.11 * b) / 255.0
            shade = [LAUNCHER_DARK[i] + (LAUNCHER_LIGHT[i] - LAUNCHER_DARK[i]) * lum for i in range(3)]
            t = a / 255.0
            op[x, y] = tuple(int(LAUNCHER_BG[i] * (1 - t) + shade[i] * t) for i in range(3))
    return out


A0, B0 = (5, 55), (53, 7)               # the needle: tip to knob, before any shrinking


def needle_scene(c, disc, rim, sc=1.0):
    # Shrink the scene about the middle so it sits inside a disc (the strokes keep their weight).
    A = (30 + (A0[0] - 30) * sc, 30 + (A0[1] - 30) * sc)
    B = (30 + (B0[0] - 30) * sc, 30 + (B0[1] - 30) * sc)
    if disc:
        c.disc(30, 30, 29.5, BLACK)
    if rim:
        c.d.ellipse([c.p(0.5, 0.5), c.p(59.5, 59.5)], outline=WHITE, width=c.w(2.4))
    dx, dy = B[0] - A[0], B[1] - A[1]
    n = math.hypot(dx, dy)
    ux, uy = dx / n, dy / n
    vx, vy = -uy, ux
    edge = BLACK
    ACROSS, ALONG = 12.0 * sc ** 0.5, 3.9             # a stitch: long across the needle, narrow along it

    def loop_points(t, half=None):
        cx, cy = A[0] + dx * t, A[1] + dy * t
        pts = []
        a0, a1 = {None: (0, 2 * math.pi), "back": (math.pi, 2 * math.pi), "front": (0, math.pi)}[half]
        for j in range(25):
            th = a0 + (a1 - a0) * j / 24
            pts.append((cx + ACROSS * math.cos(th) * vx + ALONG * math.sin(th) * ux,
                        cy + ACROSS * math.cos(th) * vy + ALONG * math.sin(th) * uy))
        return pts

    def stroke(pts, width, colour):
        c.polyline(pts, width + 2.6, edge)
        c.polyline(pts, width, colour)

    ts = (0.22, 0.5, 0.78)
    colours = (WHITE, YELLOW, WHITE)       # the middle stitch is the one you are on
    for t, col in zip(ts, colours):
        stroke(loop_points(t, "back"), 3.6, col)
    c.line(A, B, 7.0, edge)
    c.line(A, B, 4.6, LIME)
    c.disc(B[0], B[1], 6.8, edge)
    c.disc(B[0], B[1], 5.2, LIME)
    for t, col in zip(ts, colours):
        stroke(loop_points(t, "front"), 3.6, col)


VARIANTS = [("A-black-disc", "4A  Black disc", lambda c: needle_scene(c, True, False, 0.8)),
            ("B-no-disc", "4B  No disc", lambda c: needle_scene(c, False, False, 1.0)),
            ("C-ringed", "4C  Black disc, white rim", lambda c: needle_scene(c, True, True, 0.66))]


def render(fn, size):
    c = Canvas(size)
    fn(c)
    return c.done()


def fit(img, scale):
    return img.resize((img.width * scale, img.height * scale), Image.NEAREST)


def main():
    live = Image.open(os.path.join(HERE, "..", "icon_60x60.png")).convert("RGBA")
    live30 = Image.open(os.path.join(HERE, "..", "icon_30x30.png")).convert("RGBA")
    first60 = Image.open(os.path.join(HERE, "option-4-needle-loops_60x60.png")).convert("RGBA")
    first30 = Image.open(os.path.join(HERE, "option-4-needle-loops_30x30.png")).convert("RGBA")
    entries = [("The live icon", live, live30), ("Option 4 as first drawn", first60, first30)]
    for slug, title, fn in VARIANTS:
        i60, i30 = render(fn, 60), render(fn, 30)
        i60.save(os.path.join(HERE, "needle-%s_60x60.png" % slug))
        i30.save(os.path.join(HERE, "needle-%s_30x30.png" % slug))
        entries.append((title, i60, i30))

    cw, ch = 270, 700
    sheet = Image.new("RGB", (cw * len(entries), ch), (24, 26, 28))
    d = ImageDraw.Draw(sheet)
    small, label = poppins(17), ImageFont.load_default()
    for i, (title, i60, i30) in enumerate(entries):
        x = i * cw
        d.text((x + 12, 8), title, font=small, fill=(235, 235, 235))
        d.text((x + 12, 36), "real colours, on black", font=label, fill=(150, 150, 150))
        black = Image.new("RGBA", i60.size, (0, 0, 0, 255)); black.alpha_composite(i60)
        sheet.paste(fit(black.convert("RGB"), 4), (x + 15, 50))
        d.text((x + 12, 300), "as the launcher shows it (simulated)", font=label, fill=(150, 150, 150))
        sheet.paste(fit(simulate(i60), 4), (x + 15, 314))
        d.text((x + 12, 560), "30 px and 60 px, simulated", font=label, fill=(150, 150, 150))
        sheet.paste(fit(simulate(i30), 3), (x + 15, 576))
        sheet.paste(fit(simulate(i60), 1), (x + 140, 590))
    sheet.save(os.path.join(HERE, "launcher-sheet.png"))
    print("wrote", os.path.join(HERE, "launcher-sheet.png"))


if __name__ == "__main__":
    main()
