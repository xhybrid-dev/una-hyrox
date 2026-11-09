#!/usr/bin/env python3
"""The X family's brand, as code: the colourways, the fonts, the HybridX X mark,
UNA's watch, and the type helpers the icons and the store images share.

The brand is HybridX's (hybridx.club; the web repository xhybrid-dev/hybridx-web-x):
the blade X (assets/x-mark.png is its "Icon Logo.png"), the yellow #FADB5C on black,
Space Grotesk for headlines and Inter for text (its handover/entry-funnel/docs/
07-brand-and-design.md), Anton for campaigns (its Instagram, "Build a Bigger Engine").
Each app has its own colourway (Jon, 9 November 2026), tied together by the X, the
type and the wordmark: hybridx-link/Tools/Brand/README.md.

The fonts are open (SIL OFL 1.1; their licences are beside them in fonts/). The watch
is UNA's own render from the SDK's Figma UI Resource Pack, read from $UNA_SDK at run
time, as Knitting's store_images.py does; none of it is copied here.
"""
import io
import os
import sys
import zipfile

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))   # the workspace


def hexrgb(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


# -- The colourways ----------------------------------------------------------------
# Full brand colours, for the store images and the 1024 px icons. `icon` is the same
# colourway on the watch's grid: an icon is ABGR2222 in the .uapp (app_merging.py takes
# the top two bits of each channel, v >> 6), so only 0, 85, 170 and 255 survive.

COLOURWAYS = {
    "race": {
        "name": "Classic", "app": "HybridX Race", "folder": "hybridx-race",
        "dark": hexrgb("#000000"), "deep": hexrgb("#1A1A1A"), "accent": hexrgb("#FADB5C"),
        "accent2": hexrgb("#FADB5C"), "light": hexrgb("#FFFFFF"), "ink": hexrgb("#000000"),
        "mute": hexrgb("#B3B3B3"), "mute_on_light": hexrgb("#4D4D4D"),
        "watch": "black",
        "icon": {"disc": (255, 255, 85), "glyph": (0, 0, 0), "glyph2": (0, 0, 0)},
    },
    "run": {
        "name": "Engine", "app": "X Run", "folder": "hybridx-run",
        "dark": hexrgb("#5C0F1A"), "deep": hexrgb("#2C0710"), "accent": hexrgb("#C1121F"),
        "accent2": hexrgb("#E63946"), "light": hexrgb("#FBF5EF"), "ink": hexrgb("#2A1619"),
        "mute": hexrgb("#E2C9C6"), "mute_on_light": hexrgb("#4A3034"),
        "watch": "black",
        "icon": {"disc": (85, 0, 0), "glyph": (255, 255, 255), "glyph2": (255, 85, 85)},
    },
    "trail": {
        "name": "Moss", "app": "X Trail", "folder": "hybridx-trail",
        "dark": hexrgb("#1F3A2B"), "deep": hexrgb("#13251B"), "accent": hexrgb("#6B8F3A"),
        "accent2": hexrgb("#E040A0"), "light": hexrgb("#EFE9DC"), "ink": hexrgb("#16261D"),
        "mute": hexrgb("#C9D3C2"), "mute_on_light": hexrgb("#3D4A41"),
        "watch": "white",
        "icon": {"disc": (0, 85, 0), "glyph": (255, 85, 170), "glyph2": (255, 255, 255)},
    },
    "ride": {
        "name": "Velodrome", "app": "X Ride", "folder": "hybridx-ride",
        "dark": hexrgb("#0E1A4F"), "deep": hexrgb("#08102F"), "accent": hexrgb("#2346E8"),
        "accent2": hexrgb("#FF6A1A"), "light": hexrgb("#F2F4FB"), "ink": hexrgb("#0B1338"),
        "mute": hexrgb("#BCC6EA"), "mute_on_light": hexrgb("#33406E"),
        "watch": "black",
        "icon": {"disc": (0, 85, 255), "glyph": (255, 255, 255), "glyph2": (255, 85, 0)},
    },
    "workout": {
        "name": "Iron", "app": "X Workout", "folder": "hybridx-workout",
        "dark": hexrgb("#161618"), "deep": hexrgb("#0C0C0D"), "accent": hexrgb("#FF2E88"),
        "accent2": hexrgb("#FF2E88"), "light": hexrgb("#EDEDED"), "ink": hexrgb("#161618"),
        "mute": hexrgb("#B5B5B8"), "mute_on_light": hexrgb("#47474B"),
        "watch": "black",
        "icon": {"disc": (255, 0, 170), "glyph": (0, 0, 0), "glyph2": (0, 0, 0)},
    },
    "streak": {
        "name": "Glacier", "app": "X Streak", "folder": "hybridx-streak",
        "dark": hexrgb("#0B1F33"), "deep": hexrgb("#06121F"), "accent": hexrgb("#2EC4C4"),
        "accent2": hexrgb("#2EC4C4"), "light": hexrgb("#F4FAFB"), "ink": hexrgb("#0B1F33"),
        "mute": hexrgb("#B9D3DC"), "mute_on_light": hexrgb("#35505F"),
        "watch": "teal",
        "icon": {"disc": (0, 0, 85), "glyph": (0, 170, 170), "glyph2": (255, 255, 255)},
    },
}
FAMILY = ["race", "run", "trail", "ride", "workout", "streak"]


# -- Fonts ---------------------------------------------------------------------------

FONTS = os.path.join(HERE, "fonts")
_font_cache = {}


def anton(size):
    """Anton: the one big uppercase phrase on a card (HybridX's campaign face)."""
    key = ("anton", size)
    if key not in _font_cache:
        _font_cache[key] = ImageFont.truetype(os.path.join(FONTS, "Anton-Regular.ttf"), size)
    return _font_cache[key]


def grotesk(size, weight=700):
    """Space Grotesk, the brand's headline face, at 500 or 700 (the brand uses no other)."""
    key = ("grotesk", size, weight)
    if key not in _font_cache:
        f = ImageFont.truetype(os.path.join(FONTS, "SpaceGrotesk-Variable.ttf"), size)
        f.set_variation_by_axes([weight])
        _font_cache[key] = f
    return _font_cache[key]


def inter(size, weight="SemiBold"):
    """Inter, the brand's text face: Regular, SemiBold or Bold."""
    key = ("inter", size, weight)
    if key not in _font_cache:
        _font_cache[key] = ImageFont.truetype(os.path.join(FONTS, "Inter-%s.otf" % weight), size)
    return _font_cache[key]


# -- Text ----------------------------------------------------------------------------

def text_width(d, text, font, tracking=0.0):
    if tracking == 0:
        return d.textlength(text, font=font)
    size = font.size
    return sum(d.textlength(ch, font=font) for ch in text) + tracking * size * (len(text) - 1)


def draw_text(d, xy, text, font, fill, tracking=0.0, anchor="la"):
    """Text at @p xy; @p tracking in em (the brand's labels use 0.12 em). With tracking the
    anchor is the left baseline-free top ("la") only."""
    if tracking == 0:
        d.text(xy, text, font=font, fill=fill, anchor=anchor)
        return d.textlength(text, font=font)
    x, y = xy
    if anchor[0] == "m":
        x -= text_width(d, text, font, tracking) / 2
    elif anchor[0] == "r":
        x -= text_width(d, text, font, tracking)
    for ch in text:
        d.text((x, y), ch, font=font, fill=fill, anchor="l" + anchor[1])
        x += d.textlength(ch, font=font) + tracking * font.size
    return x - xy[0]


def label(d, xy, text, size, fill, anchor="la"):
    """An uppercase label: Inter 600, 0.12 em tracking (the brand's one tracking value)."""
    return draw_text(d, xy, text.upper(), inter(size, "SemiBold"), fill, 0.12, anchor)


def fit_anton(d, lines, max_w, max_size, min_size=60):
    """The largest Anton size, at most @p max_size, at which every line fits @p max_w."""
    size = max_size
    while size > min_size:
        if all(d.textlength(l, font=anton(size)) <= max_w for l in lines):
            return size
        size -= 4
    return min_size


# -- Contrast (WCAG) -------------------------------------------------------------------

def _lum(c):
    def ch(v):
        v /= 255.0
        return v / 12.92 if v <= 0.03928 else ((v + 0.055) / 1.055) ** 2.4
    r, g, b = c[:3]
    return 0.2126 * ch(r) + 0.7152 * ch(g) + 0.0722 * ch(b)


def contrast(a, b):
    la, lb = _lum(a), _lum(b)
    hi, lo = max(la, lb), min(la, lb)
    return (hi + 0.05) / (lo + 0.05)


# Every text colour on its ground, recorded as it is drawn, for the check at the end.
CONTRAST_LOG = []


def ink(fg, bg, what):
    CONTRAST_LOG.append((what, fg, bg, contrast(fg, bg)))
    return fg


# -- The X mark ------------------------------------------------------------------------

_xmark_alpha = None


def xmark(height, colour):
    """HybridX's blade X, @p height px tall, in @p colour (RGBA, transparent around it).
    The logo is a 593 px raster; its alpha is scaled with a cubic filter and then its
    edge is tightened (a contrast curve about the half-way level), so a large X keeps a
    clean edge rather than a soft one."""
    global _xmark_alpha
    if _xmark_alpha is None:
        src = Image.open(os.path.join(HERE, "assets", "x-mark.png")).convert("RGBA")
        a = src.split()[3]
        _xmark_alpha = a.crop(a.getbbox())
    a = _xmark_alpha
    w = max(1, round(a.width * height / a.height))
    big = a.resize((w, height), Image.BICUBIC)
    if height > a.height:
        big = big.point(lambda v: 0 if v < 64 else (255 if v > 192 else int((v - 64) * 2)))
    out = Image.new("RGBA", (w, height), colour + (255,) if len(colour) == 3 else colour)
    out.putalpha(big)
    return out


# -- UNA's watch -------------------------------------------------------------------------
# From Knitting's docs/experiments/store_images.py: the SDK's Figma pack is a zip; each
# render is 2000x2000 with the screen opening transparent, a 734 px circle centred at
# (999.5, 1010.5).

FIG = os.path.join(os.environ.get("UNA_SDK", os.path.join(ROOT, "una-sdk")),
                   "Docs/Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig")
RENDERS = {"black": "images/7fcc1a77ec68190d68598271ef7001237214c36d",
           "teal": "images/4ae4be5e2931ad615f699b142316f5a8f66563f5",
           "white": "images/8a9fead257a6c70ffc99d9fe6f3c0ba3fc144d36"}
HOLE_CX, HOLE_CY, HOLE_D = 999.5, 1010.5, 734
CROP = (418, 201, 1583, 1797)
_renders = {}


def _render(colour):
    if colour not in _renders:
        if not os.path.isfile(FIG):
            sys.exit("The SDK's Figma pack is not at %s: set UNA_SDK" % FIG)
        with zipfile.ZipFile(FIG) as z, z.open(RENDERS[colour]) as f:
            _renders[colour] = Image.open(io.BytesIO(f.read())).convert("RGBA")
    return _renders[colour]


def watch(screen, height, colour="black"):
    """UNA's watch render, @p height px tall, with @p screen (a 480 px simulator capture)
    in its opening."""
    w = _render(colour)
    base = Image.new("RGBA", w.size, (0, 0, 0, 0))
    d = HOLE_D + 6
    scr = screen.convert("RGB").resize((d, d), Image.LANCZOS)
    mask = Image.new("L", (d, d), 0)
    ImageDraw.Draw(mask).ellipse([0, 0, d - 1, d - 1], fill=255)
    base.paste(scr, (int(HOLE_CX - d / 2 + 0.5), int(HOLE_CY - d / 2 + 0.5)), mask)
    base.alpha_composite(w)
    base = base.crop(CROP)
    scale = height / base.height
    return base.resize((int(base.width * scale), height), Image.LANCZOS)


def screen(app, name):
    """A real simulator capture from the app's Resources/previews/ (or a glance's)."""
    return Image.open(os.path.join(ROOT, app, "Resources", name)).convert("RGB")


# -- The watch's icon format -------------------------------------------------------------

def abgr2222(img):
    """What the watch shows of an icon: app_merging.py keeps the top two bits of each
    channel and of alpha (v >> 6); the display shows each level at 0, 85, 170 or 255."""
    img = img.convert("RGBA")
    px = img.load()
    out = Image.new("RGBA", img.size)
    po = out.load()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = px[x, y]
            po[x, y] = ((r >> 6) * 85, (g >> 6) * 85, (b >> 6) * 85, (a >> 6) * 85)
    return out
