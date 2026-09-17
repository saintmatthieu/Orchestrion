#!/usr/bin/env python3
"""Generate the notation view's backdrops, one per theme.

A vertical backdrop -> accent -> backdrop gradient with a soft-edged accent
plateau in the middle, where the score lies. The metal ornaments (rules +
diamond at 15 % / 85 % of the height) are NOT part of the image: they are
drawn live by src/qml/BackdropOrnament.qml so they stay crisp at any window
size and so the top one can carry the score title.

Orchestrion ships two themes, gold on mahogany and silver on slate, so this
writes one image each. Which one the app loads is named by the
`orchestrion_wallpaper` key of src/App/configs/{light,dark}.cfg, next to the
colours below.

The image is stretched to the viewport by the (patched) paint view, so its
aspect ratio only needs to be roughly that of a screen.

Requires Pillow:  pip install pillow
"""

import os

from PIL import Image

W, H = 2560, 1440

# --- Palettes (shared with src/App/configs/{light,dark}.cfg) ---
# (file name, backdrop = top & bottom, accent = center band)
THEMES = [
    ("orchestrion_gold.jpg", (60, 31, 25), (240, 229, 200)),    # #3C1F19 / #F0E5C8
    ("orchestrion_silver.jpg", (37, 44, 51), (230, 234, 238)),  # #252C33 / #E6EAEE
]

# Half-height of the fully-accent center band, and the extra pixels of soft
# transition on either side of it.
PLATEAU_HALF = int(H * 0.18)
FALLOFF = int(H * 0.22)


def smoothstep(t: float) -> float:
    if t <= 0:
        return 0.0
    if t >= 1:
        return 1.0
    return t * t * (3 - 2 * t)


def mix(a, b, t: float):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))


def render(backdrop, accent) -> Image.Image:
    image = Image.new("RGB", (W, H))
    pixels = image.load()
    cy = H / 2
    for y in range(H):
        dist = abs(y - cy)
        if dist <= PLATEAU_HALF:
            t = 0.0
        elif dist >= PLATEAU_HALF + FALLOFF:
            t = 1.0
        else:
            t = smoothstep((dist - PLATEAU_HALF) / FALLOFF)
        row = mix(accent, backdrop, t)
        for x in range(W):
            pixels[x, y] = row
    return image


def main() -> None:
    here = os.path.dirname(os.path.abspath(__file__))
    for name, backdrop, accent in THEMES:
        out = os.path.join(here, name)
        render(backdrop, accent).save(out, "JPEG", quality=90)
        print(f"Wrote {out}  ({W} x {H})")


if __name__ == "__main__":
    main()
