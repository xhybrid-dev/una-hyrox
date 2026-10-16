#!/usr/bin/env python3
"""Store images for Knitting: the previews, the watch mock-ups in white and
teal, and the marketing cards and banner that show what the app does.

Everything on a watch screen is real, never redrawn by hand: the four screens
are the simulator captures in Resources/previews/
(docs/experiments/store_previews.sh). The watch is UNA's own render from the
SDK's Figma UI Resource Pack, read from $UNA_SDK. Only the backgrounds and the
headlines are drawn here, and the headlines make only the claims in
SUBMISSION.md's checked store text.

    UNA_SDK=... store_images.py <out-dir>

writes:
    <out-dir>/previews/    the four 480x480 screens (as uploaded in the package)
    <out-dir>/mockups/     1080x1080, each screen on a teal and on a white watch
    <out-dir>/marketing/   1080x1080 feature cards and a 1920x1080 banner
"""
import io
import os
import shutil
import subprocess
import sys
import zipfile

from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
FONTS = os.path.join(os.environ.get("UNA_SDK", ""), "Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/assets/fonts")

# The app's own palette (SDK/GUI/Color.hpp): LIME 80C000 is the icon's accent.
LIME = (0x80, 0xC0, 0x00)
WHITE = (255, 255, 255)
GREY = (150, 156, 160)
BG = (14, 16, 18)
BG2 = (28, 32, 36)


def font(weight, size):
    path = os.path.join(FONTS, "Poppins-%s.ttf" % weight)
    if not os.path.isfile(path):
        sys.exit("Poppins not found: set UNA_SDK (%s)" % path)
    return ImageFont.truetype(path, size)


# -- The watch --------------------------------------------------------------

# UNA's own watch renders, from the SDK's Figma UI Resource Pack
# (una-sdk Docs/Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig, a zip;
# Docs/figma-ui-kit.rst). Each is 2000x2000 with the screen left transparent:
# a 734 px circle centred at (999.5, 1010.5), measured from the alpha channel.
# Read from $UNA_SDK at run time, not copied into this repository.
FIG = os.path.join(os.environ.get("UNA_SDK", ""), "Docs/Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig")
RENDERS = {"black": "images/7fcc1a77ec68190d68598271ef7001237214c36d",
           "teal": "images/4ae4be5e2931ad615f699b142316f5a8f66563f5",
           "white": "images/8a9fead257a6c70ffc99d9fe6f3c0ba3fc144d36"}
HOLE_CX, HOLE_CY, HOLE_D = 999.5, 1010.5, 734
CROP = (418, 201, 1583, 1797)   # the watch's own extent, plus a 10 px margin
_renders = {}


def render(colour):
    if colour not in _renders:
        if not os.path.isfile(FIG):
            sys.exit("The SDK's Figma pack is not at %s: set UNA_SDK" % FIG)
        with zipfile.ZipFile(FIG) as z, z.open(RENDERS[colour]) as f:
            _renders[colour] = Image.open(io.BytesIO(f.read())).convert("RGBA")
    return _renders[colour]


def watch(screen, height=1000, colour="black"):
    """UNA's watch render, @p height px tall, with the 240x240 @2x @p screen in
    its screen opening."""
    w = render(colour)
    base = Image.new("RGBA", w.size, (0, 0, 0, 0))
    # The screen slightly larger than the opening, so no gap shows at its edge.
    d = HOLE_D + 6
    scr = screen.convert("RGB").resize((d, d), Image.LANCZOS)
    mask = Image.new("L", (d, d), 0)
    ImageDraw.Draw(mask).ellipse([0, 0, d - 1, d - 1], fill=255)
    base.paste(scr, (int(HOLE_CX - d / 2 + 0.5), int(HOLE_CY - d / 2 + 0.5)), mask)
    base.alpha_composite(w)
    base = base.crop(CROP)
    scale = height / base.height
    return base.resize((int(base.width * scale), height), Image.LANCZOS)


def shadowed(img):
    """@p img with a soft drop shadow under it."""
    sh = Image.new("RGBA", img.size, (0, 0, 0, 0))
    sh.paste((0, 0, 0, 170), (0, 0), img.split()[3])
    sh = sh.filter(ImageFilter.GaussianBlur(28))
    out = Image.new("RGBA", img.size, (0, 0, 0, 0))
    out.alpha_composite(sh, (0, 18))
    out.alpha_composite(img)
    return out


def background(w, h):
    img = Image.new("RGB", (w, h), BG)
    d = ImageDraw.Draw(img)
    # A soft vertical gradient and a lime glow behind the watch.
    for y in range(h):
        t = y / max(1, h - 1)
        d.line([(0, y), (w, y)], fill=tuple(int(BG[i] + (BG2[i] - BG[i]) * t) for i in range(3)))
    glow = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    r = min(w, h) // 3
    ImageDraw.Draw(glow).ellipse([w // 2 - r, h // 2 - r + 80, w // 2 + r, h // 2 + r + 80], fill=LIME + (40,))
    glow = glow.filter(ImageFilter.GaussianBlur(120))
    img = img.convert("RGBA")
    img.alpha_composite(glow)
    return img


def wrap(d, text, f, width):
    words, lines, cur = text.split(), [], ""
    for w in words:
        t = (cur + " " + w).strip()
        if d.textlength(t, font=f) <= width:
            cur = t
        else:
            lines.append(cur)
            cur = w
    if cur:
        lines.append(cur)
    return lines


def card(screen, title, body, brand, path, colour="teal", w=1080, h=1080):
    """A feature card: the headline above, the watch below."""
    img = background(w, h)
    d = ImageDraw.Draw(img)
    d.text((70, 64), brand, font=font("SemiBold", 34), fill=LIME)
    y = 120
    for line in wrap(d, title, font("SemiBold", 64), w - 140):
        d.text((70, y), line, font=font("SemiBold", 64), fill=WHITE)
        y += 76
    y += 8
    for line in wrap(d, body, font("Regular", 34), w - 140):
        d.text((70, y), line, font=font("Regular", 34), fill=GREY)
        y += 46
    top = y + 20
    wt = shadowed(watch(screen, height=h - top - 20, colour=colour))
    img.alpha_composite(wt, ((w - wt.width) // 2, top))
    img.convert("RGB").save(path)
    print("wrote", os.path.relpath(path, REPO))


def banner(screens, title, sub, brand, path, w=1920, h=1080):
    """A wide banner: the headline on the left, a teal and a white watch on the right."""
    img = background(w, h)
    d = ImageDraw.Draw(img)
    d.text((110, 330), brand, font=font("SemiBold", 42), fill=LIME)
    y = 400
    for line in wrap(d, title, font("SemiBold", 84), 700):
        d.text((110, y), line, font=font("SemiBold", 84), fill=WHITE)
        y += 100
    y += 10
    for line in wrap(d, sub, font("Regular", 40), 700):
        d.text((110, y), line, font=font("Regular", 40), fill=GREY)
        y += 54
    # (height, centre x, centre y, colour): the first in front, the other behind.
    spots = [(880, 1400, 545, "teal"), (640, 1010, 580, "white")]
    for scr, (ht, x, yy, col) in reversed(list(zip(screens, spots))):
        wt = shadowed(watch(scr, height=ht, colour=col))
        img.alpha_composite(wt, (x - wt.width // 2, yy - wt.height // 2))
    img.convert("RGB").save(path)
    print("wrote", os.path.relpath(path, REPO))


# -- Main -----------------------------------------------------------------------

def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    res = os.path.join(REPO, "Resources")
    for sub in ("previews", "mockups", "marketing"):
        os.makedirs(os.path.join(out, sub), exist_ok=True)

    names = ["1-counting", "2-undo", "3-reset", "4-next-set"]
    shots = {}
    for n in names:
        src = os.path.join(res, "previews", n + ".png")
        shutil.copy(src, os.path.join(out, "previews", n + ".png"))
        shots[n] = Image.open(src)
        for colour in ("teal", "white"):
            m = background(1080, 1080)
            wt = shadowed(watch(shots[n], height=1000, colour=colour))
            m.alpha_composite(wt, ((1080 - wt.width) // 2, (1080 - wt.height) // 2))
            path = os.path.join(out, "mockups", "%s-%s.png" % (n, colour))
            m.convert("RGB").save(path)
            print("wrote", os.path.relpath(path, REPO))

    mk = os.path.join(out, "marketing")
    brand = "Knitting"
    card(shots["1-counting"], "Count every row",
         "One press for each row. The rows in this set show big, with your set and your total beneath.",
         brand, os.path.join(mk, "1-count.png"), "teal")
    card(shots["4-next-set"], "The next set in one press",
         "Finished a set? Press once and the rows start again from zero.",
         brand, os.path.join(mk, "2-next-set.png"), "white")
    card(shots["2-undo"], "Hold to undo a row",
         "Pressed too often? Hold the row button for a moment to take the last row back.",
         brand, os.path.join(mk, "3-undo.png"), "teal")
    card(shots["3-reset"], "Start again on purpose",
         "Reset needs a hold, so a stray tap cannot wipe your project.",
         brand, os.path.join(mk, "4-reset.png"), "white")
    card(shots["1-counting"], "Opens where you left off",
         "Your count is saved every time it changes, and it stays on your watch.",
         brand, os.path.join(mk, "5-saved.png"), "teal")
    banner([shots["1-counting"], shots["4-next-set"]], "Rows and sets, on your wrist",
           "One press per row. Hold to undo.", brand, os.path.join(mk, "banner.png"))


if __name__ == "__main__":
    main()
