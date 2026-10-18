# Var. X: page 6, systems 3-7. The quarter notes marked "L.H." cross over into the treble staff
# (voice 1 there); the 16th figuration is the right hand.
KEY = (0, "major")
TIME = (2, 4)


def g(a, b, c):
    """One beat: a 16th rest and three slurred 16ths."""
    return f"r/0.5 ({a}/0.5 {b}/0.5 {c}/0.5)"


def bar(a1, b1, c1, a2, b2, c2):
    return f"{g(a1, b1, c1)} {g(a2, b2, c2)}"


_first = [
    (("s/2 @W:L.H. C5/2", "@p " + bar("E4", "C5", "E4", "E4", "G4", "E4")), "C2+C3/2 s/2"),
    (("G5/2 G5/2", bar("E4", "C5", "E4", "E4", "C5", "E4")), "s/4"),
    (("A5/2 A5/2", bar("F4", "C5", "F4", "F4", "C5", "F4")), "s/4"),
    (("G5/2 s/2", f"{g('E4', 'C5', 'E4')} @f {g('Bb4', 'E5+G5', 'Bb4')}"), "s/2 C#3+C#4/2"),
    (bar("A4", "D5+F#5", "A4", "Ab4", "D5+F5", "Ab4"), "D3+D4/2 B2+B3/2"),
    (bar("G4", "C5+E5", "G4", "A4", "C5+Eb5", "A4"), "C3+C4/2 F#2+F#3/2"),
    (bar("G4", "C5+D5", "G4", "F4", "B4+D5", "F4"), "G2+G3/2 G2/2"),
    ("E4+C5/2 r/2", "C3/2 C2/2", "end"),
]

MEASURES = (
    [(_first[0][0], _first[0][1], {"label": "VAR. X", "repeat": "start"})]
    + _first[1:]
    + [
        (("s/2 @W:L.H. G5/2", "@p " + bar("E4", "C5", "E4", "E4", "C5", "E4")), "G1+G2/2 s/2", "start"),
        (("F#5/2 F5/2", bar("D4", "C5", "D4", "D4", "B4", "D4")), "s/4"),
        (("E5/2 s/2", bar("C4", "C5", "C4", "G4", "C5+Eb5", "G4")), "s/2 G2+G3/2"),
        (bar("A4", "C5+D5", "A4", "Ab4", "B4+D5", "Ab4"), "F#2+F#3/2 F2+F3/2"),
        ("@f " + bar("G4", "C5+E5", "G4", "G4", "C#5+G5", "G4"), "E2+E3/2 Eb2+Eb3/2"),
        (bar("A4", "D5+F#5", "A4", "Ab4", "D5+F5", "Ab4"), "D2+D3/2 B1+B2/2"),
        (bar("G4", "C5+E5", "G4", "A4", "C5+Eb5", "A4"), "C2+C3/2 F#1+F#2/2"),
        (
            ("@dim (D5/0.5 C5/0.5 B4/0.5 A4/0.5 G4/0.5 F4/0.5 E4/0.5 D4/0.5) @end", "B4/1 s/1 s/2"),
            "G1+G2/2 r/2",
        ),
    ]
    + [_first[0]]
    + _first[1:]
)
