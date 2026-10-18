# Var. XII (Allegro), K. 265 (Breitkopf & Haertel 1878, p. 7 last system and p. 8)
KEY = (0, "major")
TIME = (3, 4)


def s16(notes):
    """A slurred run of 16ths."""
    toks = [f"{n}/0.5" for n in notes.split()]
    toks[0] = "(" + toks[0]
    toks[-1] += ")"
    return " ".join(toks)


def lh(bass, octave, last=None):
    """
    One bar of the A-section left hand: `bass` then C B C | D C B C | D C B C in `octave`, all
    under one slur; or, with `last` (four notes), the third beat replaced and slurred apart.
    """
    o, p = octave, octave - 1
    if last is None:
        return s16(f"{bass} C{o} B{p} C{o} D{o} C{o} B{p} C{o} D{o} C{o} B{p} C{o}")
    return s16(f"{bass} C{o} B{p} C{o} D{o} C{o} B{p} C{o}") + " " + s16(last)


def lh2(first, a, b, c, last):
    """A left-hand bar of two slurred beats `first` a b a | c a b a and a third slurred beat `last`."""
    return s16(f"{first} {a} {b} {a} {c} {a} {b} {a}") + " " + s16(last)


def tr(note, lower, upper):
    """Third beat: a trilled dotted eighth and two 32nds, the slur running into the next bar."""
    return f"({note}/1.5!trill-mark {lower}/0.25 {upper}/0.25"


def r16(a, b, c):
    """A 16th rest and three slurred 16ths."""
    return f"r/0.5 ({a}/0.5 {b}/0.5 {c}/0.5)"


_coda_up = s16("C5 G4 A4 B4 C5 D5 E5 F5 G5 A5 B5 C6")
_coda_down = " ".join(f"{n}/0.5" for n in "G5 A5 F5 G5 E5 F5 D5 E5 C5 D5 B4 D5".split())
_coda_lh = ("G3/6", "E4/1 D4/1 C4/1 F4/1 E4/1 D4/1")

MEASURES = [
    # A section (bars 1-8)
    (
        "@f E4+G4+C5/2 r/2 " + tr("C5", "B4", "C5"),
        lh("C2", 3),
        {"label": "VAR. XII", "tempo": "Allegro", "time": (3, 4), "repeat": "start"},
    ),
    ("G5/2) r/2 " + tr("C5", "B4", "C5"), lh("E2", 3)),
    ("C5+G5+A5/2) C5+F5+A5/2 C5+F5+A5/2", lh("F2", 3)),
    ("C5+E5+G5/2 r/2 " + tr("G5", "F#5", "G5"), lh("C2", 3, "C#2 C#3 B2 C#3")),
    ("F#5/2) r/2 " + tr("F5", "E5", "F5"), lh2("D2", "D3", "C#3", "E3", "B1 B2 A2 B2")),
    ("E5/2) r/2 " + tr("C5", "B4", "C5"), lh2("C2", "C3", "B2", "D3", "A1 A2 G#2 A2")),
    ("D5/2 F5/2) D4+F4+B4/2", lh2("F1", "F2", "E2", "G2", "G1 G2 F#2 G2")),
    ("E4+C5/2 r/2 r/2", s16("C2 D2 E2 F2 G2 A2 B2 C3") + " C2/2", "end"),
    # B section (bars 9-16)
    (s16("A5 G5 F#5 G5 F#5 G5 F#5 G5 F#5 G5 F#5 G5"), s16("G3 E4 D#4 E4 D#4 E4 D#4 E4 D#4 E4 D#4 E4"), "start"),
    (s16("G5 F5 E5 F5 E5 F5 E5 F5 E5 F5 E5 F5"), s16("G3 D4 C#4 D4 C#4 D4 C#4 D4 C#4 D4 C#4 D4")),
    (s16("F5 E5 D#5 E5 D#5 E5 D#5 E5 D#5 E5 D#5 E5"), s16("G3 C4 B3 C4 B3 C4 B3 C4 B3 C4 B3 C4")),
    (s16("E5 D5 C#5 D5 C#5 D5 C#5 D5 C#5 D5 C#5 D5"), s16("G3 F4 E4 F4 E4 F4 E4 F4 E4 F4 E4 F4")),
    (s16("A5 G5 F#5 G5 F#5 G5 F#5 G5 E6 G5 E6 G5"), s16("G3 E4 D#4 E4 D#4 E4 D#4 E4 G3 E4 G3 E4")),
    (s16("G5 F5 E5 F5 E5 F5 E5 F5 D6 F5 D6 F5"), s16("G3 D4 C#4 D4 C#4 D4 C#4 D4 G3 D4 G3 D4")),
    (s16("F5 E5 D#5 E5 D#5 E5 D#5 E5 C6 E5 C6 E5"), s16("G3 C4 B3 C4 B3 C4 B3 C4 G3 C4 G3 C4")),
    (s16("E5 D5 C#5 D5 C#5 D5 C#5 D5 G5 B4 G5 B4"), s16("G3 F4 E4 F4 E4 F4 E4 F4 G3 F4 G3 F4")),
    # A section, left hand an octave higher (bars 17-24)
    ("E4+G4+C5/2 r/2 " + tr("C5", "B4", "C5"), lh("C3", 4)),
    ("G5/2) r/2 " + tr("C5", "B4", "C5"), lh("E3", 4)),
    ("C5+G5+A5/2) C5+F5+A5/2 C5+F5+A5/2", lh("F3", 4)),
    ("C5+E5+G5/2 r/2 " + tr("G5", "F#5", "G5"), lh("C3", 4, "C#3 C#4 B3 C#4")),
    ("F#5/2) r/2 " + tr("F5", "E5", "F5"), lh2("D3", "D4", "C#4", "E4", "B2 B3 A3 B3")),
    ("E5/2) r/2 " + tr("C5", "B4", "C5"), lh2("C3", "C4", "B3", "D4", "A2 A3 G#3 A3")),
    ("D5/2 F5/2) D4+F4+B4/2", lh2("F2", "F3", "E3", "G3", "G2 G3 F#3 G3")),
    ("E4+C5/2 r/2 r/2", s16("C3 D3 E3 F3 G3 A3 B3 C4") + " C3/2", {"ending": "1", "repeat": "end"}),
    # Second ending, leading into the coda (bars 25-28)
    (
        "E4+G4+C5/2 r/2 " + tr("G5", "F#5", "G5"),
        lh2("E2", "E3", "D3", "F3", "Eb2 Eb3 D3 Eb3"),
        {"ending": "2-open"},
    ),
    ("F#5/2) r/2 " + tr("F5", "E5", "F5"), lh2("D2", "D3", "C#3", "E3", "B1 B2 A2 B2")),
    ("E5/2) r/2 " + tr("C5", "B4", "C5"), lh2("C2", "C3", "B2", "D3", "A1 A2 G#2 A2")),
    ("D5/2) r/2 D4+F4+B4/2", lh2("F1", "F2", "E2", "G2", "G1 G2 F#2 G2")),
    # Coda (bars 29-36)
    (_coda_up.replace(")", ""), "C2+E2+C3/2 r/2 r/2"),
    (_coda_down, _coda_lh),
    (_coda_up.replace("(", "").replace(")", ""), "C4+E4/2 r/2 r/2"),
    (_coda_down + ")", _coda_lh),
    ("C5/2 " + r16("C4", "B3", "C4") + " " + r16("E4", "D#4", "E4"), "C4+E4/2 C2+C3/2 E2+E3/2"),
    (r16("G4", "F#4", "G4") + " " + r16("E4", "D#4", "E4") + " " + r16("G4", "F#4", "G4"), "G2+G3/2 E2+E3/2 G2+G3/2"),
    (r16("C5", "B4", "C5") + " " + r16("E5", "D#5", "E5") + " " + r16("G5", "F#5", "G5"), "C2+C3/2 E2+E3/2 G2+G3/2"),
    ("C5+E5+G5+C6/2 r/2 r/2", "C3+E3+G3+C4/2 r/2 r/2", {"barline": "light-heavy"}),
]
