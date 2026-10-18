# Mozart K. 265, Var. III (Breitkopf & Haertel 1878, p. 2)
KEY = (0, "major")
TIME = (2, 4)


def t(a, b):
    """two triplet groups of eighths"""
    return f"3[ {a} ] 3[ {b} ]"


_a = [
    (t("(C4/1 E4/1 G4/1", "C5/1 E5/1 G5/1"), "C2/2 C3/2", {"label": "VAR. III", "repeat": "start"}),
    (t("C6/1 G5/1 F5/1", "E5/1!inverted-mordent D5/1 C5/1)"), ("C4/4", "E3/2 C3/2")),
    (t("(A5/1 G#5/1 A5/1", "C6/1 B5/1 A5/1)"), ("C4/4", "F3/2 C3/2")),
    (t("(A5/1 F#5/1) G5/1!staccato", "G5/1!staccato C6/1!staccato E6/1!staccato"), ("C4/4", "E3/2 C3/2")),
    (t("(E6/1 F5/1) F5/1!staccato", "F5/1!staccato B5/1!staccato D6/1!staccato"), "A3+C4/2 B3+D4/2"),
    (t("(D6/1 E5/1) E5/1!staccato", "E5/1!staccato A5/1!staccato C6/1!staccato"), "C4+E4/2 A3+C4/2"),
    (t("(C6/1 D5/1) A5/1!staccato", "(A5/1 G5/1) B4/1!staccato"), ("s/2 F4/1.5 D4/0.5", "F3+A3/2 G3+B3/2")),
]
_b = [
    (t("(G5/1 F#5/1) G5/1!staccato", "(E6/1 C6/1) G5/1!staccato"), ("E4/2 C4/2", "G3/4"), "start"),
    (t("(G5/1 E5/1) F5/1!staccato", "(D6/1 B5/1) F5/1!staccato"), ("D4/2 B3/2", "G3/4")),
    (t("(F5/1 D#5/1) E5/1!staccato", "(C6/1 G5/1) E5/1!staccato"), ("C4/2 E4/2", "G3/4")),
    (t("(E5/1 C#5/1) D5/1!staccato", "(F5/1 D5/1) B4/1!staccato"), ("F4/4", "G3/4")),
    (t("(G5/1!inverted-mordent F#5/1) G5/1!staccato", "(E6/1 A5/1) G5/1!staccato"), ("E4/2 C#4/2", "G3/4")),
    (t("(G5/1 E5/1) F5/1!staccato", "(D6/1 B5/1) F5/1!staccato"), ("D4/2 B3/2", "G3/4")),
    ("@f " + t("(F5/1 D#5/1) E5/1!staccato", "(C6/1 A5/1) E5/1!staccato"), ("C4/2 A3+C4/2", "G3/2 F#3/2")),
    ("(G5/1.5 E5/0.5 D5/2)", ("(E4/1.5 C4/0.5 B3/2)", "G3/4")),
]
_end = ("C5/2 r/2", "3[ C4+E4/1 G3/1 E3/1 ] C3/2")
MEASURES = (
    _a
    + [_end + ("end",)]
    + _b
    + [(rh, lh) for rh, lh, *_ in _a]
    + [_end + ("end",)]
)
