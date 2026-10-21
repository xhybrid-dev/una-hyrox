#!/usr/bin/env python3
"""Draw Trail Quest's launcher icons: the runner meets the Red Fox (option 2, "The duel",
chosen by Jon on 21 October 2026 from the five in docs/icons/).

The game's own runner and Red Fox sprites (Tools/Art/gen_art.py) face each other on the
grass under a blue sky, with a yellow-and-white clash star between them. Writes
icon_60x60.png and icon_30x30.png next to this script.

The watch build's app_merging.py requires exactly those sizes, square, RGBA, and
converts them to ABGR2222 (two bits per channel, alpha included), so the icon is drawn
the way it will be shown: flat colours from the display's 64, hard edges, and a
transparent background like the SDK's own app icons. Drawn at 4x and reduced with a
box filter, then snapped to the display's four levels per channel so what you see here
is what the watch shows. (The same method as X Weigh-in's and Knitting's make_icons.py.)

The other four options are drawn by make_icon_options.py, for reference.

    python3 make_icons.py        (needs Pillow)
"""

import os

import make_icon_options as options

HERE = os.path.dirname(os.path.abspath(__file__))

art = options.option_duel()
for n in (60, 30):
    out = os.path.join(HERE, "icon_%dx%d.png" % (n, n))
    art.finish(n).save(out)
    print("wrote", out)
