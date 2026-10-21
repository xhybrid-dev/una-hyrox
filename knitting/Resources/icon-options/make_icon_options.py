#!/usr/bin/env python3
"""Five launcher-icon options for Knitting, to choose from.

Each is drawn the way make_icons.py draws the live icon: 4x supersampled, reduced
with a box filter, then snapped to the display's four levels per channel, on a
transparent background, as 60x60 and 30x30 PNGs. Nothing here replaces the live
icon (../icon_60x60.png); once one is chosen, make_icons.py takes it over.

The launcher ring seems to draw an icon as a flat shape rather than in colour (X
Weigh-in's dark disc does not show there), so the contact sheet also shows each
option as a single-colour silhouette: an option should still read like that.

    python3 make_icon_options.py        (needs Pillow; Poppins from $UNA_SDK if set)

writes option-<n>-<name>_60x60.png / _30x30.png and options-sheet.png.
"""
import math
import os

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))

LIME = (170, 255, 0, 255)       # the app's accent (SDK LIME)
TEAL = (0, 170, 170, 255)
TEAL_DARK = (0, 85, 85, 255)
WHITE = (255, 255, 255, 255)
GREY = (170, 170, 170, 255)
GREY_DARK = (85, 85, 85, 255)
AMBER = (255, 170, 0, 255)
BLACK = (0, 0, 0, 255)


def poppins(size):
    sdk = os.environ.get("UNA_SDK", "")
    for path in (os.path.join(sdk, "Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/assets/fonts/Poppins-Bold.ttf"),
                 os.path.join(sdk, "Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/assets/fonts/Poppins-SemiBold.ttf"),
                 "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"):
        if os.path.isfile(path):
            return ImageFont.truetype(path, size)
    return ImageFont.load_default()


class Canvas:
    """A 60x60 design grid drawn at 4x."""

    def __init__(self, size):
        self.size = size
        self.s = size * 4
        self.k = self.s / 60.0
        self.img = Image.new("RGBA", (self.s, self.s), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.img)

    def p(self, x, y):
        return (x * self.k, y * self.k)

    def pts(self, pts):
        return [self.p(x, y) for x, y in pts]

    def w(self, width):
        return max(1, int(round(width * self.k)))

    def disc(self, cx, cy, r, fill):
        self.d.ellipse([self.p(cx - r, cy - r), self.p(cx + r, cy + r)], fill=fill)

    def line(self, a, b, width, fill, caps=True):
        self.d.line([self.p(*a), self.p(*b)], fill=fill, width=self.w(width))
        if caps:
            self.disc(a[0], a[1], width / 2.0, fill)
            self.disc(b[0], b[1], width / 2.0, fill)

    def poly(self, pts, fill):
        self.d.polygon(self.pts(pts), fill=fill)

    def polyline(self, pts, width, fill):
        for a, b in zip(pts, pts[1:]):
            self.line(a, b, width, fill)

    def rrect(self, box, radius, fill):
        (x0, y0), (x1, y1) = box
        self.d.rounded_rectangle([self.p(x0, y0), self.p(x1, y1)], radius=radius * self.k, fill=fill)

    def arc(self, box, a0, a1, width, fill):
        (x0, y0), (x1, y1) = box
        self.d.arc([self.p(x0, y0), self.p(x1, y1)], a0, a1, fill=fill, width=self.w(width))

    def done(self):
        img = self.img.resize((self.size, self.size), Image.BOX)
        px = img.load()
        for y in range(self.size):
            for x in range(self.size):
                px[x, y] = tuple((c >> 6) * 85 for c in px[x, y])
        return img


def curve(p0, p1, p2, n=24):
    """A quadratic Bezier as a list of points."""
    out = []
    for i in range(n + 1):
        t = i / n
        out.append(((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0],
                    (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]))
    return out


# -- 1. Yarn and needles -------------------------------------------------------

def yarn_and_needles(c):
    c.disc(27, 37, 18, TEAL)
    c.arc(((9, 21), (45, 53)), 205, 335, 3.0, TEAL_DARK)         # two clean wraps
    c.arc(((9, 33), (45, 49)), 15, 165, 3.0, TEAL_DARK)
    c.polyline(curve((42, 50), (53, 57), (56, 46)), 3.4, TEAL)     # the loose end
    # Two needles pushed in, crossing at the top of the ball.
    c.line((9, 5), (35, 31), 4.2, LIME)
    c.line((51, 5), (25, 31), 4.2, LIME)
    c.disc(9, 5, 4.6, LIME)
    c.disc(51, 5, 4.6, LIME)


# -- 2. A knit stitch ----------------------------------------------------------

def stitches(c):
    c.rrect(((4, 4), (56, 56)), 10, TEAL)
    for row in range(3):
        for col in range(3):
            cx, cy = 14 + 16 * col, 14 + 16 * row
            colour = LIME if (row, col) == (1, 1) else WHITE      # the stitch you are on
            c.polyline([(cx - 4.4, cy - 6), (cx, cy + 5), (cx + 4.4, cy - 6)], 3.2, colour)


# -- 3. A counter ---------------------------------------------------------------

def counter(c):
    c.disc(25, 36, 19, WHITE)
    c.arc(((6, 20), (44, 52)), 205, 335, 3.0, GREY)
    c.arc(((6, 32), (44, 48)), 15, 165, 3.0, GREY)
    c.polyline(curve((40, 51), (50, 58), (54, 48)), 3.2, WHITE)
    c.disc(44, 17, 14, BLACK)                                     # a dark ring lifts the badge off the ball
    c.disc(44, 17, 12, LIME)
    f = poppins(int(17 * c.k))
    text = "+1"
    box = c.d.textbbox((0, 0), text, font=f)
    c.d.text((44 * c.k - (box[2] + box[0]) / 2, 17 * c.k - (box[3] + box[1]) / 2), text, font=f, fill=BLACK)


# -- 4. Stitches on a needle ---------------------------------------------------

def needle_loops(c):
    a, b = (7, 53), (51, 9)                       # tip to knob
    dx, dy = b[0] - a[0], b[1] - a[1]
    n = math.hypot(dx, dy)
    ux, uy = dx / n, dy / n                       # along the needle
    vx, vy = -uy, ux                              # across it
    # The strand comes in from the left, then three loops sit on the needle.
    first = (a[0] + dx * 0.28, a[1] + dy * 0.28)
    c.polyline(curve((3, 33), (4, 50), (first[0] - vx * 10, first[1] - vy * 10)), 3.2, TEAL)
    for i, t in enumerate((0.28, 0.52, 0.76)):
        cx, cy = a[0] + dx * t, a[1] + dy * t
        pts = []
        for j in range(33):
            th = 2 * math.pi * j / 32
            pts.append((cx + 11 * math.cos(th) * vx + 4.2 * math.sin(th) * ux,
                        cy + 11 * math.cos(th) * vy + 4.2 * math.sin(th) * uy))
        c.polyline(pts, 3.2, WHITE if i % 2 == 0 else TEAL)
    c.line(a, b, 3.8, LIME)
    c.disc(b[0], b[1], 4.8, LIME)


# -- 5. A jumper ----------------------------------------------------------------

def jumper(c):
    body = [(19, 9), (26, 9), (30, 14), (34, 9), (41, 9), (57, 25), (49, 34), (42, 27),
            (42, 53), (18, 53), (18, 27), (11, 34), (3, 25)]
    c.poly(body, TEAL)
    c.polyline([(26, 9), (30, 14), (34, 9)], 3, WHITE)                  # the neck
    c.rrect(((18, 46), (42, 53)), 1.5, WHITE)                           # ribbed hem
    for x in (22, 26, 30, 34, 38):
        c.line((x, 47), (x, 52), 1.2, TEAL_DARK, caps=False)
    for y in (19, 23, 27, 31):                                          # a knitted band on the chest
        pass
    for i, x in enumerate((24, 30, 36)):
        c.polyline([(x - 3.5, 25), (x, 33), (x + 3.5, 25)], 2.8, LIME)
    c.line((5, 29), (12, 36), 3, WHITE)                                 # cuffs
    c.line((55, 29), (48, 36), 3, WHITE)


OPTIONS = [("yarn-and-needles", yarn_and_needles, "Yarn and needles"),
           ("stitches", stitches, "A knit stitch"),
           ("counter", counter, "A counter"),
           ("needle-loops", needle_loops, "Stitches on a needle"),
           ("jumper", jumper, "A jumper")]


def make(fn, size):
    c = Canvas(size)
    fn(c)
    return c.done()


def silhouette(img, colour=(60, 100, 255, 255)):
    """The icon as one flat colour, wherever it is not transparent."""
    out = Image.new("RGBA", img.size, (0, 0, 0, 0))
    px, op = img.load(), out.load()
    for y in range(img.height):
        for x in range(img.width):
            if px[x, y][3] > 0:
                op[x, y] = colour
    return out


def on(img, bg, scale):
    base = Image.new("RGBA", img.size, bg)
    base.alpha_composite(img)
    return base.resize((img.width * scale, img.height * scale), Image.NEAREST)


def main():
    label = ImageFont.load_default()
    cell_w, cell_h = 270, 640
    sheet = Image.new("RGB", (cell_w * len(OPTIONS), cell_h), (24, 26, 28))
    d = ImageDraw.Draw(sheet)
    for i, (slug, fn, title) in enumerate(OPTIONS, start=1):
        i60, i30 = make(fn, 60), make(fn, 30)
        i60.save(os.path.join(HERE, "option-%d-%s_60x60.png" % (i, slug)))
        i30.save(os.path.join(HERE, "option-%d-%s_30x30.png" % (i, slug)))
        x0 = (i - 1) * cell_w
        d.text((x0 + 14, 10), "%d  %s" % (i, title), font=poppins(20), fill=(235, 235, 235))
        sheet.paste(on(i60, (0, 0, 0, 255), 4).convert("RGB"), (x0 + 15, 46))          # 240 px, on black
        sheet.paste(on(silhouette(i60), (0, 0, 0, 255), 2).convert("RGB"), (x0 + 15, 300))   # as a flat shape
        sheet.paste(on(i60, (0, 0, 0, 255), 1).convert("RGB"), (x0 + 155, 305))        # actual size
        sheet.paste(on(i30, (0, 0, 0, 255), 1).convert("RGB"), (x0 + 155, 380))
        sheet.paste(on(i60, (255, 255, 255, 255), 1).convert("RGB"), (x0 + 155, 430))  # on white
        d.text((x0 + 15, 430), "flat shape", font=label, fill=(150, 150, 150))
        d.text((x0 + 155, 292), "60 px", font=label, fill=(150, 150, 150))
        d.text((x0 + 155, 367), "30 px", font=label, fill=(150, 150, 150))
    sheet.save(os.path.join(HERE, "options-sheet.png"))
    print("wrote", os.path.join(HERE, "options-sheet.png"))


if __name__ == "__main__":
    main()
