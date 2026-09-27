#!/usr/bin/env python3
"""Extract UNA's watch renders from the SDK's Figma UI Resource Pack.

The pack (una-sdk/Docs/Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig)
is a zip; its images/ folder holds 2000 x 2000 renders of the watch with the
screen cut out as transparency. They are UNA's artwork and carry UNA's logo,
so they are read from the SDK at render time and cached in promo/out/una/,
never committed here.

For each strap colour this crops to the watch, softens the cut ends of the
strap so they fade out, and records where the screen opening is.

    python3 una_mockups.py <una-sdk> <out-dir>
"""

import io
import json
import os
import sys
import zipfile

import numpy as np
from PIL import Image

# Image names inside the pack, by strap colour (checked by eye).
RENDERS = {
    'graphite': '7fcc1a77ec68190d68598271ef7001237214c36d',
    'teal': '4ae4be5e2931ad615f699b142316f5a8f66563f5',
    'white': '8a9fead257a6c70ffc99d9fe6f3c0ba3fc144d36',
}


def screen_hole(alpha):
    """Centre and radius of the transparent opening round the image centre."""
    h, w = alpha.shape
    cy, cx = h // 2, w // 2

    def run(dy, dx):
        y, x, k = cy, cx, 0
        while 0 <= y < h and 0 <= x < w and alpha[y, x] < 128:
            y, x, k = y + dy, x + dx, k + 1
        return k

    r, l, d, u = run(0, 1), run(0, -1), run(1, 0), run(-1, 0)
    return cx + (r - l) / 2, cy + (d - u) / 2, (r + l + d + u) / 4


def main(sdk, out):
    fig = os.path.join(sdk, 'Docs', 'Templates', 'Figma-UI-Kit', 'UNA-Watch-UI-Resource-Pack.fig')
    os.makedirs(out, exist_ok=True)
    meta = {}
    with zipfile.ZipFile(fig) as z:
        for name, key in RENDERS.items():
            im = Image.open(io.BytesIO(z.read('images/' + key))).convert('RGBA')
            a = np.array(im)
            cx, cy, r = screen_hole(a[:, :, 3])
            x0, y0, x1, y1 = im.getbbox()
            im = im.crop((x0, y0, x1, y1))
            a = np.array(im).astype(np.float32)
            # Fade the strap's cut ends over the outer 14% top and bottom.
            hh = a.shape[0]
            ramp = np.ones(hh, np.float32)
            n = int(hh * 0.14)
            ramp[:n] = np.linspace(0, 1, n) ** 1.6
            ramp[-n:] = np.linspace(1, 0, n) ** 1.6
            a[:, :, 3] *= ramp[:, None]
            Image.fromarray(a.clip(0, 255).astype(np.uint8)).save(os.path.join(out, f'{name}.png'))
            meta[name] = {'cx': cx - x0, 'cy': cy - y0, 'r': r, 'w': x1 - x0, 'h': y1 - y0}
            print(f'{name}: screen centre ({cx - x0:.1f}, {cy - y0:.1f}) radius {r:.1f}, image {x1 - x0}x{y1 - y0}')
    with open(os.path.join(out, 'meta.json'), 'w') as f:
        json.dump(meta, f, indent=1)


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
