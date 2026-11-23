#!/usr/bin/env python3
"""HybridX's blade X (hybridx-link/Tools/Brand/assets/x-mark.png, the brand's "Icon Logo.png")
traced to a vector outline for the 3D lockup: marching squares on the alpha at half coverage,
then simplified (Ramer-Douglas-Peucker, 0.2 px). Writes public/gen/x-mark.svg, its viewBox the
mark's own bounding box.

    python3 scripts/trace-xmark.py
"""

import os

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', '..', '..', 'hybridx-link', 'Tools', 'Brand', 'assets', 'x-mark.png')
OUT = os.path.join(HERE, '..', 'public', 'gen', 'x-mark.svg')


def contours(a, level=0.5):
    """Closed iso-lines of a 2D field, as lists of (x, y), by marching squares."""
    h, w = a.shape
    p = np.zeros((h + 2, w + 2))
    p[1:-1, 1:-1] = a
    segs = {}

    def interp(y0, x0, y1, x1):
        v0, v1 = p[y0, x0], p[y1, x1]
        t = (level - v0) / (v1 - v0)
        return (x0 + (x1 - x0) * t - 1, y0 + (y1 - y0) * t - 1)

    for y in range(h + 1):
        for x in range(w + 1):
            tl, tr, br, bl = p[y, x] > level, p[y, x + 1] > level, p[y + 1, x + 1] > level, p[y + 1, x] > level
            idx = tl * 8 + tr * 4 + br * 2 + bl
            if idx in (0, 15):
                continue
            top = ((y, x), (y, x + 1))
            right = ((y, x + 1), (y + 1, x + 1))
            bottom = ((y + 1, x + 1), (y + 1, x))
            left = ((y + 1, x), (y, x))
            table = {
                1: [(left, bottom)], 2: [(bottom, right)], 3: [(left, right)], 4: [(right, top)],
                5: [(left, top), (right, bottom)], 6: [(bottom, top)], 7: [(left, top)],
                8: [(top, left)], 9: [(top, bottom)], 10: [(top, right), (bottom, left)],
                11: [(top, right)], 12: [(right, left)], 13: [(right, bottom)], 14: [(bottom, left)],
            }
            for e0, e1 in table[idx]:
                a0 = interp(*e0[0], *e0[1])
                a1 = interp(*e1[0], *e1[1])
                k0 = (round(a0[0], 4), round(a0[1], 4))
                k1 = (round(a1[0], 4), round(a1[1], 4))
                segs[k0] = k1
    loops = []
    while segs:
        start, nxt = segs.popitem()
        loop = [start]
        cur = nxt
        while cur != start and cur in segs:
            loop.append(cur)
            cur = segs.pop(cur)
        if len(loop) > 8:
            loops.append(loop)
    return loops


def rdp(pts, eps):
    if len(pts) < 3:
        return pts
    a, b = np.array(pts[0]), np.array(pts[-1])
    d = b - a
    n = np.hypot(*d) or 1.0
    dist = [abs(d[0] * (a[1] - p[1]) - d[1] * (a[0] - p[0])) / n for p in pts[1:-1]]
    i = int(np.argmax(dist)) + 1
    if dist[i - 1] > eps:
        return rdp(pts[:i + 1], eps)[:-1] + rdp(pts[i:], eps)
    return [pts[0], pts[-1]]


def main():
    im = Image.open(SRC).convert('RGBA')
    a = np.array(im)[:, :, 3].astype(float) / 255.0
    loops = contours(a)
    ys, xs = np.nonzero(a > 0.5)
    x0, y0, x1, y1 = xs.min(), ys.min(), xs.max() + 1, ys.max() + 1
    paths = []
    for loop in loops:
        # Simplify a closed loop: split at its farthest pair so RDP has two ends.
        k = len(loop) // 2
        s = rdp(loop[:k + 1], 0.2)[:-1] + rdp(loop[k:] + [loop[0]], 0.2)[:-1]
        d = 'M' + ' L'.join(f'{x - x0:.2f} {y - y0:.2f}' for x, y in s) + ' Z'
        paths.append(d)
    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {x1 - x0} {y1 - y0}">'
           f'<path fill="#000" fill-rule="evenodd" d="{" ".join(paths)}"/></svg>\n')
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'w') as f:
        f.write(svg)
    print(f'trace-xmark: {len(loops)} loops, {sum(p.count("L") + 1 for p in paths)} points, '
          f'{x1 - x0} x {y1 - y0}')


if __name__ == '__main__':
    main()
