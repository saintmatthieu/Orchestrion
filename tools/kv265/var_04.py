# Mozart K. 265, Var. IV (Breitkopf & Haertel 1878, p. 3)
KEY = (0, "major")
TIME = (2, 4)


def tri(a, b, c):
    return f"3[ {a}/1 {b}/1 {c}/1 ]"


def nb(low, upper, target):
    """bass note (staccato), then a slurred chromatic/neighbour pair, as an eighth triplet"""
    return f"3[ {low}/1!staccato ({upper}/1 {target}/1) ]"


A = [
    ("C5+G4+E4/2 C5/2", f"{tri('C2', 'E2', 'G2')} {tri('C3', 'E3', 'C3')}"),
    ("G5+E5+C5/2 G5/2", f"{tri('C2', 'E2', 'G2')} {tri('C3', 'E3', 'C3')}"),
    (("A5+C5/4", "(G5/2 F5/2)"), f"{tri('C2', 'F2', 'A2')} {tri('C3', 'F3', 'C3')}"),
    (("G5+C5/4", "(F5/2 E5/2)"), f"{tri('C2', 'E2', 'G2')} {tri('C3', 'E3', 'C3')}"),
    ("F5+E5+C5/2 F5+D5+G4/2", f"{nb('A1', 'G#2', 'A2')} {nb('B1', 'A#2', 'B2')}"),
    ("E5+D5+G4/2 E5+C5+A4/2", f"{nb('C2', 'B2', 'C3')} {nb('A1', 'G#2', 'A2')}"),
    ("D5+C5+A4/2 D5+B4+F4/2", f"{nb('F1', 'E2', 'F2')} {nb('G1', 'F#2', 'G2')}"),
    ("C5+G4+E4/2 r/2", f"{tri('C2', 'E2', 'G2')} C3/2"),
]

B = [
    ("G5+E5/2 G5+E5/2", f"{tri('G2', 'C3', 'E3')} {tri('G3', 'C4', 'E4')}"),
    (("F5/4", "(E5/2 D5/2)"), f"{tri('G4', 'F4', 'G4')} {tri('G3', 'A3', 'B3')}"),
    (("E5/2~ E5/1.5 A5/0.5", "(D5/2 C5/2)"), f"{tri('C4', 'B3', 'C4')} {tri('C3', 'E3', 'F3')}"),
]

MEASURES = (
    [(*A[0], {"label": "VAR. IV", "repeat": "start"})]
    + A[1:7]
    + [(*A[7], "end")]
    + [(*B[0], "start")]
    + B[1:]
    + [(("(E5/2 3[ D5/1) E5/1 F5/1 ]", "(C5/2 B4/2)"), f"{tri('G3', 'F#3', 'G3')} G2/2")]
    + B
    + [(("(E5/2 3[ D5/1) F5/1 D5/1 ]", "(C5/2 B4/2)"), f"{tri('G3', 'F#3', 'G3')} G2/2")]
    + A[:7]
    + [(*A[7], "end")]
)
