#!/usr/bin/env python3
"""UNA's watch art, read from the SDK's Figma UI Resource Pack into public/gen/una/.

The pack (una-sdk/Docs/Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig) is a zip whose
images/ folder holds UNA's renders. They carry UNA's logo, so, as promo/tools/una_mockups.py does,
they are read from the SDK at build time and never committed here.

Writes:
  bezel-ring.png    the printed inner bezel (UP, DOWN, START SELECT STOP, BACK, UNA, ticks),
                    2048 px, its hole cut out; meta.json gives the inner radius as a fraction
  una-wordmark.png  UNA's white wordmark, 1600 px wide
  ref-graphite.png  the graphite watch from the front, cropped, for checking the 3D model
  meta.json

    python3 scripts/extract-una.py          (UNA_SDK, or ../../una-sdk)
"""

import io
import json
import os
import sys
import zipfile

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, '..', 'public', 'gen', 'una')

# Image names inside the pack (checked by eye; promo/tools/una_mockups.py names the renders).
RING = '41c317d3a2701d20f60c18ad0d81978651fc8a32'      # 3042 px bezel print
WORDMARK = 'dab12a846023edfef71bdacc57d515ddcc765da8'  # 4096 x 1372 white UNA
GRAPHITE = '7fcc1a77ec68190d68598271ef7001237214c36d'  # 2000 px graphite watch, front


def sdk_path():
    env = os.environ.get('UNA_SDK')
    if env:
        return env
    return os.path.abspath(os.path.join(HERE, '..', '..', '..', 'una-sdk'))


def hole(alpha, cx, cy):
    """Centre and radius of the transparent opening round (cx, cy), from four runs."""
    h, w = alpha.shape

    def run(dy, dx):
        y, x, k = cy, cx, 0
        while 0 <= y < h and 0 <= x < w and alpha[y, x] < 128:
            y, x, k = y + dy, x + dx, k + 1
        return k

    r, l, d, u = run(0, 1), run(0, -1), run(1, 0), run(-1, 0)
    return cx + (r - l) / 2, cy + (d - u) / 2, (r + l + d + u) / 4


def main():
    fig = os.path.join(sdk_path(), 'Docs', 'Templates', 'Figma-UI-Kit', 'UNA-Watch-UI-Resource-Pack.fig')
    if not os.path.exists(fig):
        sys.exit(f'extract-una: the SDK Figma pack is not at {fig}; set UNA_SDK')
    os.makedirs(OUT, exist_ok=True)
    meta = {}
    with zipfile.ZipFile(fig) as z:
        def load(key):
            return Image.open(io.BytesIO(z.read('images/' + key))).convert('RGBA')

        ring = load(RING)
        a = np.array(ring)
        _, _, r_in = hole(a[:, :, 3], ring.width // 2, ring.height // 2)
        meta['ringInner'] = r_in / (ring.width / 2)
        ring.resize((2048, 2048), Image.LANCZOS).save(os.path.join(OUT, 'bezel-ring.png'))

        mark = load(WORDMARK)
        mark = mark.crop(mark.getbbox())
        w = 1600
        mark = mark.resize((w, round(mark.height * w / mark.width)), Image.LANCZOS)
        mark.save(os.path.join(OUT, 'una-wordmark.png'))
        meta['wordmarkAspect'] = mark.width / mark.height

        g = load(GRAPHITE)
        ga = np.array(g)
        sx, sy, r_screen = hole(ga[:, :, 3], g.width // 2, g.height // 2)
        x0, y0, x1, y1 = g.getbbox()
        g = g.crop((x0, y0, x1, y1))
        g.save(os.path.join(OUT, 'ref-graphite.png'))
        meta['ref'] = {'cx': sx - x0, 'cy': sy - y0, 'r': r_screen, 'w': g.width, 'h': g.height}

    with open(os.path.join(OUT, 'meta.json'), 'w') as f:
        json.dump(meta, f, indent=1)
    print('extract-una:', json.dumps(meta))


if __name__ == '__main__':
    main()
