#!/usr/bin/env python3
"""Draw HybridX Workout launcher icons: a teal dumbbell.

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

# Darkest available grey on a 2-bit-per-channel display.
GRAY_DARK = (0, 0, 0, 255)

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

    # White central bar.
    d.rounded_rectangle(
        [p(14), p(28), p(46), p(32)],
        radius=p(2),
        fill=WHITE,
    )

    # Teal weight plates, mirrored around the centre.
    # Inner plates are brighter; outer plates are darker.
    plates = [
        (16, 22, 13, TEAL),
        (10, 15, 9, TEAL_DARK),
        (38, 44, 13, TEAL),
        (45, 50, 9, TEAL_DARK),
    ]

    for x0, x1, half_height, colour in plates:
        d.rounded_rectangle(
            [
                p(x0),
                p(30 - half_height),
                p(x1),
                p(30 + half_height),
            ],
            radius=p(1.5),
            fill=colour,
        )

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
