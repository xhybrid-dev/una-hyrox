#!/usr/bin/env python3
"""The Clock Probe's text-fit audit: is the widest text each row can show clear of the ring?

    python3 x-clock/docs/experiments/audit.py [build-tests/worst_rows]

Each row's worst-case text comes from worst_rows (built with the host tests). Its width is the sum of
the glyph advances in the generated LVGL font the screen uses (poppins_regular_14.c, read straight from
the file, so it is what the watch draws, less kerning, which is a pixel or two). It is measured against
the disc: the text starts at x = 42 and must end before the ring's inner edge, taken at the text's
outer corner (the row's top or bottom, whichever is further from the centre), less a margin.

The VO2max row is also held clear of its trend bars (up to ten, X Run's history, 4 px apart, ending at x = 204).
Exit status 1 if any row does not fit.
"""
import math
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
FONT = os.path.join(REPO, "Tools/Probe/Software/Apps/LVGL-GUI/assets/fonts/poppins_regular_14.c")
WORST = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "build-tests/worst_rows")

# The screen's layout (ProbeScreen.cpp).
ROWS_Y, ROW_STEP, TEXT_X = 60, 18, 42
LINE_H = 21            # Poppins 14's line height in LVGL
RING_INNER = 106       # the ring's centre line is 110 and it is 8 px thick
MARGIN = 4
BARS_LEFT = 204 - 10 * 4 + 1   # the left edge of the first of ten bars


def advances(path):
    """Advance widths (px) of printable ASCII from an lv_font_conv C file: glyph 0 is the font's
    placeholder, then U+0020 upward in order; .adv_w is in 1/16 px."""
    text = open(path, encoding="utf-8").read()
    start = text.index("glyph_dsc[]")
    end = text.index("};", start)
    adv = [int(m) / 16.0 for m in re.findall(r"\.adv_w\s*=\s*(\d+)", text[start:end])]
    return {chr(0x20 + i - 1): w for i, w in enumerate(adv) if i > 0}


def width(adv, s):
    return sum(adv[c] for c in s)


def limit(row):
    """The right-most x a row's text may reach."""
    top = ROWS_Y + row * ROW_STEP
    far = max(abs(top - 120), abs(top + LINE_H - 120))
    room = (RING_INNER - MARGIN) ** 2 - far ** 2
    return 120 + math.sqrt(room) if room > 0 else 0


def main():
    adv = advances(FONT)
    rows = subprocess.run([WORST], capture_output=True, text=True, check=True).stdout.splitlines()
    bad = 0
    print(f"{'row':>3}  {'text':<26} {'width':>6} {'ends at':>8} {'limit':>6}  verdict")
    for line in rows:
        n, text = line.split(" ", 1)
        row = int(n)
        w = width(adv, text)
        ends = TEXT_X + w
        lim = limit(row)
        if row == 6:
            lim = min(lim, BARS_LEFT - 3)
        ok = ends <= lim
        bad += 0 if ok else 1
        print(f"{row:>3}  {text:<26} {w:>6.1f} {ends:>8.1f} {lim:>6.1f}  {'ok' if ok else 'TOO WIDE'}")
    print()
    print("every row fits" if not bad else f"{bad} text(s) do not fit")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
