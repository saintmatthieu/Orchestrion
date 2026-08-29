#!/usr/bin/env python3
"""Generate orchestrion_parchment.jpg, the notation view's backdrop.

A vertical mahogany -> cream -> mahogany gradient with a soft-edged cream
plateau in the middle, where the score lies. The gold ornaments (rules +
diamond at 15 % / 85 % of the height) are NOT part of the image: they are
drawn live by src/qml/GoldOrnament.qml so they stay crisp at any window size
and so the top one can carry the score title.

The image is stretched to the viewport by the (patched) paint view, so its
aspect ratio only needs to be roughly that of a screen.

Requires Pillow:  pip install pillow
"""

import os

from PIL import Image

W, H = 2560, 1440

# --- Palette (shared with GoldOrnament.qml / Theme.qml) ---
MAHOGANY = (61, 31, 26)  # #3D1F1A — top & bottom
CREAM = (240, 229, 200)  # #F0E5C8 — center band

# Half-height of the fully-cream center band, and the extra pixels of soft
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


def main() -> None:
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
        row = mix(CREAM, MAHOGANY, t)
        for x in range(W):
            pixels[x, y] = row

    out = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       "orchestrion_parchment.jpg")
    image.save(out, "JPEG", quality=90)
    print(f"Wrote {out}  ({W} x {H})")


if __name__ == "__main__":
    main()
