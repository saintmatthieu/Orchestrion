# Mozart K. 265, Var. VI (Breitkopf & Haertel 1878, p. 4)
KEY = (0, "major")
TIME = (2, 4)


def x(*notes):
    return " ".join(f"{n}/0.5" for n in notes)


def ch(c1, c2):
    """an eighth chord, eighth rest, eighth chord, eighth rest"""
    return f"{c1}/1 r/1 {c2}/1 r/1"


C = "E4+G4+C5"
CEG = "C5+E5+G5"
G2C = "G2+C3+E3/2"

MEASURES = [
    ("@p " + ch(C, C), "@w:legato " + x("C3", "B2", "C3", "B2", "C3", "B2", "C3", "B2"),
     {"label": "VAR. VI", "repeat": "start"}),
    (ch(CEG, CEG), x("C3", "B2", "C3", "B2", "C3", "D3", "E3", "C3")),
    ("@w:cresc. " + ch("C5+G5+A5", "C5+F5+A5"), x("F3", "E3", "F3", "E3", "F3", "G3", "A3", "B3")),
    (ch("C5+F5+G5", CEG), x("C4", "B3", "C4", "B3", "C4", "D4", "E4", "C4")),
    (ch("A4+E5+F5", "G4+D5+F5"), x("D4", "C#4", "D4", "C4", "B3", "A3", "B3", "G3")),
    ("G4+D5+E5/1 r/1 @f A4+C5+E5/1 r/1", x("C4", "B3", "C4", "B3", "A3", "G#3", "A3", "G3")),
    (ch("F4+A4+D5", "D4+F4+B4"), x("F3", "E3", "F3", "D3", "G3", "F#3", "G3", "G2")),
    ("E4+G4+C5/2 r/2", x("C3", "G2", "E2", "G2") + " C2/2", "end"),
    # second half: the right hand rests (unprinted) while the left hand plays alone
    ("s/4", ("@p @w:legato " + x("E3+G3", "F#3", "G3", "F#3", "E3+G3", "F#3", "G3", "F#3"), "G2/2 G2/2"), "start"),
    ("s/4", (x("D3+F3+G3", "F#3", "G3", "F#3", "D3+F3+G3", "A3", "B3", "G3"), "G2/2 G2/2")),
    ("s/1.5 " + x("D4", "E4", "D4", "E4", "F#4"), (x("C4", "B3", "C4") + " s/0.5 s/2", f"{G2C} {G2C}")),
    (x("G4", "F#4", "G4", "F#4", "G4", "F#4", "G4", "F#4"), "G2+B2+D3/2 G2+B2+D3/2"),
    ("@w:cresc. " + x("G4", "F#4", "G4", "F#4", "G4", "F#4", "G4", "F#4"), "G2+E3+G3/2 G2+E3+G3/2"),
    (x("G4", "F#4", "G4", "F#4", "G4", "A4", "B4", "G4"), "G2+D3+F3/2 G2+D3+F3/2"),
    ("@f " + x("C5", "B4", "C5", "D5", "E5", "D5", "E5", "F#5"), f"{G2C} {G2C}"),
    ("@dim " + x("G5", "A5", "F5", "G5", "E5", "F5", "D5", "E5") + " @end", "G2+B2+D3/2 r/2"),
    ("@p C5/1 r/1 E4+G4+C5/1 r/1", x("C3", "B2", "C3", "B2", "C3", "B2", "C3", "B2")),
    (ch(CEG, CEG), x("C3", "B2", "C3", "B2", "C3", "D3", "E3", "C3")),
    ("@w:cresc. " + ch("C5+G5+A5", "C5+F5+A5"), x("F3", "E3", "F3", "E3", "F3", "G3", "A3", "B3")),
    (ch("C5+F5+G5", CEG), x("C4", "B3", "C4", "B3", "C4", "D4", "E4", "C4")),
    (ch("A4+E5+F5", "G4+D5+F5"), x("D4", "C#4", "D4", "C4", "B3", "A3", "B3", "G3")),
    ("G4+D5+E5/1 r/1 @f A4+C5+E5/1 r/1", x("C4", "B3", "C4", "B3", "A3", "G#3", "A3", "G3")),
    (ch("F4+A4+D5", "D4+F4+B4"), x("F3", "E3", "F3", "D3", "G3", "F#3", "G3", "G2")),
    ("E4+G4+C5/2 r/2", x("C3", "G2", "E2", "G2") + " C2/2", "end"),
]
