#!/usr/bin/env python3
"""Draw five launcher-icon options for Trail Quest (21 October 2026), to choose from.

Writes, into docs/icons/ next to the Resources folder:
  option-N-<name>-60.png   the real size, as it would ship (60x60, RGBA, a dark disc)
  option-N-<name>-30.png   the 30x30 version
  options-sheet.png        all five, large, and at the sizes the watch shows

Same method as make_icons.py: drawn at 4x, reduced with a box filter, then
snapped to the display's four levels per channel (0, 85, 170, 255) so what is
seen here is what the watch shows. The characters are the game's own sprites
(Tools/Art/gen_art.py), so the icon looks like the game.

Nothing here replaces icon_60x60.png / icon_30x30.png: that is make_icons.py's, and
changes only when one of these is chosen.

    python3 make_icon_options.py        (needs Pillow)
"""

import os
import sys

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "Tools", "Art"))
import gen_art  # noqa: E402  (its main() runs only as a script)

OUT = os.path.join(HERE, "..", "docs", "icons")

TRANSPARENT = (0, 0, 0, 0)
BLACK = (0, 0, 0, 255)
NAVY = (0, 0, 85, 255)
BLUE = (0, 85, 170, 255)
SKY = (85, 170, 255, 255)
GREEN = (0, 170, 0, 255)
GREEN_DARK = (0, 85, 0, 255)
GREY = (170, 170, 170, 255)
GREY_MID = (127, 127, 127, 255)
GREY_DARK = (85, 85, 85, 255)
WHITE = (255, 255, 255, 255)
YELLOW = (255, 255, 0, 255)
ORANGE = (255, 170, 0, 255)
RED = (255, 0, 0, 255)
RED_DARK = (170, 0, 0, 255)
BROWN = (170, 85, 0, 255)
TAN = (255, 170, 85, 255)
PURPLE = (170, 0, 170, 255)


def sprite_image(canvas, tint=None):
    """A gen_art Canvas as an RGBA image, one pixel per sprite pixel; tint makes a silhouette."""
    n = canvas.size
    img = Image.new("RGBA", (n, n), TRANSPARENT)
    px = img.load()
    for y in range(n):
        for x in range(n):
            idx = canvas.px[y][x]
            if idx == 0:
                continue
            if tint is not None:
                px[x, y] = tint
            else:
                v = gen_art.PALETTE[idx]
                px[x, y] = ((v >> 16) & 255, (v >> 8) & 255, v & 255, 255)
    return img


class Art:
    """A 60x60 icon drawn at 4x (240 px): coordinates are the icon's own."""

    def __init__(self):
        self.s = 240
        self.img = Image.new("RGBA", (self.s, self.s), TRANSPARENT)
        self.d = ImageDraw.Draw(self.img)

    def p(self, x, y):
        return (x * 4.0, y * 4.0)

    def disc(self, color):
        self.d.ellipse([self.p(2, 2), self.p(58, 58)], fill=color)

    def circle(self, cx, cy, r, color):
        self.d.ellipse([self.p(cx - r, cy - r), self.p(cx + r, cy + r)], fill=color)

    def poly(self, pts, color):
        self.d.polygon([self.p(x, y) for x, y in pts], fill=color)

    def rect(self, x0, y0, x1, y1, color):
        self.d.rectangle([self.p(x0, y0), self.p(x1, y1)], fill=color)

    def line(self, pts, color, width):
        self.d.line([self.p(x, y) for x, y in pts], fill=color, width=int(width * 4), joint="curve")

    def sprite(self, canvas, x, y, scale=1.0, tint=None, flip=False):
        """Paste a sprite with its top-left at icon (x, y); scale is icon pixels per sprite pixel."""
        im = sprite_image(canvas, tint)
        if flip:
            im = im.transpose(Image.FLIP_LEFT_RIGHT)
        w = int(round(im.width * scale * 4))
        im = im.resize((w, w), Image.NEAREST)
        self.img.alpha_composite(im, (int(round(x * 4)), int(round(y * 4))))

    def clip_to_disc(self):
        """Keep only the disc (so the scene never spills past the round edge)."""
        mask = Image.new("L", (self.s, self.s), 0)
        ImageDraw.Draw(mask).ellipse([self.p(2, 2), self.p(58, 58)], fill=255)
        out = Image.new("RGBA", (self.s, self.s), TRANSPARENT)
        out.paste(self.img, (0, 0), mask)
        self.img = out

    def finish(self, size):
        img = self.img.resize((size, size), Image.BOX)
        px = img.load()
        for y in range(size):
            for x in range(size):
                px[x, y] = tuple((c >> 6) * 85 for c in px[x, y])
        return img


# -- 1. Dawn on the ridge: the runner on the crest, the sun rising ---------------------

def option_dawn():
    a = Art()
    a.disc(NAVY)
    a.circle(38, 25, 12, ORANGE)                                          # the rising sun
    a.circle(38, 25, 7, YELLOW)
    a.poly([(2, 46), (17, 22), (31, 40), (41, 28), (58, 46), (58, 58), (2, 58)], GREY_DARK)   # far ridge
    a.poly([(17, 22), (13, 31), (16, 29), (19, 33), (22, 29), (24, 31)], WHITE)               # a snow cap
    a.poly([(2, 58), (2, 44), (20, 38), (36, 44), (58, 41), (58, 58)], GREEN_DARK)            # the near crest
    a.rect(2, 46, 58, 58, GREEN)
    a.sprite(gen_art.runner(0), 17, 20, 1.0)                                                  # the runner, on the crest
    a.clip_to_disc()
    return a


# -- 2. The duel: the runner meets the Red Fox -----------------------------------------

def option_duel():
    a = Art()
    a.disc(BLUE)
    a.rect(2, 44, 58, 58, GREEN)                                          # the grass
    a.rect(2, 52, 58, 58, GREEN_DARK)
    a.poly([(30, 8), (33, 17), (42, 17), (35, 23), (38, 32), (30, 26), (22, 32), (25, 23), (18, 17), (27, 17)], YELLOW)   # the clash
    a.poly([(30, 12), (32, 18), (38, 18), (33, 22), (35, 28), (30, 24), (25, 28), (27, 22), (22, 18), (28, 18)], WHITE)
    a.sprite(gen_art.runner(0), 4, 22, 1.0)
    a.sprite(gen_art.fox(0), 33, 22, 1.0)
    a.clip_to_disc()
    return a


# -- 3. The Summit Wolf: a silhouette against the moon ---------------------------------

def option_wolf():
    a = Art()
    a.disc(NAVY)
    a.circle(36, 24, 21, WHITE)                                           # the moon
    a.circle(46, 14, 3, GREY)                                             # craters
    a.circle(47, 30, 2, GREY)
    for sx, sy in ((9, 12), (16, 5), (50, 6), (10, 24)):                  # stars
        a.rect(sx, sy, sx + 1, sy + 1, YELLOW)
    a.sprite(gen_art.summit_wolf(0), 18, 17, 1.0, tint=BLACK)             # the wolf, in silhouette, on the moon
    a.poly([(2, 58), (2, 50), (12, 47), (24, 50), (36, 47), (48, 50), (58, 47), (58, 58)], BLACK)   # the ridge
    a.rect(25, 25, 26, 26, RED)                                           # its red eyes
    a.rect(28, 25, 29, 26, RED)
    a.clip_to_disc()
    return a


# -- 4. The needle: the game's signature strike, under a peak --------------------------

def option_needle():
    a = Art()
    a.disc(NAVY)
    a.poly([(8, 32), (24, 9), (33, 22), (41, 13), (52, 32)], GREY)       # a two-peaked range
    a.poly([(24, 9), (33, 22), (28, 32), (22, 32)], GREY_MID)
    a.poly([(41, 13), (52, 32), (36, 32)], GREY_MID)
    a.poly([(24, 9), (19, 17), (22, 16), (24, 20), (27, 16), (29, 17)], WHITE)     # snow caps
    a.poly([(41, 13), (37, 20), (40, 19), (41, 22), (44, 19), (46, 20)], WHITE)
    a.rect(24, 3, 25, 9, WHITE)                                           # a flag on the high peak
    a.poly([(25, 3), (32, 5), (25, 7)], RED)
    a.rect(11, 37, 49, 47, GREY_MID)                                      # the timing bar
    a.rect(22, 37, 42, 47, ORANGE)                                        # good
    a.rect(28, 37, 36, 47, GREEN)                                         # perfect
    a.rect(31, 34, 33, 50, WHITE)                                         # the needle, in the green
    a.clip_to_disc()
    return a


# -- 5. The trail map: a route, a fork, a hut, a spirit, the boss ----------------------

def option_map():
    a = Art()
    a.disc(GREEN_DARK)
    a.circle(30, 30, 24, GREEN)
    path = [(10, 46), (20, 40), (28, 44), (36, 36), (30, 28), (38, 20), (47, 16)]
    a.line(path, TAN, 4)                                                  # the trail
    a.line([(28, 44), (40, 49), (48, 40), (47, 16)], TAN, 3)              # the fork, rejoining
    a.rect(9, 36, 10, 47, WHITE)                                          # the start: a flag
    a.poly([(10, 36), (17, 39), (10, 42)], RED)
    a.poly([(30, 22), (35, 27), (25, 27)], RED_DARK)                      # a hut: roof and walls
    a.rect(26, 27, 34, 33, WHITE)
    a.circle(18, 39, 0, WHITE)
    a.circle(40, 49, 4, PURPLE)                                           # a spirit on the fork
    a.rect(39, 46, 40, 49, WHITE)
    a.rect(39, 50, 40, 51, WHITE)
    a.circle(47, 16, 6, RED)                                              # the boss
    a.rect(44, 14, 45, 15, BLACK)
    a.rect(48, 14, 49, 15, BLACK)
    a.rect(44, 19, 49, 19, BLACK)
    a.clip_to_disc()
    return a


OPTIONS = [
    ("dawn", "Dawn on the ridge", option_dawn),
    ("duel", "The duel", option_duel),
    ("wolf", "Wolf and moon", option_wolf),
    ("needle", "The needle", option_needle),
    ("map", "The trail map", option_map),
]


def sheet(images):
    """The five at 60 px x 5, at 30 px x 5, and on a light tile (the store's grey disc)."""
    cell = 330
    sheet = Image.new("RGBA", (cell * len(images), 700), (40, 40, 48, 255))
    d = ImageDraw.Draw(sheet)
    for i, (key, title, im60, im30) in enumerate(images):
        x = i * cell
        big = im60.resize((300, 300), Image.NEAREST)
        sheet.alpha_composite(big, (x + 15, 15))
        d.text((x + 20, 322), "%d  %s" % (i + 1, title), fill=(255, 255, 255, 255))
        # The sizes the watch shows, on the watch's dark launcher and on a light tile.
        for j, (bg, sz, label) in enumerate(((10, 60, "60 on dark"), (190, 60, "60 on light"))):
            col = (20, 20, 20, 255) if j == 0 else (225, 225, 225, 255)
            d.rectangle([x + 15, 360 + (0 if j == 0 else 150) - 0, x + 15 + 150, 360 + (0 if j == 0 else 150) + 130], fill=col)
            sheet.alpha_composite(im60.resize((120, 120), Image.NEAREST), (x + 30, 365 + (0 if j == 0 else 150)))
        for j in range(2):
            col = (20, 20, 20, 255) if j == 0 else (225, 225, 225, 255)
            d.rectangle([x + 180, 360 + 150 * j, x + 180 + 130, 360 + 150 * j + 130], fill=col)
            sheet.alpha_composite(im30.resize((90, 90), Image.NEAREST), (x + 200, 380 + 150 * j))
    return sheet


def main():
    os.makedirs(OUT, exist_ok=True)
    images = []
    for n, (key, title, fn) in enumerate(OPTIONS, 1):
        art = fn()
        im60, im30 = art.finish(60), art.finish(30)
        im60.save(os.path.join(OUT, "option-%d-%s-60.png" % (n, key)))
        im30.save(os.path.join(OUT, "option-%d-%s-30.png" % (n, key)))
        images.append((key, title, im60, im30))
        print("option %d: %s" % (n, title))
    sheet(images).save(os.path.join(OUT, "options-sheet.png"))
    print("wrote", os.path.normpath(OUT))


if __name__ == "__main__":
    main()
