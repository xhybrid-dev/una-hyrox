#!/usr/bin/env python3
"""UNA's watch art, read from the SDK's Figma UI Resource Pack into public/gen/una/.

The pack (una-sdk/Docs/Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig) is a zip whose
images/ folder holds UNA's renders. They carry UNA's logo, so, as promo/tools/una_mockups.py does,
they are read from the SDK at build time and never committed here.

The film's watch is UNA's CAD housing (scripts/una-cad.mjs) with UNA's front render projected on its
face: the graphite or the white watch from the front, its screen opening transparent.

Writes:
  watch-graphite.png  the graphite watch, cropped to the watch with a clear margin
  watch-white.png     the white watch, cropped the same way (the same watch: the same opening)
  una-wordmark.png    UNA's white wordmark, 1600 px wide
  meta.json           where the screen opening is (centre and radius, px), the image's size, the
                      wordmark's aspect, and each button's outer edge (angle in TouchGFX degrees,
                      0 at 12 o'clock and clockwise, and distance in screen radii)

    python3 scripts/extract-una.py          (UNA_SDK, or ../../una-sdk)
"""

import io
import json
import math
import os
import sys
import zipfile

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, '..', 'public', 'gen', 'una')

# Image names inside the pack (checked by eye; promo/tools/una_mockups.py names the renders).
WORDMARK = 'dab12a846023edfef71bdacc57d515ddcc765da8'  # 4096 x 1372 white UNA
GRAPHITE = '7fcc1a77ec68190d68598271ef7001237214c36d'  # 2000 px graphite watch, front
WHITE = '8a9fead257a6c70ffc99d9fe6f3c0ba3fc144d36'  # 2000 px white watch, front

# The SDK's button hint angles (screen/lvgl.ts BTN_ANGLE); each button is looked for within 15 degrees.
BUTTONS = {'l1': 299.5, 'l2': 240.5, 'r1': 60.5, 'r2': 120.0}
MARGIN = 24  # px of clear border, so the film's edge light can sample past the outline


def sdk_path():
    env = os.environ.get('UNA_SDK')
    if env:
        return env
    return os.path.abspath(os.path.join(HERE, '..', '..', '..', 'una-sdk'))


def fit_circle(px, py):
    a = np.c_[2 * px, 2 * py, np.ones_like(px)]
    c = np.linalg.lstsq(a, px ** 2 + py ** 2, rcond=None)[0]
    return c[0], c[1], math.sqrt(c[2] + c[0] ** 2 + c[1] ** 2)


def screen_opening(alpha):
    """The transparent circle in the middle: a circle fitted to its edge, the bezel's ticks left out."""
    h, w = alpha.shape
    clear = alpha < 128
    yy, xx = np.mgrid[0:h, 0:w]
    inside = clear & ((xx - w / 2) ** 2 + (yy - h / 2) ** 2 < (0.21 * w) ** 2)
    edge = inside & ~(np.roll(clear, 1, 0) & np.roll(clear, -1, 0) & np.roll(clear, 1, 1) & np.roll(clear, -1, 1))
    py, px = (v.astype(float) for v in np.nonzero(edge))
    cx, cy, r = fit_circle(px, py)
    for _ in range(3):
        keep = np.abs(np.hypot(px - cx, py - cy) - r) < 3
        cx, cy, r = fit_circle(px[keep], py[keep])
    return cx, cy, r


def outline_radius(alpha, cx, cy, r, deg):
    """How far out the watch reaches along a direction (TouchGFX degrees), in screen radii."""
    th = math.radians(90 - deg)
    dx, dy = math.cos(th), -math.sin(th)
    h, w = alpha.shape
    last = 0.0
    for s in np.arange(r, 3 * r, 0.5):
        x, y = int(round(cx + dx * s)), int(round(cy + dy * s))
        if 0 <= x < w and 0 <= y < h and alpha[y, x] >= 128:
            last = s
    return last / r


def buttons(alpha, cx, cy, r):
    """Each button's centre angle and outer edge: the raised stretch of the outline near its hint angle."""
    out = {}
    for name, hint in BUTTONS.items():
        degs = np.arange(hint - 15, hint + 15.01, 0.5)
        prof = np.array([outline_radius(alpha, cx, cy, r, d) for d in degs])
        lo, hi = prof.min(), prof.max()
        raised = prof > lo + 0.6 * (hi - lo)
        out[name] = {'deg': round(float(degs[raised].mean()), 2), 'r': round(float(np.median(prof[raised])), 4)}
    return out


def bleed(im, steps=32):
    """Gives the fully transparent pixels the colour of their nearest visible ones. They are stored
    white, and texture filtering and mipmaps would mix that white into the outline as a light fringe."""
    arr = np.array(im).astype(np.float32)
    rgb, known = arr[:, :, :3], arr[:, :, 3] > 0
    for _ in range(steps):
        acc = np.zeros_like(rgb)
        n = np.zeros(known.shape, np.float32)
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if dy or dx:
                    k = np.roll(np.roll(known, dy, 0), dx, 1)
                    acc += np.roll(np.roll(rgb, dy, 0), dx, 1) * k[:, :, None]
                    n += k
        grow = ~known & (n > 0)
        rgb[grow] = acc[grow] / n[grow][:, None]
        known = known | grow
    rgb[~known] = rgb[arr[:, :, 3] == 255].mean(0)
    arr[:, :, :3] = rgb
    return Image.fromarray(arr.round().clip(0, 255).astype(np.uint8), 'RGBA')


def main():
    fig = os.path.join(sdk_path(), 'Docs', 'Templates', 'Figma-UI-Kit', 'UNA-Watch-UI-Resource-Pack.fig')
    if not os.path.exists(fig):
        sys.exit(f'extract-una: the SDK Figma pack is not at {fig}; set UNA_SDK')
    os.makedirs(OUT, exist_ok=True)
    meta = {}
    with zipfile.ZipFile(fig) as z:
        def load(key):
            return Image.open(io.BytesIO(z.read('images/' + key))).convert('RGBA')

        mark = load(WORDMARK)
        mark = mark.crop(mark.getbbox())
        w = 1600
        mark = mark.resize((w, round(mark.height * w / mark.width)), Image.LANCZOS)
        mark.save(os.path.join(OUT, 'una-wordmark.png'))
        meta['wordmarkAspect'] = mark.width / mark.height

        renders = {'graphite': load(GRAPHITE), 'white': load(WHITE)}
        alpha = np.array(renders['graphite'])[:, :, 3]
        cx, cy, r = screen_opening(alpha)
        wx, wy, wr = screen_opening(np.array(renders['white'])[:, :, 3])
        if max(abs(wx - cx), abs(wy - cy), abs(wr - r)) > 1.5:
            sys.exit(f"extract-una: the white render's opening ({wx:.1f}, {wy:.1f}, {wr:.1f}) is not the graphite's")
        meta['buttons'] = buttons(alpha, cx, cy, r)
        boxes = [im.getbbox() for im in renders.values()]
        x0, y0 = min(b[0] for b in boxes) - MARGIN, min(b[1] for b in boxes) - MARGIN
        x1, y1 = max(b[2] for b in boxes) + MARGIN, max(b[3] for b in boxes) + MARGIN
        for name, im in renders.items():
            bleed(im.crop((x0, y0, x1, y1))).save(os.path.join(OUT, f'watch-{name}.png'))  # PIL pads with transparency
        g = renders['graphite'].crop((x0, y0, x1, y1))
        meta['watch'] = {'cx': round(cx - x0, 2), 'cy': round(cy - y0, 2), 'r': round(r, 2), 'w': g.width, 'h': g.height}

    with open(os.path.join(OUT, 'meta.json'), 'w') as f:
        json.dump(meta, f, indent=1)
    print('extract-una:', json.dumps(meta))
    for old in ('bezel-ring.png', 'ref-graphite.png'):
        p = os.path.join(OUT, old)
        if os.path.exists(p):
            os.remove(p)


if __name__ == '__main__':
    main()
