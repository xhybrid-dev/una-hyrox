#!/usr/bin/env python3
"""The X family's icons: one system for all six apps and both glances (Jon, 9 November
2026; README.md).

Each icon is a full disc in the app's colour with its glyph on it, drawn once on a 60-unit
grid and rendered:
  - for the watch, 60 and 30 px, in the colourway's grid colours (`icon` in brand.py): the
    .uapp keeps two bits a channel (ABGR2222), so only 0, 85, 170 and 255 survive. These are
    each app's Resources/icon_60x60.png and icon_30x30.png (the store's icon.png is the
    60 px one, so the phone and the watch show the same thing);
  - for the store page and socials, 1024 px in full brand colour:
    hybridx-link/docs/brand/icons/<app>-1024.png.
The glances carry no icon on the watch (APP_TYPE Glance); each has its own store icon, its
app's glyph reversed onto the light colour: Resources/Glance/icon_60x60.png.

    python3 hybridx-link/Tools/Brand/make_icons.py

Also writes hybridx-link/docs/brand/icons/watch-preview.png: every icon as the watch will
show it (the ABGR2222 truncation simulated), at 60 and 30 px, on the app list's black.
"""
import math
import os

from PIL import Image, ImageDraw

import brand

ROOT = brand.ROOT
OUT_MASTERS = os.path.join(ROOT, "hybridx-link", "docs", "brand", "icons")


def bezier(p0, p1, p2, p3, n=40):
    pts = []
    for i in range(n + 1):
        t = i / n
        u = 1 - t
        pts.append((u * u * u * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t * t * t * p3[0],
                    u * u * u * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t * t * t * p3[1]))
    return pts


class Pen:
    """Draws on the 60-unit grid at @p size px (supersampled by the caller)."""

    def __init__(self, img):
        self.img = img
        self.d = ImageDraw.Draw(img)
        self.k = img.width / 60.0

    def p(self, v):
        return v * self.k

    def pt(self, xy):
        return (self.p(xy[0]), self.p(xy[1]))

    def disc(self, colour, r=28.5):
        c = 30
        self.d.ellipse([self.p(c - r), self.p(c - r), self.p(c + r), self.p(c + r)], fill=colour)

    def circle(self, cx, cy, r, fill=None, outline=None, width=0):
        self.d.ellipse([self.p(cx - r), self.p(cy - r), self.p(cx + r), self.p(cy + r)],
                       fill=fill, outline=outline, width=round(self.p(width)) if width else 0)

    def line(self, pts, width, colour, round_ends=True):
        q = [self.pt(x) for x in pts]
        self.d.line(q, fill=colour, width=max(1, round(self.p(width))), joint="curve")
        if round_ends:
            r = self.p(width) / 2
            for x, y in (q[0], q[-1]):
                self.d.ellipse([x - r, y - r, x + r, y + r], fill=colour)

    def arc(self, cx, cy, r, start, end, width, colour):
        self.d.arc([self.p(cx - r), self.p(cy - r), self.p(cx + r), self.p(cy + r)], start, end,
                   fill=colour, width=max(1, round(self.p(width))))

    def poly(self, pts, colour):
        self.d.polygon([self.pt(x) for x in pts], fill=colour)


# -- The glyphs (each: pen, glyph colour, second colour) -----------------------------------

def g_race(pen, c, c2):
    # HybridX's own blade X: Race is the original, and wears the brand mark itself.
    h = round(pen.p(25))
    x = brand.xmark(h, c)
    pen.img.alpha_composite(x, (round(pen.p(30) - x.width / 2), round(pen.p(30.5) - h / 2)))


def g_run(pen, c, c2):
    # The engine: a gauge over the top, its last stretch in the red, the needle near it.
    cx, cy, r = 30, 33, 16.5
    pen.arc(cx, cy, r, 150, 330, 5.5, c)
    pen.arc(cx, cy, r, 330, 390, 5.5, c2)
    a = math.radians(-38)
    pen.line([(cx, cy), (cx + 12.5 * math.cos(a), cy + 12.5 * math.sin(a))], 4.2, c)
    pen.circle(cx, cy, 4.2, fill=c)


def g_trail(pen, c, c2):
    # The route: a line through the country, from a start dot to a finish ring.
    pts = bezier((17, 44), (44, 46), (12, 17), (42, 15.5))
    pen.line(pts, 5.2, c)
    pen.circle(17, 44, 5.0, fill=c2)
    pen.circle(42.5, 15.5, 4.6, outline=c2, width=2.4)


def g_ride(pen, c, c2):
    # Two wheels and a frame.
    rear, front, bb = (18.5, 37.5), (41.5, 37.5), (28.5, 37.5)
    seat, head = (25, 24.5), (38.5, 26)
    for w in (rear, front):
        pen.circle(w[0], w[1], 9.6, outline=c, width=3.2)
    pen.line([rear, bb, seat, head, bb], 3.4, c2)
    pen.line([head, front], 3.4, c2)
    pen.line([(21.5, 22.8), (28.5, 22.8)], 3.0, c2)              # the saddle
    pen.line([(head[0] - 0.5, head[1]), (40, 21.5), (44.5, 21.5)], 3.0, c2)   # the bars


def g_workout(pen, c, c2):
    # A kettlebell (the iron of HybridX's own photographs).
    # The handle stands clear of the bell, so its opening reads even at 30 px.
    pen.circle(30, 37.5, 12.8, fill=c)
    pen.arc(30, 21.5, 9.6, 180, 360, 4.4, c)
    pen.line([(20.4, 21.5), (21.6, 30)], 4.4, c)
    pen.line([(39.6, 21.5), (38.4, 30)], 4.4, c)
    # A flat base: the bell's bottom cut off in the disc's colour.
    pen.d.rectangle([pen.p(14), pen.p(48.2), pen.p(46), pen.p(54)], fill=pen.img.getpixel(
        (round(pen.p(30)), round(pen.p(55))))[:3] + (255,))


def g_streak(pen, c, c2):
    # The summit, snow on its top and the flag planted (the app's own mountain).
    pen.poly([(11.5, 45.5), (30, 17.5), (48.5, 45.5)], c)
    pen.poly([(30, 17.5), (24.4, 26), (27.6, 24.6), (30, 27), (32.4, 24.6), (35.6, 26)], c2)
    pen.line([(30, 18), (30, 8.5)], 1.8, c2, round_ends=False)
    pen.poly([(30.6, 8.5), (38.5, 11), (30.6, 13.5)], c2)


GLYPHS = {"race": g_race, "run": g_run, "trail": g_trail, "ride": g_ride,
          "workout": g_workout, "streak": g_streak}


def icon(kind, size, disc, glyph, glyph2, ss=4):
    """One icon: @p size px, drawn at @p ss times and reduced with a box filter."""
    big = Image.new("RGBA", (size * ss, size * ss), (0, 0, 0, 0))
    pen = Pen(big)
    pen.disc(disc + (255,))
    GLYPHS[kind](pen, glyph + (255,), glyph2 + (255,))
    return big.resize((size, size), Image.BOX)


def master_colours(kind, glance=False):
    """The full brand colours of an app's 1024 px icon (its glance's: reversed onto light)."""
    cw = brand.COLOURWAYS[kind]
    table = {
        "race":    (cw["accent"], (0, 0, 0), (0, 0, 0)),
        "run":     (cw["dark"], cw["light"], cw["accent2"]),
        "trail":   (cw["dark"], cw["accent2"], cw["light"]),
        "ride":    (cw["accent"], (255, 255, 255), cw["accent2"]),
        "workout": (cw["accent"], cw["dark"], cw["dark"]),
        "streak":  (cw["dark"], cw["accent"], cw["light"]),
    }
    disc, g1, g2 = table[kind]
    if glance:
        if kind == "streak":
            return cw["light"], cw["dark"], cw["accent"]
        if kind == "run":
            return cw["light"], cw["dark"], cw["accent"]
    return disc, g1, g2


def grid_colours(kind, glance=False):
    ic = brand.COLOURWAYS[kind]["icon"]
    if glance:
        if kind == "streak":
            return (255, 255, 255), (0, 0, 85), (0, 170, 170)
        if kind == "run":
            return (255, 255, 255), (85, 0, 0), (255, 0, 0)
    return ic["disc"], ic["glyph"], ic["glyph2"]


def main():
    os.makedirs(OUT_MASTERS, exist_ok=True)
    listings = [(k, False) for k in brand.FAMILY] + [("streak", True), ("run", True)]
    previews = []
    for kind, glance in listings:
        cw = brand.COLOURWAYS[kind]
        res = os.path.join(ROOT, cw["folder"], "Resources", "Glance" if glance else "")
        os.makedirs(res, exist_ok=True)
        disc, g1, g2 = grid_colours(kind, glance)
        i60 = icon(kind, 60, disc, g1, g2)
        i60.save(os.path.join(res, "icon_60x60.png"))
        i30 = None
        if not glance:
            i30 = icon(kind, 30, disc, g1, g2)
            i30.save(os.path.join(res, "icon_30x30.png"))
        name = kind + ("-glance" if glance else "")
        m = icon(kind, 1024, *master_colours(kind, glance), ss=2)
        m.save(os.path.join(OUT_MASTERS, name + "-1024.png"))
        previews.append((name, i60, i30, m))
        print("wrote", name)

    # What the watch will show: each icon through the ABGR2222 truncation, 60 and 30 px at
    # 3x (nearest neighbour, so the pixels show), on the app list's black; then the master.
    cell = 420
    sheet = Image.new("RGB", (cell * len(previews), 470), (24, 24, 24))
    d = ImageDraw.Draw(sheet)
    for i, (name, i60, i30, m) in enumerate(previews):
        x0 = i * cell
        d.rectangle([x0 + 10, 10, x0 + cell - 10, 210], fill=(0, 0, 0))
        w60 = brand.abgr2222(i60).resize((180, 180), Image.NEAREST)
        sheet.paste(w60, (x0 + 20, 20), w60)
        if i30 is not None:
            w30 = brand.abgr2222(i30).resize((90, 90), Image.NEAREST)
            sheet.paste(w30, (x0 + 220, 65), w30)
            a30 = brand.abgr2222(i30)
            sheet.paste(a30, (x0 + 340, 95), a30)
        mm = m.resize((200, 200), Image.LANCZOS)
        sheet.paste(mm, (x0 + 110, 240), mm)
        d.text((x0 + 20, 448), name, fill=(220, 220, 220), font=brand.inter(18, "SemiBold"))
    sheet.save(os.path.join(OUT_MASTERS, "watch-preview.png"))
    print("wrote", os.path.join(OUT_MASTERS, "watch-preview.png"))


if __name__ == "__main__":
    main()
