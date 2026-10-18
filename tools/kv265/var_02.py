# Mozart K. 265, Var. II (Breitkopf & Haertel 1878, p. 2). Its ties all curve under, as printed.
KEY = (0, "major")
TIME = (2, 4)


def s16(notes):
    return " ".join(f"{n}/0.5" for n in notes.split())


_v7 = ("D5/2 D5/2", "A4+C5/2 G4+B4/1.5 F4/0.5")
MEASURES = [
    ("C5/2 C5/2", "@w:legato " + s16("C3 C4 B3 C4 D4 C4 B3 C4"), {"label": "VAR. II", "repeat": "start"}),
    ("C5+G5/2 C5+G5/2~_", s16("E3 C4 B3 C4 D4 C4 B3 C4")),
    (("A5/4", "G5/2 F5/2~_", "C5/4~_"), s16("F3 C4 B3 C4 D4 C4 B3 C4")),
    (("G5/4", "F5/2 E5/2~_", "C5/4~_"), s16("C3 C4 B3 C4 D4 C4 B3 C4")),
    (("F5/2 F5/2", "C5+E5/2 G4+D5/2"), s16("A2 A3 G#3 A3 B2 B3 A#3 B3")),
    (("E5/2 E5/2", "G4+D5/2 A4+C5/2~_"), s16("C3 C4 B3 C4 A2 A3 G#3 A3")),
    (_v7, s16("F2 F3 E3 F3 G2 G3 F#3 G3")),
    ("E4+G4+C5/2 r/2", "C2/2 C3/2", "end"),
    ("C5+G5/2 C5+G5/2", s16("G3 E4 D#4 E4 F4 E4 D#4 E4"), "start"),
    ("C5+F5/2 B4+F5/2", s16("G3 D4 C#4 D4 E4 D4 C#4 D4")),
    ("C5+E5/1.5 D5/0.5 C5+E5/1.5 F5/0.5", s16("G3 G4 F#4 G4 A4 G4 F#4 G4")),
    (("(E5/2 D5/2)", "B4/4"), s16("G3 F4 E4 F4 G4 F4 E4 F4")),
    ("C5+G5/2 C5+G5/2", s16("G3 E4 D#4 E4 F4 E4 D#4 E4")),
    ("C5+F5/2 B4+F5/2", s16("G3 D4 C#4 D4 E4 D4 C#4 D4")),
    ("G4+E5/1.5 D5/0.5 G4+E5/1.5 F5/0.5", s16("G3 C4 B3 C4 D4 C4 B3 C4")),
    (("(E5/2 D5/2)", "G4/4"), s16("G3 B3 A#3 B3 C4 B3 A#3 B3")),
    ("E4+G4+C5/2 (C5/1.5!trill-mark B4/0.25 C5/0.25", s16("C3 C4 B3 C4 D4 C4 B3 C4")),
    ("G5/2) C5+G5/2~_", s16("E3 C4 B3 C4 D4 C4 B3 C4")),
    (("A5/2 A5/2", "C5+G5/2 C5+F5/2~_"), s16("E3 C4 B3 C4 D4 C4 B3 C4")),
    (("G5/2 Bb4+G5/2", "C5+F5/2 E5/2~_"), s16("E3 C4 B3 C4 C#3 C#4 B#3 C#4")),
    (("F#5/2 Ab4+F5/2", "A4+E5/2 D5/2~_"), s16("D3 D4 C#4 D4 B2 B3 A#3 B3")),
    (("E5/2 E5/2", "G4+D5/2 A4+C5/2~_"), s16("C3 C4 B3 C4 A2 A3 G#3 A3")),
    (_v7, s16("F2 F3 E3 F3 G2 G3 F#3 G3")),
    ("E4+G4+C5/2 r/2", "C2/2 C3/2", "end"),
]
