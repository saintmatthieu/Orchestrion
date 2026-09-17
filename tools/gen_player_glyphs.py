#!/usr/bin/env python3
"""Regenerate the transport glyphs in icons/player/.

The playback controls are cut from Cinzel Decorative, the face the score title
is set in (fonts/CinzelDecorative, loaded by GoldOrnament.qml). Two glyphs come
straight out of the font, the guillemets used for previous and next; everything
else is drawn here with a pen taken from the same letter.

The constants below were measured off the font's lower-case o, which is its
capital with the swash removed:

  - the silhouette is a superellipse of exponent 2.13, a shade squarer than an
    ellipse, 769 units wide by 728 tall;
  - the stress axis stands upright, about 5 degrees off vertical;
  - the contrast is 2.65:1, flank to crown.

The weights are lighter than the letter's own. A 36 px icon is a small optical
size and the family ships no light cut to borrow, so the ring is re-cut on the
letter's silhouette rather than lifted from it at text weight. Two floors keep
it legible there: 1.10 px for a straight line, which is where a short
horizontal starts to look broken, and 0.85 px for a curve, which thins
gradually and never reads as a gap.

Requires fontTools. Run from the repository root:

    python3 tools/gen_player_glyphs.py

Copyright (C) 2026 Matthieu Hodgkinson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
"""

import math
import os
import sys

from fontTools.pens.boundsPen import BoundsPen
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.ttLib import TTFont

FONT = "fonts/CinzelDecorative/CinzelDecorative-Regular.ttf"
OUT_DIR = "icons/player"
CONTROLLER_DIR = "icons/controllers"
# Only ever seen when a file is opened on its own: in the app every glyph is
# re-tinted with the current theme's metal (see MetalGlyph).
PREVIEW_FILL = "#D4A858"

BOX = 36.0          # the icon's box, and the size the buttons run at
CX = CY = BOX / 2

RING_H = 27.0       # the letter's height inside that box
RING_AR = 769 / 728  # it is a little wider than tall
RING_N = 2.13       # superellipse exponent of its silhouette
AXIS = 5.0          # stress axis, degrees off vertical
CONTRAST = 2.65     # the font's own thick-to-thin ratio

RING_W = 1.50       # the ring's flank; the letter's own is 2.89
W = 1.46            # every inner mark's heaviest stroke
FLOOR = 1.10        # thinnest straight line
RING_FLOOR = 0.85   # thinnest curve
STEM_CORR = 0.85    # a vertical shows all its ink in one column while a chevron
                    # splits the same measure over two diagonals, so the stem is
                    # cut narrower to look its equal
MARK_H = 12.4       # chevrons and stems stand the same height

# The guillemet's arm is 47 units thick at 400 units tall, so at MARK_H it comes
# out at W and the stems can be cut to match it.
GUIL_ARM, GUIL_H = 47.0, 400.0


def fmt(v):
    return f"{v:.2f}".rstrip("0").rstrip(".")


def pts(seq):
    return " ".join(f"{fmt(x)},{fmt(y)}" for x, y in seq)


def stroke_w(phi_deg, w=None, floor=None):
    """Thickness of a stroke running at phi degrees (0 = horizontal, y down)."""
    w = W if w is None else w
    floor = FLOOR if floor is None else floor
    thin = w / CONTRAST
    return max(floor, thin + (w - thin) * abs(math.sin(math.radians(phi_deg - AXIS))))


# --------------------------------------------------------------- the font
def font_glyph(font, ch, height, cx, cy, dx=0.0, dy=0.0):
    """One glyph's own outline, scaled to `height` px and centred on (cx, cy)."""
    glyph_set = font.getGlyphSet()
    glyph = glyph_set[font.getBestCmap()[ord(ch)]]
    path = SVGPathPen(glyph_set)
    glyph.draw(path)
    bounds = BoundsPen(glyph_set)
    glyph.draw(bounds)
    x0, y0, x1, y1 = bounds.bounds
    s = height / (y1 - y0)
    return (f'<path transform="translate({cx - s * (x0 + x1) / 2 + dx:.3f},'
            f'{cy + s * (y0 + y1) / 2 + dy:.3f}) scale({s:.5f},{-s:.5f})" '
            f'd="{path.getCommands()}"/>')


# --------------------------------------------------------------- the letter
def superellipse(u, a, b, n=RING_N):
    c, s = math.cos(u), math.sin(u)
    return (a * math.copysign(abs(c) ** (2 / n), c),
            b * math.copysign(abs(s) ** (2 / n), s))


def se_point(u, a, b):
    """A point on the letter's curve at parameter u, with its tangent."""
    x, y = superellipse(u, a, b)
    x2, y2 = superellipse(u + 1e-3, a, b)
    dx, dy = x2 - x, -(y2 - y)                   # tangent, SVG y down
    length = math.hypot(dx, dy)
    return (CX + x, CY - y), (dx / length, dy / length)


def inward(px, py, tx, ty):
    nx, ny = -ty, tx
    if (px - CX) * nx + (py - CY) * ny > 0:
        nx, ny = -nx, -ny
    return nx, ny


def ring(w=RING_W, n=144):
    """The letter's outline, re-cut at weight w: the silhouette is sampled from
    the superellipse and the counter offset inward by what each direction
    earns."""
    a, b = RING_H * RING_AR / 2, RING_H / 2
    outer, inner = [], []
    for i in range(n):
        (px, py), (tx, ty) = se_point(2 * math.pi * i / n, a, b)
        t = stroke_w(math.degrees(math.atan2(ty, tx)), w, RING_FLOOR)
        nx, ny = inward(px, py, tx, ty)
        outer.append((px, py))
        inner.append((px + nx * t, py + ny * t))
    return f'<path fill-rule="evenodd" d="M{pts(outer)} Z M{pts(inner)} Z"/>'


def ring_area(n=720):
    """Area enclosed by the letter's silhouette, by shoelace on a fine
    sampling. The full-screen corners take their side from it."""
    a, b = RING_H * RING_AR / 2, RING_H / 2
    p = [superellipse(2 * math.pi * i / n, a, b) for i in range(n)]
    s = sum(p[i][0] * p[(i + 1) % n][1] - p[(i + 1) % n][0] * p[i][1]
            for i in range(n))
    return abs(s) / 2


# --------------------------------------------------------------- pen shapes
def cup(a, b, gx, gy, amount):
    mx, my = (a[0] + b[0]) / 2, (a[1] + b[1]) / 2
    vx, vy = gx - mx, gy - my
    n = math.hypot(vx, vy) or 1
    return mx + vx / n * amount, my + vy / n * amount


def cupped(poly, amount, reverse=False):
    """Closed path whose sides bow inward, so the corners flare like a serif.
    `amount` may be one value for every edge or one per edge, where 0 leaves
    that edge straight."""
    seq = list(reversed(poly)) if reverse else list(poly)
    amounts = amount if isinstance(amount, (list, tuple)) else [amount] * len(seq)
    gx = sum(p[0] for p in seq) / len(seq)
    gy = sum(p[1] for p in seq) / len(seq)
    d = f"M{fmt(seq[0][0])},{fmt(seq[0][1])} "
    for i in range(len(seq)):
        a, b = seq[i], seq[(i + 1) % len(seq)]
        c = cup(a, b, gx, gy, amounts[i])
        d += f"Q{fmt(c[0])},{fmt(c[1])} {fmt(b[0])},{fmt(b[1])} "
    return d + "Z "


def inset(poly):
    """Inner contour: every edge pushed in by the weight its direction earns."""
    n = len(poly)
    gx = sum(p[0] for p in poly) / n
    gy = sum(p[1] for p in poly) / n
    lines = []
    for i in range(n):
        (x0, y0), (x1, y1) = poly[i], poly[(i + 1) % n]
        dx, dy = x1 - x0, y1 - y0
        length = math.hypot(dx, dy)
        nx, ny = -dy / length, dx / length
        if (gx - x0) * nx + (gy - y0) * ny < 0:
            nx, ny = -nx, -ny
        t = stroke_w(math.degrees(math.atan2(dy, dx)))
        lines.append(((x0 + nx * t, y0 + ny * t), (dx, dy)))
    out = []
    for i in range(n):
        (px, py), (d1x, d1y) = lines[i - 1]
        (qx, qy), (d2x, d2y) = lines[i]
        t = ((qx - px) * d2y - (qy - py) * d2x) / (d1x * d2y - d1y * d2x)
        out.append((px + d1x * t, py + d1y * t))
    return out


def outlined(poly, cup_out=0.9, cup_in=0.55):
    d = cupped(poly, cup_out) + cupped(inset(poly), -cup_in, reverse=True)
    return f'<path fill-rule="evenodd" d="{d}"/>'


def stem(cx, top, bot, w=None, hair=0.55, bracket=1.25):
    """A Roman stem with bracketed serifs, after the font's I."""
    w = W * STEM_CORR if w is None else w
    serif = w * 2.1
    left, right = cx - w / 2, cx + w / 2
    sl, sr = cx - serif / 2, cx + serif / 2
    th, tb = top + hair, top + hair + bracket
    bb, bh = bot - hair - bracket, bot - hair
    f = fmt
    return (f'<path d="M{f(sl)},{f(top)} L{f(sr)},{f(top)} L{f(sr)},{f(th)} '
            f'Q{f(right)},{f(th)} {f(right)},{f(tb)} L{f(right)},{f(bb)} '
            f'Q{f(right)},{f(bh)} {f(sr)},{f(bh)} L{f(sr)},{f(bot)} '
            f'L{f(sl)},{f(bot)} L{f(sl)},{f(bh)} Q{f(left)},{f(bh)} {f(left)},{f(bb)} '
            f'L{f(left)},{f(tb)} Q{f(left)},{f(th)} {f(sl)},{f(th)} Z"/>')


# --------------------------------------------------------------- the marks
def play_poly(cx=18.7, cy=CY, w=11.6, h=13.2):
    return [(cx - w * 0.4, cy - h / 2), (cx + w * 0.6, cy), (cx - w * 0.4, cy + h / 2)]


def square_poly(a=11.9):
    h = a / 2
    return [(CX - h, CY - h), (CX + h, CY - h), (CX + h, CY + h), (CX - h, CY + h)]


def loop(w=RING_W, head=4.5, gap=36.0):
    """The ring's own curve, at the ring's own size, opened at both flanks and
    set turning: the letter, in motion.

    The arcs occupy the ring's exact band. The superellipse is the ring's outer
    silhouette, so the stroke is laid inward from it rather than straddling it,
    which would leave the figure half a width proud of the letter.

    Both arcs are walked from u0 down to u1, so travel always runs against the
    parameter and both heads point the same way round.
    """
    a, b = RING_H * RING_AR / 2, RING_H / 2
    out = []
    for u0, u1 in ((180 - gap / 2, gap / 2), (360 - gap / 2, 180 + gap / 2)):
        n = 44
        outer, inner = [], []
        for i in range(n + 1):
            u = math.radians(u0 + (u1 - u0) * i / n)
            (px, py), (tx, ty) = se_point(u, a, b)
            t = stroke_w(math.degrees(math.atan2(ty, tx)), w, RING_FLOOR)
            nx, ny = inward(px, py, tx, ty)
            outer.append((px, py))
            inner.append((px + nx * t, py + ny * t))
        out.append(f'<path d="M{pts(outer)} L{pts(reversed(inner))} Z"/>')

        # The head sits where the arc stops, pointing the way it was going. It
        # is centred on the stroke rather than on the silhouette, and its base
        # stays straight and a little inside the arc so the two join flush.
        (px, py), (tx, ty) = se_point(math.radians(u1), a, b)
        t = stroke_w(math.degrees(math.atan2(ty, tx)), w, RING_FLOOR)
        mx, my = inward(px, py, tx, ty)
        px, py = px + mx * t / 2, py + my * t / 2
        vx, vy = -tx, -ty                        # direction of travel
        nx, ny = -vy, vx                         # across it
        half, back = t * 1.7, 0.35
        bx, by = px - vx * back, py - vy * back
        out.append('<path d="%s"/>' % cupped(
            [(bx + nx * half, by + ny * half),
             (px + vx * head, py + vy * head),
             (bx - nx * half, by - ny * half)], [0.5, 0.5, 0.0]))
    return "".join(out)


KEYS = 4            # white keys in the MIDI keyboard indicator
KEY_START = "F"     # the note it starts on, which decides where the black keys go

# Whether the step up to the next white key is a whole tone, and so has a black
# key between them.
WHITE = "CDEFGAB"
WHOLE_TONE = {"C": True, "D": True, "E": False,
              "F": True, "G": True, "A": True, "B": False}


def keyboard(keys=None, start=None):
    """The MIDI keyboard indicator, built on the square the full-screen corners
    imply, so the two marks stand on the same footprint.

    Its case is drawn with the corners' own two widths, and the lines between
    the white keys are the Roman stem's width. The keys themselves take what is
    left over rather than the stem's serif width: the serif would fit six keys
    into the case, which leaves a gap 47 % as wide as a key and reads as a
    railing rather than an instrument. Four keys give a gap of 27 % and leave
    room for a black key.

    Which white key it starts on decides where the black keys fall, since a
    black key sits only where the step up to the next white key is a whole
    tone. Starting on F gives F G A B, whose three gaps all take one; starting
    on C would leave the E to F gap bare.

    A black key is half again the width of the line it stands on, rather than
    the 0.55 of a white key an instrument would use: at this size that came out
    heavy enough to read as the subject of the drawing. Below it, the line
    between two white keys carries on down to the bottom rail.
    """
    keys = KEYS if keys is None else keys
    start = KEY_START if start is None else start
    side = math.sqrt(ring_area())
    tv, th = stroke_w(90), stroke_w(0)
    x0, y0 = CX - side / 2, CY - side / 2
    x1, y1 = CX + side / 2, CY + side / 2

    # The case, as a frame.
    out = [f'<path fill-rule="evenodd" d="M{fmt(x0)},{fmt(y0)} H{fmt(x1)} V{fmt(y1)} '
           f'H{fmt(x0)} Z M{fmt(x0 + tv)},{fmt(y0 + th)} V{fmt(y1 - th)} '
           f'H{fmt(x1 - tv)} V{fmt(y0 + th)} Z"/>']

    ix0, iy0 = x0 + tv, y0 + th                  # the case's inside
    ix1, iy1 = x1 - tv, y1 - th
    sep = W * STEM_CORR                          # the stem
    key_w = (ix1 - ix0 - (keys - 1) * sep) / keys
    black_w = sep * 1.5
    black_d = (iy1 - iy0) * 0.6                  # how far a black key reaches down

    first = WHITE.index(start)
    for i in range(1, keys):
        # The gap between white key i and the one after it.
        cx = ix0 + i * key_w + (i - 0.5) * sep
        note = WHITE[(first + i - 1) % len(WHITE)]
        if not WHOLE_TONE[note]:                 # a semitone: no black key here
            out.append(f'<rect x="{fmt(cx - sep / 2)}" y="{fmt(iy0)}" '
                       f'width="{fmt(sep)}" height="{fmt(iy1 - iy0)}"/>')
        else:
            out.append(f'<rect x="{fmt(cx - black_w / 2)}" y="{fmt(iy0)}" '
                       f'width="{fmt(black_w)}" height="{fmt(black_d)}"/>')
            out.append(f'<rect x="{fmt(cx - sep / 2)}" y="{fmt(iy0 + black_d)}" '
                       f'width="{fmt(sep)}" height="{fmt(iy1 - iy0 - black_d)}"/>')
    return "".join(out)


def fullscreen():
    """Four corners. The square they imply is given the letter's area, so the
    two marks cover the same ground even though one is round."""
    side = math.sqrt(ring_area())
    arm = side * 0.34
    x0, y0 = CX - side / 2, CY - side / 2
    x1, y1 = CX + side / 2, CY + side / 2
    tv, th = stroke_w(90), stroke_w(0)
    out = []
    for x, y, sx, sy in ((x0, y0, 1, 1), (x1, y1, -1, -1), (x1, y0, -1, 1), (x0, y1, 1, -1)):
        vx = x if sx > 0 else x - tv
        vy = y if sy > 0 else y - arm
        hx = x if sx > 0 else x - arm
        hy = y if sy > 0 else y - th
        out.append(f'<rect x="{fmt(vx)}" y="{fmt(vy)}" width="{fmt(tv)}" height="{fmt(arm)}"/>'
                   f'<rect x="{fmt(hx)}" y="{fmt(hy)}" width="{fmt(arm)}" height="{fmt(th)}"/>')
    return "".join(out)


def build(font):
    chevron_left = font_glyph(font, "‹", MARK_H, CX, CY, dx=-0.3)
    chevron_right = font_glyph(font, "›", MARK_H, CX, CY, dx=0.3)
    band = ring()
    return {
        # Ringed: the letter, holding a mark cut with its own pen.
        # The stem and the chevron are placed as a pair so that the two together
        # sit centred in the ring, and the stem is cut a shade shorter than
        # MARK_H so that its serifs do not crowd the chevron beside it.
        "rewind": band + stem(12.9, CY - 6.1, CY + 6.1)
                       + font_glyph(font, "‹", MARK_H, 21.0, CY),
        "previous": band + chevron_left,
        "play": band + outlined(play_poly(), 1.05, 0.6),
        "stop": band + outlined(square_poly(), 0.85, 0.5),
        # Taken from the font rather than mirrored in QML, so the ring is never
        # rotated with the arrow.
        "next": band + chevron_right,
        # Unringed: these two are not transport actions.
        "loop": loop(),
        "fullscreen": fullscreen(),
    }


def write(path, body):
    with open(path, "w", encoding="utf-8") as f:
        f.write(f'<svg xmlns="http://www.w3.org/2000/svg" width="{fmt(BOX)}" '
                f'height="{fmt(BOX)}" viewBox="0 0 {fmt(BOX)} {fmt(BOX)}">'
                f'<g fill="{PREVIEW_FILL}">{body}</g></svg>\n')
    print(f"wrote {path}")


def main():
    if not os.path.isfile(FONT):
        sys.exit(f"{FONT} not found. Run this from the repository root.")
    font = TTFont(FONT)
    os.makedirs(OUT_DIR, exist_ok=True)
    os.makedirs(CONTROLLER_DIR, exist_ok=True)
    for name, body in build(font).items():
        write(os.path.join(OUT_DIR, name + ".svg"), body)
    # Not a transport control, but cut from the same drawing.
    write(os.path.join(CONTROLLER_DIR, "midi-keyboard.svg"), keyboard())


if __name__ == "__main__":
    main()
