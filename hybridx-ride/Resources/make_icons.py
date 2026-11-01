#!/usr/bin/env python3
"""Draw HybridX Ride launcher icons: a bicycle, teal frame and white wheels.

Writes icon_60x60.png and icon_30x30.png next to this script.
Uses supersampling, box-filter downsampling and the display's
four levels per channel for crisp, consistent rendering.

    python3 make_icons.py        (needs Pillow)
"""

from pathlib import Path
from PIL import Image, ImageDraw

# Display palette: 2 bits per channel
TEAL = (0, 170, 170, 255)
TEAL_DARK = (0, 85, 85, 255)
WHITE = (255, 255, 255, 255)

# Darkest available grey on a 2-bit-per-channel display: level 1 of 4 on
# each channel, the SDK's SDK::GUI::Color::GRAY_DARK (0x404040 on its scale).
GRAY_DARK = (85, 85, 85, 255)

OUT = Path(__file__).resolve().parent
SCALE = 4


def draw_icon(size):
    """Draw the icon at 4x resolution and reduce to the target size."""
    s = size * SCALE
    k = s / 60.0

    def p(v):
        return round(v * k)

    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # Dark grey circular background; corners remain transparent.
    d.ellipse(
        [p(2), p(2), p(58), p(58)],
        fill=GRAY_DARK,
    )

    def line(points, width, colour):
        """A stroke with round ends, in the 60-unit grid."""
        pts = [(p(x), p(y)) for x, y in points]
        d.line(pts, fill=colour, width=p(width), joint="curve")
        r = p(width) / 2
        for x, y in (pts[0], pts[-1]):
            d.ellipse([x - r, y - r, x + r, y + r], fill=colour)

    def ring(cx, cy, radius, width, colour):
        d.ellipse([p(cx - radius), p(cy - radius), p(cx + radius), p(cy + radius)], outline=colour, width=p(width))

    # Two wheels, white.
    rear, front = (18, 37), (42, 37)
    ring(rear[0], rear[1], 10.5, 2.6, WHITE)
    ring(front[0], front[1], 10.5, 2.6, WHITE)

    # The frame, teal: rear hub, bottom bracket, seat, head, and the fork to the front hub.
    bb, seat, head = (30, 37), (25, 23), (38, 23)
    line([rear, bb, seat, rear], 2.6, TEAL)
    line([seat, head, bb], 2.6, TEAL)
    line([head, front], 2.6, TEAL)

    # The saddle and the handlebar, white; the hubs, dark.
    line([(21, 19.5), (28, 19.5)], 2.4, WHITE)
    line([(38, 23), (36.5, 18), (41, 17)], 2.4, WHITE)
    for cx, cy in (rear, front, bb):
        d.ellipse([p(cx - 1.6), p(cy - 1.6), p(cx + 1.6), p(cy + 1.6)], fill=TEAL_DARK)

    # Downsample for clean edges at the target resolution.
    img = img.resize((size, size), Image.Resampling.BOX)

    # Snap RGB channels to the display's 0, 85, 170, 255 levels.
    # Preserve alpha so the outside corners remain transparent.
    pixels = img.load()

    for y in range(size):
        for x in range(size):
            r, g, b, a = pixels[x, y]

            def snap(c):
                return min(255, round(c / 85) * 85)

            pixels[x, y] = (
                snap(r),
                snap(g),
                snap(b),
                snap(a),
            )

    return img


if __name__ == "__main__":
    for size in (60, 30):
        icon = draw_icon(size)
        path = OUT / f"icon_{size}x{size}.png"
        icon.save(path)
        print(f"Created {path}")
