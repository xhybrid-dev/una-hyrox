#!/usr/bin/env python3
"""Store images for X Weigh-in's two listings: previews, watch mock-ups and
marketing cards, one set per listing.

Everything on a watch screen is real, never redrawn by hand:
  - the app's screens are the simulator captures in Resources/previews/
    (docs/experiments/capture_screens.sh);
  - the glance is drawn from the real layout code's output
    (Tests/Host/tools/glance_preview), as glance_preview.py does, because the
    simulator cannot run glances.
Only the watch body around the screen, the headlines and the backgrounds are
drawn here. The watch is a plain round sports watch with four buttons, as
labelled on Jon's watch (UP, DOWN, SELECT, BACK). It carries no maker's logo.

    UNA_SDK=... store_images.py <glance_preview binary> <out-dir>

writes:
    <out-dir>/app/previews/    the five 480x480 screens (as uploaded)
    <out-dir>/app/mockups/     1080x1080, each screen on the watch
    <out-dir>/app/marketing/   1080x1080 feature cards and a 1920x1080 banner
    <out-dir>/glance/...       the same three for the glance
"""
import os
import shutil
import subprocess
import sys

from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
FONTS = os.path.join(os.environ.get("UNA_SDK", ""), "Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/assets/fonts")

# The app's own palette (SDK/GUI/Color.hpp): LIME 80C000 is its accent.
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

def watch(screen, face=600):
    """A round watch with the 240x240 @2x screen at @p face px, on a transparent
    canvas sized to fit it."""
    case_r = int(face * 0.70)
    bezel_r = int(face * 0.61)
    size = int(face * 1.62)
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    c = size // 2
    # Strap stubs top and bottom, to the canvas edge.
    sw = int(face * 0.33)
    d.rounded_rectangle([c - sw, 0, c + sw, size - 1], int(face * 0.07), fill=(22, 24, 27))
    # Four buttons, two a side.
    bw, bh = int(face * 0.05), int(face * 0.15)
    for sgn in (-1, 1):
        for dy in (-0.30, 0.30):
            x, y = c + sgn * (case_r - bw // 3), c + int(dy * face)
            d.rounded_rectangle([x - bw, y - bh // 2, x + bw, y + bh // 2], bw // 2, fill=(44, 47, 51))
    # Case, bezel ring, the black around the screen.
    d.ellipse([c - case_r, c - case_r, c + case_r, c + case_r], fill=(30, 33, 36))
    r2 = case_r - int(face * 0.02)
    d.ellipse([c - r2, c - r2, c + r2, c + r2], fill=(16, 18, 20))
    d.ellipse([c - bezel_r, c - bezel_r, c + bezel_r, c + bezel_r], fill=(6, 7, 8))
    # Button labels on the bezel ring, as on the watch.
    lab = font("Medium", max(12, int(face * 0.038)))
    rl = (bezel_r + r2) / 2 / 2 ** 0.5
    for text, sx, sy in (("UP", -1, -1), ("DOWN", -1, 1), ("SELECT", 1, -1), ("BACK", 1, 1)):
        d.text((c + sx * rl, c + sy * rl), text, font=lab, fill=(118, 124, 128), anchor="mm")
    # The screen, round.
    scr = screen.convert("RGB").resize((face, face), Image.LANCZOS)
    mask = Image.new("L", (face, face), 0)
    ImageDraw.Draw(mask).ellipse([0, 0, face - 1, face - 1], fill=255)
    img.paste(scr, (c - face // 2, c - face // 2), mask)
    # A faint glass highlight.
    glare = Image.new("RGBA", (face, face), (0, 0, 0, 0))
    ImageDraw.Draw(glare).ellipse([-face * 0.3, -face * 0.55, face * 0.9, face * 0.35], fill=(255, 255, 255, 6))
    img.alpha_composite(Image.composite(glare, Image.new("RGBA", (face, face)), mask), (c - face // 2, c - face // 2))
    return img


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


def card(screen, title, body, brand, path, w=1080, h=1080):
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
    wt = shadowed(watch(screen, face=410))
    img.alpha_composite(wt, ((w - wt.width) // 2, max(y + 10, h - wt.height + 10)))
    img.convert("RGB").save(path)
    print("wrote", os.path.relpath(path, REPO))


def banner(screens, title, sub, brand, path, w=1920, h=1080):
    """A wide banner: the headline on the left, up to three watches on the right."""
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
    # (face, centre x, centre y): the first screen in front, the others behind.
    spots = [(310, 1420, 540), (220, 1080, 570), (220, 1740, 570)]
    for scr, (face, x, yy) in reversed(list(zip(screens, spots))):
        wt = shadowed(watch(scr, face=face))
        img.alpha_composite(wt, (x - wt.width // 2, yy - wt.height // 2))
    img.convert("RGB").save(path)
    print("wrote", os.path.relpath(path, REPO))


# -- The glance's screens -----------------------------------------------------

COLOURS = {0x3F: (255, 255, 255), 0x2A: (170, 170, 170), 0x15: (85, 85, 85), 0x00: (0, 0, 0),
           0x0C: (0, 255, 0)}
SIZES = {0: (18, "Regular"), 2: (18, "Medium"), 5: (20, "SemiBold"), 6: (25, "SemiBold")}
BAND = 172
LIST_LINE = (70, 90, 255)


def glance_screens(binary, s=2):
    """{panel name: a 480x480 round screen with the glance between the list's
    separators}, from the real layout code's output (Gate W3 geometry)."""
    lines = subprocess.run([binary, "240", "60", "32"], capture_output=True, text=True, check=True).stdout.splitlines()
    panels, cur = {}, None
    for line in lines:
        p = line.split(" ")
        if p[0] == "state":
            cur = Image.new("RGB", (int(p[-2]) * s, int(p[-1]) * s), (0, 0, 0))
            panels[" ".join(p[1:-2])] = cur
            continue
        d = ImageDraw.Draw(cur)
        if p[0] == "rect":
            x, y, w, h, c = map(int, p[1:6])
            d.rectangle([x * s, y * s, (x + w) * s - 1, (y + h) * s - 1], fill=COLOURS.get(c, (255, 0, 255)))
        elif p[0] == "text":
            x, y, w, h, f, c, a = map(int, p[1:8])
            text = " ".join(p[8:])
            size, weight = SIZES.get(f, (18, "Regular"))
            fo = font(weight, size * s)
            tw = d.textlength(text, font=fo)
            tx = x * s if a == 0 else (x * s + (w * s - tw) / 2 if a == 1 else x * s + w * s - tw)
            d.text((tx, y * s), text, font=fo, fill=COLOURS.get(c, (255, 0, 255)))
    out = {}
    for name, pan in panels.items():
        if pan.width != 240 * s:
            continue
        size = 240 * s
        scr = Image.new("RGB", (size, size), (0, 0, 0))
        top = (size - pan.height) // 2
        scr.paste(pan, (0, top))
        d = ImageDraw.Draw(scr)
        x0 = (size - BAND * s) // 2
        for ly in (top - 10, top + pan.height + 8):
            d.rectangle([x0, ly, x0 + BAND * s - 1, ly + 2], fill=LIST_LINE)
        out[name] = scr
    return out


# -- Main -----------------------------------------------------------------------

def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    binary, out = sys.argv[1], sys.argv[2]
    res = os.path.join(REPO, "Resources")

    # The app.
    app = os.path.join(out, "app")
    for sub in ("previews", "mockups", "marketing"):
        os.makedirs(os.path.join(app, sub), exist_ok=True)
    names = ["1-home", "2-graph", "3-recent", "4-weigh-in", "5-saved"]
    shots = {}
    for n in names:
        src = os.path.join(res, "previews", n + ".png")
        shutil.copy(src, os.path.join(app, "previews", n + ".png"))
        shots[n] = Image.open(src)
        m = background(1080, 1080)
        wt = shadowed(watch(shots[n], face=600))
        m.alpha_composite(wt, ((1080 - wt.width) // 2, (1080 - wt.height) // 2))
        m.convert("RGB").save(os.path.join(app, "mockups", n + ".png"))
        print("wrote", os.path.relpath(os.path.join(app, "mockups", n + ".png"), REPO))
    mk = os.path.join(app, "marketing")
    brand = "X Weigh-in"
    card(shots["1-home"], "See where your weight is heading",
         "A smoothed trend rides over the day-to-day swings, with the change over the last week.",
         brand, os.path.join(mk, "1-trend.png"))
    card(shots["4-weigh-in"], "Weigh in from your wrist",
         "In kg, in 0.1 kg steps, starting from your last weigh-in. Hold UP or DOWN to move quickly.",
         brand, os.path.join(mk, "2-weigh-in.png"))
    card(shots["2-graph"], "30 days at a glance",
         "The trend line and every weigh-in, with your last five days a page away.",
         brand, os.path.join(mk, "3-graph.png"))
    card(shots["5-saved"], "Your weigh-ins stay on your watch",
         "Nothing is sent to your phone or recorded as an activity. A tracker, not a medical device.",
         brand, os.path.join(mk, "4-private.png"))
    banner([shots["1-home"], shots["2-graph"], shots["4-weigh-in"]], "Your weight trend, on your wrist",
           "Weigh in, see the trend, track the week.", brand, os.path.join(mk, "banner.png"))

    # The glance.
    gl = os.path.join(out, "glance")
    for sub in ("previews", "mockups", "marketing"):
        os.makedirs(os.path.join(gl, sub), exist_ok=True)
    gres = os.path.join(res, "Glance", "previews")
    for f in sorted(os.listdir(gres)):
        shutil.copy(os.path.join(gres, f), os.path.join(gl, "previews", f))
    screens = glance_screens(binary)
    pick = [("1-six-weeks", "Six weeks, weighed in today"), ("2-days-off", "Back after five days off"),
            ("3-first-days", "The first three days")]
    for fname, pname in pick:
        m = background(1080, 1080)
        wt = shadowed(watch(screens[pname], face=600))
        m.alpha_composite(wt, ((1080 - wt.width) // 2, (1080 - wt.height) // 2))
        m.convert("RGB").save(os.path.join(gl, "mockups", fname + ".png"))
        print("wrote", os.path.relpath(os.path.join(gl, "mockups", fname + ".png"), REPO))
    mk = os.path.join(gl, "marketing")
    brand = "X Weigh-in Glance"
    card(screens[pick[0][1]], "Your trend, without opening an app",
         "Your trend weight and the change over the last week, right on the glances screen.",
         brand, os.path.join(mk, "1-trend.png"))
    card(screens[pick[1][1]], "Two weeks of weigh-ins as bars",
         "One bar for each day you weighed in, so a few days off show up at once.",
         brand, os.path.join(mk, "2-bars.png"))
    card(screens[pick[2][1]], "Works with X Weigh-in",
         "Weigh in with the X Weigh-in app; the glance shows what it has logged.",
         brand, os.path.join(mk, "3-needs-app.png"))
    banner([screens[pick[0][1]], screens[pick[1][1]], screens[pick[2][1]]], "Your weight trend at a glance",
           "Needs the X Weigh-in app.", brand, os.path.join(mk, "banner.png"))


if __name__ == "__main__":
    main()
