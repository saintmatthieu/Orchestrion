# Var. XI (Adagio): page 7, systems 1-5, from the Breitkopf & Haertel edition (1878, IMSLP 24784).
KEY = (0, "major")
TIME = (2, 4)


def alb(lo, hi):
    """Four 16ths: a low note then three repeated thirds (the left hand of bars 9-10 and 13-14)."""
    return f"{lo}/0.5 {hi}/0.5 {hi}/0.5 {hi}/0.5"


_first = [
    ("@p (C5/1.5 G4/0.5) (E5/1.5 C5/0.5)", "R/4"),
    ("G5/1 G5/2 G5/1", "(C4/1.5 G3/0.5) (E4/1.5 C4/0.5)"),
    ("@fp C5+G5+A5/1 C5+F5+A5/2 C5+F5+A5/1", "(F3/1.5 C3/0.5) (A3/1.5 F3/0.5)"),
    ("@fp C5+F5+G5/1 C5+E5+G5/2 C5+E5+G5/1", "(C3/1.5 G2/0.5) (E3/1.5 C3/0.5)"),
    (
        "(G5/0.5 F5/0.5) F5/1 r/0.5 (E6/0.25 D6/0.25) (C6/0.25 B5/0.25) (A5/0.25 G5/0.25)",
        "A3+C4+F4/1 r/1 B3+D4+G4/1 r/1",
    ),
    (
        "(F5/0.75 D#5/0.25 E5/1) r/0.5 (A5/0.25 G5/0.25) (F5/0.25 E5/0.25) (D5/0.25 C5/0.25)",
        "C4+E4+G4/1 r/1 E3+G3+C4/1 r/1",
    ),
    (
        "(B4/0.75 A4/0.25) | (G5/0.25 F5/0.25 E5/0.25 D5/0.25 | C5/1) B4/1!staccato",
        ("D4/2 G4/1 F4/1", "F3/2 G3/2"),
    ),
    ("(F4+G4+B4+D5/2 E4+G4+C5/1) r/1", "@dim C4/1 G3/1 C3/1 @end r/1", "end"),
]

_second_first = [
    (
        "@p (G5/1.5 A5/0.25 B5/0.25 C6/0.5) B5/0.5!staccato A5/0.5!staccato G5/0.5!staccato",
        alb("G3", "C4+E4") + " " + alb("G3", "C4+E4"),
        "start",
    ),
    (
        "(G5/0.5 F5/0.25) r/0.25 (E5/0.5 D5/0.25) r/0.25 (C5/1 B4/0.5) r/0.5",
        alb("G3", "D4+F4") + " " + alb("G3", "D4+F4"),
    ),
    (
        "(C5/1.5!delayed-turn D5/0.5) (E5/1.5!delayed-turn F5/0.5)",
        "E4+G4/1.5 B3+G4/0.5 C4+G4/1.5 D4/0.5",
    ),
    (
        "(E5/1.5 F5/0.25 E5/0.25 D5/0.5) (E5/0.5!staccato F5/0.5!staccato F#5/0.5!staccato)",
        "(G3+C4/2 G3+B3/1) r/1",
    ),
    (
        "(G5/1.5!delayed-turn A5/0.25 B5/0.25 C6/0.25 D6/0.25 E6/0.25 D6/0.25 C6/0.25 B5/0.25 A5/0.25 G5/0.25)",
        alb("G3", "E4+G4") + " " + alb("G3", "E4+G4"),
    ),
    (
        "@w:cresc. (G5/0.25 F5/0.25) (F6/0.25 E6/0.25) (E6/0.25 D6/0.25) (D6/0.25 C6/0.25) "
        "(C6/0.25 B5/0.25) (B5/0.25 A5/0.25) (A5/0.25 G5/0.25) (G5/0.25 F5/0.25)",
        alb("G3", "D4+F4") + " " + alb("G3", "B3+D4"),
    ),
    (
        "(E5/0.75 C6/0.25) C6/1!fermata (C6/0.25 D6/0.25 E6/0.25 D6/0.25) C6/0.75 F#5/0.25",
        "C4+E4/1 @f A3+C4+F#4/2!fermata r/1",
    ),
    (
        "(A5/2 G5/1) 3[ r/0.25!fermata (A5/0.25 G5/0.25 ] 3[ F5/0.25 E5/0.25 D5/0.25) ]",
        "@dim (G3+C4/2 G3+B3/1) @end r/1!fermata",
    ),
]

MEASURES = (
    [(_first[0][0], _first[0][1], {"label": "VAR. XI", "tempo": "Adagio", "repeat": "start"})]
    + _first[1:]
    + _second_first
    + _first
)
