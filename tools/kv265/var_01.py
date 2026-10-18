# Mozart K. 265, Var. I (Breitkopf & Haertel 1878, p. 1)
KEY = (0, "major")
TIME = (2, 4)


def s16(notes):
    return " ".join(f"{n}/0.5" for n in notes.split())


_a = [
    ("@w:legato " + s16("D5 C5 B4 C5 B4 C5 B4 C5"), "C3/2 C4/2", {"label": "VAR. I", "repeat": "start"}),
    (s16("A5 G5 F#5 G5 F#5 G5 F#5 G5"), "E4/2 C4/2"),
    (s16("G#5 A5 C6 B5 D6 C6 B5 A5"), "F4/2 C4/2"),
    (s16("A5 G5 E6 D6 C6 B5 A5") + " G5/0.5~", "E4/2 r/1.5 C#4/0.5"),
    (s16("G5 F5 D6 C6 B5 A5 G5") + " F5/0.5~", "D4/2 r/1.5 B3/0.5"),
    (s16("F5 E5 C6 B5 A5 G5 F5 E5"), "C4/2 r/1.5 A3/0.5"),
    ("D5/1 A5/1 G5/1 B4/1", "F3/2 G3/2"),
]
_b = [
    (s16("A5 G5 F#5 G5 F#5 G5 A5 G5"), "E4/2 G3/2", "start"),
    (s16("G5 F5 E5 F5 E5 F5 G5 F5"), "D4/2 G3/2"),
    (s16("F5 E5 D#5 E5 D#5 E5 F5 E5"), "C4/2 G3/2"),
    (s16("E5 D5 C#5 D5 C#5 D5 E5 D5"), "F4/2 G3/2"),
    (s16("A5 G5 F#5 G5 E6 C6 A5 G5"), "G3+E4/4"),
    (s16("G5 F5 E5 F5 D6 B5 G5 F5"), "G3+D4/4"),
    (s16("F5 E5 D#5 E5 C6 G5 F5 E5"), "G3+C4/2 r/1.5 C4/0.5"),
    ("(G5/1.5 E5/0.5 D5/2)", ("(E4/1.5 C4/0.5 B3/2)", "G3/4")),
]
MEASURES = (
    _a
    + [
        ("C5/2 r/2", "C4/1 G3/1 E3/1 G3/1", {"ending": "1", "repeat": "end"}),
        ("C5/2 r/2", "C4/2 C3/2", {"ending": "2"}),
    ]
    + _b
    + [(rh.replace("@w:legato ", ""), lh) for rh, lh, *_ in _a]
    + [("C5/2 r/2", "C4/2 C3/2", "end")]
)
