#!/usr/bin/env python3
"""Draw Trail Quest's pixel art from code, and write it where the GUI builds it.

Everything is drawn on a 24x24 logical grid (the game's pixel grid is 120x120
logical pixels, each shown as 2x2 panel pixels); a boss is drawn on 32x32, so
it stands taller at the same pixel size. Colours are indices into a
16-entry palette; index 0 is transparent. Every palette entry must sit on the
display's grid: 2 bits a channel, so each of R, G and B is one of
0x00, 0x40, 0x80, 0xC0 (SDK/GUI/Color.hpp). check_palette() fails the run
otherwise, so what is drawn here is what the panel shows.

Output (deterministic, so diffs are reviewable):
  Software/Apps/LVGL-GUI/assets/sprites/sprites.c         the sprites, 4 bits a pixel
  Software/Apps/LVGL-GUI/gui/include/gui/Sprites.hpp      their declarations, the
                                                         index of each and the count
  docs/screens/art-sheet.png                               a preview, 6x, both frames

The header is generated with the table, so an index can never name a sprite
that is not there (the GUI also bounds-checks it: no MMU).

Why not LVGL's own indexed images: LVGL v9 decodes them to 32-bit ARGB in its
pool at draw time (una-sdk Docs/Tutorials/RunLVGL/ARCHITECTURE.md), which is no
saving on the watch. So the GUI keeps these packed sprites (288 bytes a frame,
512 for a boss) and expands
the few on screen into RGB565A8 buffers of its own (gui/Sprite.cpp).

    python3 gen_art.py          (needs Pillow for the preview only)
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SPRITES_C = os.path.join(ROOT, "Software", "Apps", "LVGL-GUI", "assets", "sprites", "sprites.c")
SPRITES_H = os.path.join(ROOT, "Software", "Apps", "LVGL-GUI", "gui", "include", "gui", "Sprites.hpp")
SHEET_PNG = os.path.join(ROOT, "docs", "screens", "art-sheet.png")

SIZE = 24        # a sprite's side, logical pixels
BOSS_SIZE = 32   # a boss's

# Palette index -> 0xRRGGBB. Index 0 is transparent (its colour is unused).
PALETTE = [
    0x000000,  # 0  transparent
    0x000040,  # 1  outline
    0xC0C0C0,  # 2  white
    0x808080,  # 3  grey
    0x404040,  # 4  dark grey
    0xC08000,  # 5  orange light
    0xC04000,  # 6  orange mid
    0x804000,  # 7  orange dark
    0xC0C080,  # 8  cream
    0x804040,  # 9  brown mid
    0x400000,  # 10 brown dark
    0x00C000,  # 11 green
    0x008000,  # 12 green dark
    0xC00000,  # 13 red
    0x0080C0,  # 14 sky blue
    0xC0C000,  # 15 yellow
]

OUTLINE = 1


def check_palette():
    bad = [(i, hex(c)) for i, c in enumerate(PALETTE)
           if any(((c >> s) & 0xFF) not in (0x00, 0x40, 0x80, 0xC0) for s in (16, 8, 0))]
    if bad:
        sys.exit("palette entries off the display's grid (0x00/0x40/0x80/0xC0): %s" % bad)
    if len(PALETTE) != 16 or len(set(PALETTE[1:])) != 15:
        sys.exit("palette must have 16 entries, 15 of them distinct")


class Canvas:
    def __init__(self, size=SIZE):
        self.size = size
        self.px = [[0] * size for _ in range(size)]

    def set(self, x, y, c):
        if 0 <= x < self.size and 0 <= y < self.size:
            self.px[y][x] = c

    def rect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.set(x, y, c)

    def ellipse(self, cx, cy, rx, ry, c):
        """Filled ellipse centred on the pixel (cx, cy), radii in pixels."""
        for y in range(cy - ry, cy + ry + 1):
            for x in range(cx - rx, cx + rx + 1):
                dx, dy = (x - cx) / (rx + 0.5), (y - cy) / (ry + 0.5)
                if dx * dx + dy * dy <= 1.0:
                    self.set(x, y, c)

    def tri(self, a, b, c3, col):
        """Filled triangle by barycentric test (integer-free, but deterministic)."""
        (x0, y0), (x1, y1), (x2, y2) = a, b, c3
        for y in range(self.size):
            for x in range(self.size):
                d = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2)
                if d == 0:
                    continue
                l1 = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) / d
                l2 = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) / d
                l3 = 1 - l1 - l2
                if l1 >= 0 and l2 >= 0 and l3 >= 0:
                    self.set(x, y, col)

    def outline(self):
        """Dark edge round every shape, drawn into the transparent pixels."""
        out = [row[:] for row in self.px]
        n = self.size
        for y in range(n):
            for x in range(n):
                if self.px[y][x] != 0:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < n and 0 <= ny < n and self.px[ny][nx] not in (0,):
                        out[y][x] = OUTLINE
                        break
        self.px = out

    def shifted(self, dx, dy):
        c = Canvas(self.size)
        for y in range(self.size):
            for x in range(self.size):
                if self.px[y][x]:
                    c.set(x + dx, y + dy, self.px[y][x])
        return c


# -- the sprites ------------------------------------------------------------------------

def runner(frame):
    """The trail runner: red cap, green top, dark shorts, mid-stride legs."""
    c = Canvas()
    c.ellipse(12, 5, 3, 3, 8)                     # head
    c.rect(9, 2, 15, 3, 13)                       # cap
    c.rect(14, 3, 17, 3, 13)                      # cap peak
    c.rect(10, 9, 14, 15, 11)                     # top
    c.rect(10, 13, 14, 15, 12)                    # lower top, shaded
    c.rect(8, 10, 9, 14, 8)                       # back arm
    c.rect(15, 10, 16, 14, 8)                     # front arm
    c.rect(10, 16, 14, 17, 4)                     # shorts
    if frame == 0:
        c.rect(9, 18, 10, 21, 8)
        c.rect(14, 18, 15, 20, 8)
        c.rect(8, 22, 11, 22, 2)
        c.rect(14, 21, 17, 21, 2)
    else:
        c.rect(10, 18, 11, 20, 8)
        c.rect(13, 18, 15, 21, 8)
        c.rect(9, 21, 12, 21, 2)
        c.rect(13, 22, 16, 22, 2)
    c.set(14, 5, 1)                               # eye
    c.outline()
    return c


def fox(frame):
    """The Red Fox: it gives you a stitch (the yellow zig-zag)."""
    c = Canvas()
    c.tri((5, 9), (8, 1), (11, 9), 6)             # left ear
    c.tri((13, 9), (16, 1), (19, 9), 6)           # right ear
    c.ellipse(12, 14, 8, 6, 5)                    # body
    c.ellipse(12, 16, 5, 3, 8)                    # belly
    c.rect(4, 17, 5, 20, 7)                       # back foot
    c.rect(18, 17, 19, 20, 7)                     # front foot
    c.rect(20, 11, 22, 13, 6)                     # tail
    c.rect(22, 10, 23, 11, 2)                     # tail tip
    for x, y in ((8, 13), (9, 14), (10, 13), (11, 14), (12, 13)):
        c.set(x, y, 15)                           # the stitch
    c.set(8, 11, 1); c.set(15, 11, 1)             # eyes
    c.set(12, 13, 13)                             # nose
    c.outline()
    return c.shifted(0, -1) if frame else c


def bear(frame):
    """The Brown Bear: heavy and tired (it is the bonk); the blue drop is the sweat."""
    c = Canvas()
    c.ellipse(6, 5, 2, 2, 9)                      # left ear
    c.ellipse(18, 5, 2, 2, 9)                     # right ear
    c.ellipse(12, 8, 6, 5, 9)                     # head
    c.ellipse(12, 16, 9, 6, 9)                    # body
    c.ellipse(12, 17, 5, 4, 8)                    # belly
    c.rect(5, 20, 8, 22, 10)                      # back foot
    c.rect(16, 20, 19, 22, 10)                    # front foot
    c.rect(8, 8, 10, 8, 1); c.rect(14, 8, 16, 8, 1)   # half-closed eyes
    c.rect(11, 10, 13, 11, 10)                    # muzzle
    c.set(12, 10, 1)                              # nose
    c.set(19, 6, 14); c.set(19, 7, 14)            # sweat drop
    c.outline()
    return c.shifted(0, 1) if frame else c


def beetle(frame):
    """The Ruby Beetle: small and patient, red as a rubbed heel (a blister)."""
    c = Canvas()
    c.ellipse(12, 14, 8, 7, 13)                   # shell
    c.rect(12, 8, 12, 21, 10)                     # where the wing cases meet
    c.ellipse(9, 11, 1, 1, 8)                     # shine
    c.set(15, 16, 8); c.set(8, 17, 8); c.set(16, 12, 8)   # spots
    c.ellipse(12, 6, 4, 2, 10)                    # head
    c.set(10, 6, 2); c.set(14, 6, 2)              # eyes
    c.rect(8, 2, 8, 4, 10); c.rect(16, 2, 16, 4, 10)      # feelers
    legs = ((3, 11), (2, 15), (3, 19)) if frame == 0 else ((2, 12), (3, 16), (2, 20))
    for x, y in legs:
        c.rect(x, y, x + 1, y, 10)                # left legs
        c.rect(22 - x, y, 23 - x, y, 10)          # right legs
    c.outline()
    return c


def crab(frame):
    """The Sand Crab: walks sideways into every seam you own (chafing)."""
    c = Canvas()
    c.ellipse(12, 15, 8, 4, 6)                    # body
    c.ellipse(12, 16, 5, 2, 5)                    # underside
    up = 1 if frame else 0
    for cx in (3, 21):
        c.ellipse(cx, 9 - up, 2, 2, 6)            # claw
        c.set(cx, 7 - up, 0)                      # the pincer's gap
        c.rect(cx, 11 - up, cx, 13, 7)            # arm
    c.rect(9, 8, 9, 11, 7); c.rect(15, 8, 15, 11, 7)      # eye stalks
    c.set(9, 7, 2); c.set(15, 7, 2)               # eyes
    for x in (6, 9, 15, 18):
        c.rect(x, 19, x, 21, 7)                   # legs
    c.outline()
    return c


def tortoise(frame):
    """The Old Tortoise: mossy, the valley's guard. Never once hurried."""
    c = Canvas(BOSS_SIZE)
    c.ellipse(18, 16, 12, 9, 12)                  # shell
    c.rect(7, 21, 29, 22, 9)                      # the shell's rim
    for x, y in ((12, 17), (18, 18), (24, 17), (15, 12), (21, 12)):
        c.ellipse(x, y, 2, 1, 9)                  # plates
    c.ellipse(16, 9, 4, 2, 11)                    # moss on top
    c.ellipse(22, 10, 3, 1, 11)
    c.set(13, 10, 11); c.set(25, 13, 11)          # tufts
    c.rect(9, 24, 12, 28, 3); c.rect(23, 24, 26, 28, 3)      # legs
    c.rect(8, 28, 13, 28, 4); c.rect(22, 28, 27, 28, 4)      # feet
    dy = 1 if frame else 0
    c.rect(3, 15 + dy, 7, 19 + dy, 3)             # neck
    c.ellipse(4, 13 + dy, 3, 3, 3)                # head (it faces the runner)
    c.rect(1, 13 + dy, 3, 13 + dy, 1)             # a half-shut eye
    c.rect(1, 16 + dy, 3, 16 + dy, 4)             # mouth
    c.outline()
    return c


SPRITES = [
    ("runner", runner),
    ("fox", fox),
    ("bear", bear),
    ("beetle", beetle),
    ("crab", crab),
    ("tortoise", tortoise),
]


# -- output -----------------------------------------------------------------------------

def packed(canvas):
    """Two pixels a byte, high nibble first, row-major: 288 bytes (512 for a boss)."""
    n = canvas.size
    flat = [canvas.px[y][x] for y in range(n) for x in range(n)]
    return [(flat[i] << 4) | flat[i + 1] for i in range(0, len(flat), 2)]


def write_c(sprites):
    os.makedirs(os.path.dirname(SPRITES_C), exist_ok=True)
    lines = [
        "/* Generated by Tools/Art/gen_art.py -- do not edit.",
        " * Square sprites (24x24, a boss 32x32), 4 bits a pixel (high nibble first), index 0 transparent. */",
        '#include "gui/Sprites.hpp"',
        "",
        "const uint32_t trailquest_palette[16] = {",
        "    " + ", ".join("0x%06X" % c for c in PALETTE),
        "};",
        "",
    ]
    for name, frames in sprites:
        for f, canvas in enumerate(frames):
            data = packed(canvas)
            lines.append("static const uint8_t sprite_%s_%d[%d] = {" % (name, f, len(data)))
            for i in range(0, len(data), 16):
                lines.append("    " + ", ".join("0x%02X" % b for b in data[i:i + 16]) + ",")
            lines.append("};")
            lines.append("")
    lines.append("const TrailQuestSprite trailquest_sprites[TRAILQUEST_SPRITE_COUNT] = {")
    for name, frames in sprites:
        lines.append('    { "%s", %d, { sprite_%s_0, sprite_%s_1 } },' % (name, frames[0].size, name, name))
    lines.append("};")
    lines.append("")
    with open(SPRITES_C, "w", newline="\n") as f:
        f.write("\n".join(lines))
    print("wrote", SPRITES_C)


def write_h(sprites):
    lines = [
        "/**",
        " ******************************************************************************",
        " * @file    Sprites.hpp",
        " * @brief   The packed sprites. Generated by Tools/Art/gen_art.py -- do not edit.",
        " *",
        " * Square, 24x24 logical pixels (a boss 32x32), 4 bits a pixel (high nibble",
        " * first), palette index 0 transparent, two frames each: 288 bytes a frame (512",
        " * for a boss). See gui/Sprite.hpp for how they reach the screen.",
        " ******************************************************************************",
        " */",
        "",
        "#ifndef TRAILQUEST_SPRITES_HPP",
        "#define TRAILQUEST_SPRITES_HPP",
        "",
        "#include <stdint.h>",
        "",
    ]
    for i, (name, _) in enumerate(sprites):
        lines.append("#define TRAILQUEST_SPRITE_%s %d" % (name.upper(), i))
    lines += [
        "#define TRAILQUEST_SPRITE_COUNT %d" % len(sprites),
        "",
        "#ifdef __cplusplus",
        'extern "C" {',
        "#endif",
        "",
        "typedef struct {",
        "    const char*    name;",
        "    uint8_t        size;       /* side, logical pixels: 24, or 32 for a boss */",
        "    const uint8_t* frame[2];",
        "} TrailQuestSprite;",
        "",
        "/// The largest side of any sprite.",
        "#define TRAILQUEST_SPRITE_MAX_SIZE %d" % max(f[0].size for _, f in sprites),
        "",
        "/// 16 entries, 0xRRGGBB, each channel 0x00/0x40/0x80/0xC0; entry 0 unused.",
        "extern const uint32_t trailquest_palette[16];",
        "",
        "extern const TrailQuestSprite trailquest_sprites[TRAILQUEST_SPRITE_COUNT];",
        "",
        "#ifdef __cplusplus",
        '} // extern "C"',
        "#endif",
        "",
        "#endif // TRAILQUEST_SPRITES_HPP",
        "",
    ]
    with open(SPRITES_H, "w", newline="\n") as f:
        f.write("\n".join(lines))
    print("wrote", SPRITES_H)


def write_sheet(sprites):
    try:
        from PIL import Image
    except ImportError:
        print("Pillow not installed: no preview sheet")
        return
    scale, pad = 6, 8
    cell = max(f[0].size for _, f in sprites)
    cols = len(sprites) * 2
    sheet = Image.new("RGB", (cols * (cell * scale + pad) + pad, cell * scale + 2 * pad), (0x40, 0x80, 0x40))
    px = sheet.load()
    for i, (_, frames) in enumerate(sprites):
        for f, canvas in enumerate(frames):
            ox = pad + (i * 2 + f) * (cell * scale + pad)
            oy = pad + (cell - canvas.size) * scale   # standing on one line
            for y in range(canvas.size):
                for x in range(canvas.size):
                    idx = canvas.px[y][x]
                    if not idx:
                        continue
                    c = PALETTE[idx]
                    rgb = ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF)
                    for yy in range(scale):
                        for xx in range(scale):
                            px[ox + x * scale + xx, oy + y * scale + yy] = rgb
    os.makedirs(os.path.dirname(SHEET_PNG), exist_ok=True)
    sheet.save(SHEET_PNG)
    print("wrote", SHEET_PNG)


def main():
    check_palette()
    sprites = [(name, [fn(0), fn(1)]) for name, fn in SPRITES]
    for name, frames in sprites:
        if all(packed(frames[0]) == packed(f) for f in frames[1:]):
            sys.exit("%s: the two frames are identical" % name)
    write_c(sprites)
    write_h(sprites)
    write_sheet(sprites)


if __name__ == "__main__":
    main()
