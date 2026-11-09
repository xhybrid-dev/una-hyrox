#!/usr/bin/env python3
"""The X family's store images, in the HybridX brand (Jon, 9 November 2026; README.md).

For each listing, into its app's docs/store/ (a glance's into docs/store/glance/):
  01-banner.png            1920 x 1080, the first image of the listing
  02-... to 06-....png     1080 x 1080 feature cards, ending with the family card
and hybridx-link/docs/brand/family-banner.png, the whole family.

The system:
  - each app keeps to its own colourway (brand.COLOURWAYS); its cards alternate a dark
    ground and a light one;
  - HybridX's blade X, big and cropped, behind the watch on the dark cards; on the light ones a
    band cut at the X's 45 degrees;
  - Anton for the one big phrase, Space Grotesk for the line under it, Inter for labels
    (uppercase, 0.12 em), HYBRIDX.CLUB at the foot of every image;
  - the watch is UNA's own render with a real simulator capture (Resources/previews/);
  - every headline is a claim from hybridx-link/docs/STORE_RELEASE.md's checked facts.
Text colours are checked for contrast (>= 4.5:1) as they are drawn; the run fails if any is not.

    UNA_SDK=... python3 hybridx-link/Tools/Brand/store_images.py
"""
import os
import sys

from PIL import Image, ImageDraw

import brand
from brand import COLOURWAYS, anton, grotesk, inter

ROOT = brand.ROOT
ICONS = os.path.join(ROOT, "hybridx-link", "docs", "brand", "icons")

# -- The listings ---------------------------------------------------------------------------
# Each card: (headline lines, the line under it, the screen, light ground?).

LISTINGS = {
    "race": {
        "title": "HYBRIDX RACE", "kicker": "For UNA Watch",
        "tagline": "One button. Every split.",
        "body": "A race and simulation timer for HYROX-format events. Every run and station is a lap.",
        "hero": "previews/4-race-station.png",
        "cards": [
            (["ONE BUTTON.", "EVERY SPLIT."], "Press once as each run or station ends. The split is on your wrist at once.",
             "previews/5-split.png", False),
            (["EVERY RUN.", "EVERY STATION."], "Sixteen segments, each its own lap, with what comes next.",
             "previews/4-race-station.png", True),
            (["ON YOUR", "MARKS"], "A full race or a simulation, with runs from 100 m to the full kilometre.",
             "previews/2-on-your-marks.png", False),
            (["THE WHOLE", "RACE"], "Runs, stations and heart rate in one summary, synced to the UNA app and Strava.",
             "previews/6-summary.png", True),
        ],
    },
    "run": {
        "title": "X RUN", "kicker": "For UNA Watch",
        "tagline": "Build a bigger engine.",
        "body": "A running app with its own VO2max estimate, worked out on the watch from your steady running.",
        "hero": "previews/3-vo2max.png",
        "cards": [
            (["VO2MAX ON", "YOUR WRIST"], "Estimated from steady running: pace and gradient against your heart-rate reserve.",
             "previews/3-vo2max.png", False),
            (["FOLLOW THE", "TREND"], "Your figure and the runs behind it. An estimate for your own trend, not a lab test.",
             "previews/4-recent-runs.png", True),
            (["EVERY", "DETAIL"], "Cadence, time in each zone, your best 1 km, mile and 5 km, and how evenly you paced it.",
             "previews/6-best-splits.png", False),
            (["PACE.", "DISTANCE.", "HEART."], "Outdoors with GPS, or on a treadmill, synced to the UNA app and Strava.",
             "previews/1-running.png", True),
        ],
    },
    "run-glance": {
        "kind": "run", "glance": True, "name": "VO2max Glance", "needs": "X Run",
        "title": "VO2MAX GLANCE", "kicker": "For UNA Watch",
        "tagline": "Your engine, at a glance.",
        "body": "Your X Run VO2max on the glances screen, with your last 10 runs as bars. Needs X Run.",
        "hero": "Glance/previews/1-getting-fitter.png",
        "cards": [
            (["YOUR LAST", "10 RUNS"], "The figure X Run shows, its change at your latest run, and the trend as bars.",
             "Glance/previews/1-getting-fitter.png", True),
            (["NEEDS", "X RUN"], "X Run works the figure out. The glance shows what it has saved.",
             "Glance/previews/2-steady.png", False),
        ],
    },
    "trail": {
        "title": "X TRAIL", "kicker": "For UNA Watch",
        "tagline": "Wander off? You'll feel it.",
        "body": "Follow a GPX route on your wrist, with a buzz when you leave the line.",
        "hero": "previews/4-off-course.png",
        "cards": [
            (["WANDER OFF?", "YOU'LL", "FEEL IT."], "25 m from the line: a buzz, a banner, and an arrow back to the route.",
             "previews/4-off-course.png", False),
            (["YOUR", "ROUTES"], "Copy your GPX files over USB. Each shows its distance and climb.",
             "previews/1-routes.png", True),
            (["HEADING", "UP"], "A map that turns with you, zoomed from 300 m to 3 km with one button.",
             "previews/3-map.png", False),
            (["KNOW THE", "CLIMB"], "An elevation profile for routes with heights, and turn cues before each turn.",
             "previews/5-elevation.png", True),
        ],
    },
    "ride": {
        "title": "X RIDE", "kicker": "For UNA Watch",
        "tagline": "Outdoors or on the turbo.",
        "body": "GPS speed, distance and climb outdoors. Intervals and Tabata on the turbo.",
        "hero": "previews/2-riding.png",
        "cards": [
            (["STOPS WHEN", "YOU STOP"], "Auto-pause holds the clock at the lights and starts it as you ride off.",
             "previews/3-auto-paused.png", False),
            (["INTERVALS", "ON THE", "TURBO"], "Your work, rest and rounds, or Tabata: a countdown and a buzz for every step.",
             "previews/4-intervals.png", True),
            (["EVERY RIDE,", "SUMMED UP"], "Average and top speed and the climb, synced to the UNA app and Strava.",
             "previews/5-summary.png", False),
            (["COUNTS IN", "X STREAK"], "Every saved ride counts towards your week, with its training load.",
             "previews/6-saved.png", True),
        ],
    },
    "workout": {
        "title": "X WORKOUT", "kicker": "For UNA Watch",
        "tagline": "Seven ways to train.",
        "body": "Strength, EMOM, AMRAP, Tabata, intervals and Hyrox-style circuits, timed for you.",
        "hero": "previews/2-strength-set.png",
        "cards": [
            (["SEVEN", "WAYS TO", "TRAIN"], "Free, Strength, EMOM, AMRAP, Tabata, Intervals and Hyrox-style Circuits.",
             "previews/1-menu.png", False),
            (["3, 2, 1.", "GO."], "Every step is timed: amber for the last three seconds, with a tick for each.",
             "previews/3-rest-countdown.png", True),
            (["SETS AND", "REST"], "Strength sets with a rest timer, each saved as a set in the file.",
             "previews/2-strength-set.png", False),
            (["HYROX-STYLE", "CIRCUITS"], "Runs and stations, each saved as its own lap.",
             "previews/5-circuit-station.png", True),
        ],
    },
    "streak": {
        "title": "X STREAK", "kicker": "For UNA Watch",
        "tagline": "Keep the streak. Climb the mountain.",
        "body": "A weekly streak for the X family. Every week you hit your target is a step up.",
        "hero": "previews/1-home.png",
        "cards": [
            (["EVERY WEEK,", "A STEP UP"], "Five summits, from Arthur's Seat at 4 weeks to Everest at two years.",
             "previews/1-home.png", False),
            (["SESSIONS", "COUNT", "THEMSELVES"], "Save in X Run, X Trail, X Ride, X Workout or HybridX Race: open Streak and it is there.",
             "previews/2-session-found.png", True),
            (["FITNESS.", "FATIGUE.", "FORM."], "Every session's heart-rate load, turned into where you are this week.",
             "previews/4-training-load.png", False),
            (["LIFE", "HAPPENS"], "Shields, earned every four good weeks, save the streak when you miss one.",
             "previews/5-shield.png", True),
        ],
    },
    "streak-glance": {
        "kind": "streak", "glance": True, "name": "X Streak Glance", "needs": "X Streak",
        "title": "X STREAK GLANCE", "kicker": "For UNA Watch",
        "tagline": "Your week, at a glance.",
        "body": "Your streak and this week's sessions on the glances screen. Needs X Streak.",
        "hero": "Glance/previews/1-climbing.png",
        "cards": [
            (["YOUR WEEK,", "AT A GLANCE"], "Your streak, this week against your target, and what comes next.",
             "Glance/previews/1-climbing.png", True),
            (["NEEDS", "X STREAK"], "Streak counts your sessions. The glance shows what it has saved.",
             "Glance/previews/3-at-risk.png", False),
        ],
    },
}

FAMILY_NAMES = {"race": "HybridX Race", "run": "X Run", "trail": "X Trail", "ride": "X Ride",
                "workout": "X Workout", "streak": "X Streak"}

# -- Colours on a card -----------------------------------------------------------------------


def scheme(cw, light):
    """The ground, the text colours and the graphic colours of one card."""
    if light and cw["app"] == "HybridX Race":
        g = cw["accent"]            # the brand's yellow fill, black on it
        return {"ground": g, "text": (0, 0, 0), "sub": (0, 0, 0), "band": (0, 0, 0), "x": (0, 0, 0),
                "label": (0, 0, 0)}
    if light:
        g = cw["light"]
        return {"ground": g, "text": cw["ink"], "sub": cw["mute_on_light"],
                "band": cw["dark"], "x": cw["accent2"] if cw["accent2"] != cw["accent"] else cw["accent"],
                "label": pick(cw, g, [cw["accent"], cw["dark"]])}
    g = cw["dark"]
    return {"ground": g, "text": (255, 255, 255), "sub": cw["mute"],
            "band": cw["deep"], "x": cw["accent"] if cw["accent"] != cw["dark"] else cw["accent2"],
            "label": pick(cw, g, [cw["accent2"], cw["accent"], cw["light"]])}


def pick(cw, ground, options):
    """The first colour that reads on @p ground (4.5:1)."""
    for c in options:
        if brand.contrast(c, ground) >= 4.5:
            return c
    return (255, 255, 255) if brand.contrast((255, 255, 255), ground) > brand.contrast((0, 0, 0), ground) else (0, 0, 0)


# -- Text, checked against what is really under it --------------------------------------------

LOW = []          # (image, what, worst contrast)
CHECKED = [0]


class Card:
    """An image being drawn: the background goes in first (ground, X, watch); then
    snapshot(); then text through put(), which records each block's box. check() measures
    each block's colour against the snapshot's pixels in its box (the worst colour covering at
    least 3 % of the box), so text on the X or on the watch is caught, not just on the ground."""

    def __init__(self, w, h, ground, name):
        self.img = Image.new("RGBA", (w, h), ground + (255,))
        self.ground = ground
        self.name = name
        self.blocks = []
        self.base = None

    def snapshot(self):
        self.base = self.img.convert("RGB")
        self.d = ImageDraw.Draw(self.img)

    def put(self, xy, text, font, fg, anchor="la", tracking=0.0, what="text"):
        d = self.d
        if tracking == 0:
            box = d.textbbox(xy, text, font=font, anchor=anchor)
            d.text(xy, text, font=font, fill=fg, anchor=anchor)
        else:
            brand.draw_text(d, xy, text, font, fg, tracking, anchor)
            w = brand.text_width(d, text, font, tracking)
            x = xy[0] - (w / 2 if anchor[0] == "m" else (w if anchor[0] == "r" else 0))
            b = d.textbbox((0, 0), text, font=font, anchor="l" + anchor[1])
            box = (x, xy[1] + b[1], x + w, xy[1] + b[3])
        self.blocks.append((box, fg, what, text))

    def check(self):
        for box, fg, what, text in self.blocks:
            x0, y0, x1, y1 = [int(round(v)) for v in box]
            x0, y0 = max(0, x0), max(0, y0)
            x1, y1 = min(self.base.width, x1), min(self.base.height, y1)
            if x1 <= x0 or y1 <= y0:
                continue
            region = self.base.crop((x0, y0, x1, y1))
            colours = region.getcolors(region.width * region.height) or []
            total = sum(n for n, _ in colours)
            worst = min(brand.contrast(fg, c) for n, c in colours if n >= 0.03 * total) if colours else 21
            CHECKED[0] += 1
            if worst < 4.5:
                LOW.append((self.name, what, text, round(worst, 2)))

    def done(self):
        self.check()
        return self.img.convert("RGB")


# -- Pieces -----------------------------------------------------------------------------------


NAMES = ["HybridX Race", "X Run", "X Trail", "X Ride", "X Workout", "X Streak", "UNA app",
         "300 m", "3 km", "25 m", "100 m", "1 km", "5 km", "10 runs", "4 weeks"]


def keep(text):
    """Names and numbers with their units stay on one line (a no-break space inside them)."""
    for n in NAMES:
        text = text.replace(n, n.replace(" ", "\u00a0"))
    return text


def wrap(d, text, font, width):
    """Lines no wider than @p width, never ending on a lone word if a narrower measure avoids it."""
    lines = _wrap(d, keep(text), font, width)
    w = width
    while len(lines) > 1 and len(lines[-1].split()) < 2 and w > width * 0.7:
        w -= 10
        trial = _wrap(d, keep(text), font, w)
        if len(trial) > len(lines):
            break
        lines = trial
    return lines


def _wrap(d, text, font, width):
    words, lines, cur = text.split(" "), [], ""
    for w in words:
        t = (cur + " " + w).strip()
        if d.textlength(t, font=font) <= width:
            cur = t
        else:
            lines.append(cur)
            cur = w
    if cur:
        lines.append(cur)
    return lines


def footer(c, s, x, y, size=28):
    """HYBRIDX.CLUB with the X, at the foot."""
    xm = brand.xmark(size, s["text"])
    c.img.alpha_composite(xm, (x, y - size))
    c.put((x + xm.width + 14, y), "HYBRIDX.CLUB", grotesk(size, 500), s["text"], "ls", 0.04, "wordmark")


def headline(c, x, y, lines, max_w, max_size, colour):
    size = brand.fit_anton(c.d, lines, max_w, max_size)
    f = anton(size)
    cap = f.getbbox("H")[3] - f.getbbox("H")[1]
    lead = int(cap * 1.18)
    yy = y + cap
    for l in lines:
        c.put((x, yy), l, f, colour, "ls", what="headline")
        yy += lead
    return yy - lead + int(cap * 0.1), size


def tag(c, xy, text, size, colour, anchor="la"):
    """An uppercase label: Inter 600, 0.12 em (the brand's one tracking value)."""
    c.put(xy, text.upper(), inter(size, "SemiBold"), colour, anchor, 0.12, "label")


def big_x(img, colour, height, left, cy):
    """HybridX's X, @p height px tall, its left edge at @p left: on a card it starts right of
    the text column, so its chevron shows behind the watch and the rest runs off the edge."""
    xm = brand.xmark(height, colour)
    img.alpha_composite(xm, (int(left), int(cy - xm.height / 2)))


def band(img, colour, cx, width):
    """A band at the X's 45 degrees through @p cx, the card's full height."""
    d = ImageDraw.Draw(img)
    h = img.height
    d.polygon([(cx - width / 2 - h / 2, h), (cx + width / 2 - h / 2, h),
               (cx + width / 2 + h / 2, 0), (cx - width / 2 + h / 2, 0)], fill=colour)


def icon_on(img, kind_file, size, xy, ground, ring):
    """An icon at @p xy; if its disc is close to the @p ground, a thin ring in @p ring shows its edge."""
    ic = Image.open(os.path.join(ICONS, kind_file)).convert("RGBA").resize((size, size), Image.LANCZOS)
    disc = ic.getpixel((size // 2, size // 10))[:3]
    if sum(abs(a - b) for a, b in zip(disc, ground)) < 120:
        d = ImageDraw.Draw(img)
        w = max(3, size // 45)
        d.ellipse([xy[0] - w, xy[1] - w, xy[0] + size + w - 1, xy[1] + size + w - 1], outline=ring, width=w)
    img.alpha_composite(ic, (int(xy[0]), int(xy[1])))


def place_watch(img, scr, colour, height, cx, cy):
    w = brand.watch(scr, height, colour)
    # The render's face centre is at 49.9 % across and 50.7 % down (brand.HOLE_* in CROP).
    fx = (brand.HOLE_CX - brand.CROP[0]) / (brand.CROP[2] - brand.CROP[0])
    fy = (brand.HOLE_CY - brand.CROP[1]) / (brand.CROP[3] - brand.CROP[1])
    img.alpha_composite(w, (int(cx - fx * w.width), int(cy - fy * w.height)))


# -- Cards -----------------------------------------------------------------------------------
# The layout: text in the left column (x 72 to 540 on a square, 96 to 900 on a banner), the
# watch on the right, and HybridX's X behind the watch, its left edge right of the text, so
# no text ever sits on it (Card.check() measures the pixels to be sure).


def square(cw, kind, number, card, app_dir, name, listing_name):
    lines, sub, scr_name, light = card
    s = scheme(cw, light)
    c = Card(1080, 1080, s["ground"], name)
    big_x(c.img, s["x"], 1240, 560, 540)
    place_watch(c.img, brand.screen(app_dir, scr_name), cw["watch"], 900, 800, 560)
    c.snapshot()
    M = 72
    tag(c, (M, M), "%02d  ·  %s" % (number, listing_name), 24, s["label"])
    y, size = headline(c, M, M + 64, lines, 400, 150, s["text"])
    f = grotesk(34, 500)
    yy = y + 40
    for l in wrap(c.d, sub, f, 380):
        c.put((M, yy), l, f, s["sub"], "la", what="sub")
        yy += 46
    footer(c, s, M, 1080 - M)
    return c.done(), size


def family_card(cw, name, light=False):
    s = scheme(cw, light)
    c = Card(1080, 1080, s["ground"], name)
    c.snapshot()
    M = 72
    tag(c, (M, M), "Part of the family", 24, s["label"])
    y, _ = headline(c, M, M + 64, ["THE X FAMILY"], 936, 150, s["text"])
    size = 190
    gap_x = (1080 - 2 * M - 3 * size) / 2
    top = y + 70
    for i, k in enumerate(brand.FAMILY):
        col, row = i % 3, i // 3
        x = M + col * (size + gap_x)
        yy = top + row * (size + 92)
        icon_on(c.img, k + "-1024.png", size, (int(x), int(yy)), s["ground"], s["text"])
        tag(c, (x + size / 2, yy + size + 18), FAMILY_NAMES[k], 22, s["text"], "ma")
    c.put((M, 1080 - M - 80), "Every session counts in X Streak.", grotesk(34, 500), s["sub"], "ls",
          what="family line")
    footer(c, s, M, 1080 - M)
    return c.done()


def banner(cw, kind, listing, app_dir, glance, name):
    s = scheme(cw, False)
    W, H = 1920, 1080
    c = Card(W, H, s["ground"], name)
    big_x(c.img, s["x"], 1500, 960, 540)
    place_watch(c.img, brand.screen(app_dir, listing["hero"]), cw["watch"], 1020, 1400, 545)
    c.snapshot()
    M = 96
    tag(c, (M, M), listing["kicker"], 26, s["label"])
    y, _ = headline(c, M, M + 70, [listing["title"]], 760, 250, s["text"])
    f = grotesk(64, 700)
    yy = y + 56
    for l in wrap(c.d, listing["tagline"], f, 760):
        c.put((M, yy), l, f, s["text"], "la", what="tagline")
        yy += 78
    f = inter(32, "Regular")
    yy += 18
    for l in wrap(c.d, listing["body"], f, 700):
        c.put((M, yy), l, f, s["sub"], "la", what="body")
        yy += 46
    ic_name = kind + ("-glance" if glance else "") + "-1024.png"
    icon_on(c.img, ic_name, 120, (M, H - M - 120 - 70), s["ground"], s["sub"])
    if glance:
        badge = "Needs " + listing["needs"]
    elif kind == "streak":
        badge = "For the X family"
    else:
        badge = "Counts in X Streak"
    tag(c, (M + 146, H - M - 120 - 70 + 46), badge, 24, s["text"])
    footer(c, s, M, H - M)
    return c.done()


def family_banner(name):
    """The whole family: six columns, each in its app's colour, its icon and its name."""
    W, H = 1920, 1080
    c = Card(W, H, (0, 0, 0), name)
    d = ImageDraw.Draw(c.img)
    cols = len(brand.FAMILY)
    cw_ = W / cols
    for i, k in enumerate(brand.FAMILY):
        cw = COLOURWAYS[k]
        x0 = int(i * cw_)
        col = cw["dark"] if k != "race" else (26, 26, 26)
        d.rectangle([x0, 330, int((i + 1) * cw_), H], fill=col)
        icon_on(c.img, k + "-1024.png", 220, (int(x0 + cw_ / 2 - 110), 470), col, cw["mute"])
    c.snapshot()
    for i, k in enumerate(brand.FAMILY):
        cw = COLOURWAYS[k]
        x0 = i * cw_
        tag(c, (x0 + cw_ / 2, 740), FAMILY_NAMES[k], 26, (255, 255, 255), "ma")
        tag(c, (x0 + cw_ / 2, 790), cw["name"], 22, cw["mute"] if k != "race" else cw["accent"], "ma")
    headline(c, 96, 96, ["THE X FAMILY"], 1100, 170, (255, 255, 255))
    c.put((1000, 96 + 130), "Six apps for UNA Watch.", grotesk(40, 500), COLOURWAYS["race"]["accent"],
          "ls", what="family tag")
    c.put((1000, 96 + 180), "Every session counts in X Streak.", grotesk(40, 500), (255, 255, 255),
          "ls", what="family tag")
    s = {"text": (255, 255, 255), "ground": (0, 0, 0)}
    footer(c, s, 96, H - 60)
    return c.done()


def main():
    for key, listing in LISTINGS.items():
        kind = listing.get("kind", key)
        glance = listing.get("glance", False)
        cw = COLOURWAYS[kind]
        app_dir = cw["folder"]
        out = os.path.join(ROOT, app_dir, "docs", "store", "glance" if glance else "")
        os.makedirs(out, exist_ok=True)
        for f in os.listdir(out):
            if f.endswith(".png") and f[:2].isdigit():
                os.remove(os.path.join(out, f))
        banner(cw, kind, listing, app_dir, glance, key + " banner").save(os.path.join(out, "01-banner.png"))
        n = 2
        for i, card in enumerate(listing["cards"], start=1):
            slug = "-".join(w.strip(".,?'").lower() for w in " ".join(card[0]).split())[:40].strip("-")
            img, size = square(cw, kind, i, card, app_dir, "%s %s" % (key, slug),
                               listing.get("name", FAMILY_NAMES[kind]))
            img.save(os.path.join(out, "%02d-%s.png" % (n, slug)))
            n += 1
        if not glance:
            family_card(cw, key + " family", light=(kind != "race")).save(os.path.join(out, "%02d-the-x-family.png" % n))
        print("wrote", out)
    fam = os.path.join(ROOT, "hybridx-link", "docs", "brand")
    family_banner("family banner").save(os.path.join(fam, "family-banner.png"))
    print("wrote", os.path.join(fam, "family-banner.png"))

    print("contrast: %d text blocks checked against the pixels under them, %d under 4.5:1"
          % (CHECKED[0], len(LOW)))
    for b in LOW:
        print("  LOW", b)
    sys.exit(1 if LOW else 0)


if __name__ == "__main__":
    main()
