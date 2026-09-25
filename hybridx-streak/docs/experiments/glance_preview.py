#!/usr/bin/env python3
"""Draw the HybridX Streak glance as the layout code specifies it.

The PC simulator cannot run glances, so this draws the controls that
Tests/Host/tools/glance_preview prints -- the real GlanceLayout code's output --
at 2x, in the glance's colours (GlanceControl.h). The face is DejaVu Sans
standing in for Poppins: positions and colours are exact, letter shapes are
not. A mock-up until the watch shows the real thing.

    glance_preview.py <glance_preview binary> <out.png> [w h controls]
"""
import subprocess
import sys

from PIL import Image, ImageDraw, ImageFont

# GlanceControl.h colour codes -> RGB (the display's 2-bit levels, x85).
COLOURS = {0x3F: (255, 255, 255), 0x2A: (170, 170, 170), 0x15: (85, 85, 85), 0x00: (0, 0, 0),
           0x38: (255, 170, 0), 0x0A: (0, 170, 170), 0x05: (0, 85, 85), 0x0C: (0, 255, 0),
           0x04: (0, 85, 0), 0x39: (255, 170, 85), 0x30: (255, 0, 0), 0x10: (85, 0, 0),
           0x03: (0, 0, 255), 0x01: (0, 0, 85), 0x1F: (85, 255, 255), 0x2F: (170, 255, 255)}
# GlanceFont_t order: REGULAR_18, MEDIUM_10, MEDIUM_18, MEDIUM_25, SEMIBOLD_18, SEMIBOLD_20, ...
SIZES = {0: (18, False), 1: (10, False), 2: (18, False), 3: (25, False), 4: (18, True), 5: (20, True),
         6: (25, True), 7: (30, True), 8: (35, True)}
S = 2  # scale

binary, out = sys.argv[1], sys.argv[2]
args = sys.argv[3:6] if len(sys.argv) > 5 else ["240", "60", "32"]
lines = subprocess.run([binary] + args, capture_output=True, text=True, check=True).stdout.splitlines()

panels = []
for line in lines:
    parts = line.split(" ")
    if parts[0] == "state":
        name = " ".join(parts[1:-2])
        w, h = int(parts[-2]), int(parts[-1])
        img = Image.new("RGB", (w * S, h * S), (0, 0, 0))
        panels.append((name, img, ImageDraw.Draw(img)))
        continue
    _, img, d = panels[-1]
    if parts[0] == "line":
        x1, y1, x2, y2, c = map(int, parts[1:6])
        d.line([(x1 * S, y1 * S), (x2 * S, y2 * S)], fill=COLOURS.get(c, (255, 0, 255)), width=S)
    elif parts[0] == "rect":
        x, y, w, h, c = map(int, parts[1:6])
        d.rectangle([x * S, y * S, (x + w) * S - 1, (y + h) * S - 1], fill=COLOURS.get(c, (255, 0, 255)))
    elif parts[0] == "text":
        x, y, w, h, f, c, a = map(int, parts[1:8])
        text = " ".join(parts[8:])
        size, bold = SIZES.get(f, (18, False))
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans%s.ttf" % ("-Bold" if bold else ""),
                                  int(size * S * 0.82))
        tw = d.textlength(text, font=font)
        tx = x * S if a == 0 else (x * S + (w * S - tw) / 2 if a == 1 else x * S + w * S - tw)
        d.text((tx, y * S), text, font=font, fill=COLOURS.get(c, (255, 0, 255)))

label_font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 22)
pad, gap = 24, 56
W = max(max(p[1].width for p in panels) + 2 * pad, 720)
H = sum(p[1].height + gap for p in panels) + pad + 40
sheet = Image.new("RGB", (W, H), (27, 31, 34))
sd = ImageDraw.Draw(sheet)
sd.text((pad, 14), "HybridX Streak glance - mock-up (layout exact, font stand-in)", font=label_font,
        fill=(170, 255, 0))
y = 40 + pad
for name, img, _ in panels:
    sd.text((pad, y), name, font=label_font, fill=(180, 186, 190))
    sheet.paste(img, (pad, y + 28))
    sd.rectangle([pad - 1, y + 27, pad + img.width, y + 28 + img.height], outline=(58, 64, 68))
    y += img.height + gap
sheet.save(out)
print("wrote", out)
