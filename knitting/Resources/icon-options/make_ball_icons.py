#!/usr/bin/env python3
"""The live icon (a ball of yarn with two needles), recoloured for the launcher.

Jon (21 October 2026): keep the original ball, improve the colours for the watch's screen.
The launcher draws an icon in shades of blue by brightness (see make_needle_icons.py), so the
live icon's dark grey disc, grey wraps and thin lime needles sit too close to the background
and to each other. Same picture, only changed for brightness:

  - a black disc (or none) instead of dark grey, so the bright parts stand out;
  - the ball white, with black wraps instead of grey, so the wraps read;
  - thicker needles, in lime, with a black edge where they cross the ball;
  - a black edge round the ball when there is no disc.

    python3 make_ball_icons.py

writes ball-<variant>_60x60.png / _30x30.png and ball-sheet.png.
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from make_icon_options import Canvas, curve, LIME, WHITE, BLACK, poppins
from make_needle_icons import simulate, fit, YELLOW


def clipped_to_disc(c, cx, cy, r, draw):
    """Run draw(canvas) on a spare layer and paste it only inside the disc, so strokes
    stop at the ball's edge."""
    layer = Canvas(c.size)
    draw(layer)
    mask = Image.new("L", c.img.size, 0)
    ImageDraw.Draw(mask).ellipse([c.p(cx - r, cy - r), c.p(cx + r, cy + r)], fill=255)
    c.img.paste(layer.img, (0, 0), Image.composite(layer.img.split()[3], Image.new("L", c.img.size, 0), mask))


def ball_scene(c, disc, ball=WHITE, needle=LIME):
    if disc:
        c.disc(30, 30, 29.5, BLACK)
    # The ball of yarn, white with a black edge, and the winding of the yarn as three
    # black diagonal bands across it (they read as yarn, where arcs read as a face).
    cx, cy, r = 30, 39.5, 18
    c.disc(cx, cy, r + 2.0, BLACK)
    c.disc(cx, cy, r, ball)

    def windings(layer):
        # Three evenly spaced bands, running up to the right and bowing a little
        # towards the bottom right, as yarn lies round a ball.
        for o in (-7, 0, 7):
            pts = curve((cx - 26 + o, cy + 26 + o), (cx + o + 5, cy + o + 5), (cx + 26 + o, cy - 26 + o))
            layer.polyline(pts, 2.8, BLACK)

    clipped_to_disc(c, cx, cy, r - 0.5, windings)
    # The two needles cross in front of the top of the ball, a black edge round each so
    # they stand off the ball and each other.
    n1 = ((10, 6), (37, 33))
    n2 = ((50, 6), (23, 33))
    for a, b in (n1, n2):
        c.line(a, b, 7.0, BLACK)
    for a, b in (n1, n2):
        c.line(a, b, 4.2, needle)
    for x, y in ((10, 6), (50, 6)):
        c.disc(x, y, 6.6, BLACK)
    for x, y in ((10, 6), (50, 6)):
        c.disc(x, y, 5.0, needle)


VARIANTS = [("A-black-disc", "A  Black disc", lambda c: ball_scene(c, True)),
            ("B-no-disc", "B  No disc", lambda c: ball_scene(c, False)),
            ("C-yellow-ball", "C  Yellow ball", lambda c: ball_scene(c, True, ball=YELLOW, needle=WHITE))]


def render(fn, size):
    c = Canvas(size)
    fn(c)
    return c.done()


def main():
    live = Image.open(os.path.join(HERE, "..", "icon_60x60.png")).convert("RGBA")
    live30 = Image.open(os.path.join(HERE, "..", "icon_30x30.png")).convert("RGBA")
    entries = [("The live icon", live, live30)]
    for slug, title, fn in VARIANTS:
        i60, i30 = render(fn, 60), render(fn, 30)
        i60.save(os.path.join(HERE, "ball-%s_60x60.png" % slug))
        i30.save(os.path.join(HERE, "ball-%s_30x30.png" % slug))
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
    sheet.save(os.path.join(HERE, "ball-sheet.png"))
    print("wrote", os.path.join(HERE, "ball-sheet.png"))


if __name__ == "__main__":
    main()
