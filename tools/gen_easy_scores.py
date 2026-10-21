#!/usr/bin/env python3
"""Generates easy piano scores in scores/ as compressed MusicXML (.mxl): transcriptions of
public-domain originals (credited in scores/attributions.json) and our own arrangements of
traditional tunes, marked CC0.

Run from the repository root: python3 tools/gen_easy_scores.py

Piano, two staves.

The notation is described in tools/scorenotation.py.
"""
import sys

import kv265
from scorenotation import score_xml, write_mxl

# Ode to Joy - D major
D, A = "D3+F#3+A3", "A2+C#3+E3"
ode_a = [
    ("F#4/2 F#4/2 G4/2 A4/2", f"{D}/8"),
    ("A4/2 G4/2 F#4/2 E4/2", f"{D}/4 {A}/4"),
    ("D4/2 D4/2 E4/2 F#4/2", f"{D}/4 {A}/4"),
]
ode = (
    ode_a
    + [("F#4/3 E4/1 E4/4", f"{D}/4 {A}/4")]
    + ode_a
    + [("E4/3 D4/1 D4/4", f"{A}/4 {D}/4")]
    + [
        ("E4/2 E4/2 F#4/2 D4/2", f"{A}/4 {D}/4"),
        ("E4/2 F#4/1 G4/1 F#4/2 D4/2", f"{A}/4 {D}/4"),
        ("E4/2 F#4/1 G4/1 F#4/2 E4/2", f"{A}/4 {D}/4"),
        ("D4/2 E4/2 A3/4", f"{D}/4 {A}/4"),
    ]
    + ode_a
    + [("E4/3 D4/1 D4/4", f"{A}/4 {D}/4")]
)

# Greensleeves - A minor, 3/4, with a pickup
Am, Cg, Gg, Em, Fg, E = "A2+C3+E3", "C3+E3+G3", "G2+B2+D3", "E3+G3+B3", "F2+A2+C3", "E3+G#3+B3"
gs_phrase = [
    ("C5/4 D5/2", Am),
    ("E5/3 F5/1 E5/2", Cg),
    ("D5/4 B4/2", Gg),
    ("G4/3 A4/1 B4/2", Em),
]
gs_tail = [
    ("C5/3 B4/1 A4/2", Am),
    ("G#4/3 F#4/1 G#4/2", E),
    ("A4/6~", f"{Am}/6~"),
    ("A4/6", f"{Am}/6"),
]
gs_verse = gs_phrase + [
    ("C5/4 A4/2", Am),
    ("A4/3 G#4/1 A4/2", Fg),
    ("B4/4 G#4/2", E),
    ("E4/4 A4/2", E),
] + gs_phrase + gs_tail
gs_chorus = [
    ("G5/6", Cg),
    ("G5/3 F#5/1 E5/2", Em),
    ("D5/4 B4/2", Gg),
    ("G4/3 A4/1 B4/2", Em),
    ("C5/4 A4/2", Am),
    ("A4/3 G#4/1 A4/2", Fg),
    ("B4/4 G#4/2", E),
    ("E4/6", E),
    ("G5/6", Cg),
    ("G5/3 F#5/1 E5/2", Em),
    ("D5/4 B4/2", Gg),
    ("G4/3 A4/1 B4/2", Em),
] + gs_tail
greensleeves = [("A4/2", "r/2")] + [(rh, lh if "/" in lh else f"{lh}/6") for rh, lh in gs_verse + gs_chorus]

# Minuet in G major, BWV Anh. 114 (Christian Petzold), from the Bach-Gesellschaft edition (vol. 43, 1894)
# as typeset by Allen Garvin for the Mutopia Project (public domain): note for note, ornaments included.
minuet = [
    ("D5/2 G4/1 A4/1 B4/1 C5/1", ("G3+B3+D4/4 A3/2"), "start"),
    ("D5/2 G4/2 G4/2", "B3/6"),
    ("E5/2 C5/1!mordent D5/1 E5/1 F#5/1", "C4/6"),
    ("G5/2 G4/2 G4/2", "B3/6"),
    ("C5/2!mordent D5/1 C5/1 B4/1 A4/1", "A3/6"),
    ("B4/2 C5/1 B4/1 A4/1 G4/1", "G3/6"),
    ("F#4/2 G4/1 A4/1 B4/1 G4/1", "D4/2 B3/2 G3/2"),
    ("^B4/1 A4/6", "D4/2 D3/1 C4/1 B3/1 A3/1"),
    ("D5/2 G4/1 A4/1 B4/1 C5/1", "B3/4 A3/2"),
    ("D5/2 G4/2 G4/2", "G3/2 B3/2 G3/2"),
    ("E5/2 C5/1!mordent D5/1 E5/1 F#5/1", "C4/6"),
    ("G5/2 G4/2 G4/2", "B3/2 C4/1 B3/1 A3/1 G3/1"),
    ("C5/2!mordent D5/1 C5/1 B4/1 A4/1", "A3/4 F#3/2"),
    ("B4/2 C5/1 B4/1 A4/1 G4/1", "G3/4 B3/2"),
    ("A4/2 B4/1 A4/1 G4/1 F#4/1", "C4/2 D4/2 D3/2"),
    ("G4/6", "G3/4 G2/2", "end"),
    ("B5/2 G5/1 A5/1 B5/1 G5/1", "G3/6", "start"),
    ("A5/2 D5/1 E5/1 F#5/1 D5/1", "F#3/6"),
    ("G5/2 E5/1 F#5/1 G5/1 D5/1", "E3/2 G3/2 E3/2"),
    ("C#5/2 B4/1 C#5/1 A4/2", "A3/4 A2/2"),
    ("A4/1 B4/1 C#5/1 D5/1 E5/1 F#5/1", "A3/6"),
    ("G5/2 F#5/2 E5/2", "B3/2 D4/2 C#4/2"),
    ("F#5/2 A4/2 C#5/2", "D4/2 F#3/2 A3/2"),
    ("D5/6", "D4/2 D3/2 C4/2"),
    ("D5/2 G4/1 F#4/1 G4/2", ("r/2 D4/4", "B3/4 B3/2")),
    ("E5/2 G4/1 F#4/1 G4/2", ("r/2 E4/4", "C4/4 C4/2")),
    ("D5/2 C5/2 B4/2", "B3/2 A3/2 G3/2"),
    ("A4/1 G4/1 F#4/1 G4/1 A4/2", "D4/4 r/2"),
    ("D4/1 E4/1 F#4/1 G4/1 A4/1 B4/1", ("r/2 r/2 F#3/2", "D3/6")),
    ("C5/2 B4/2!inverted-mordent A4/2", "E3/2 G3/2 F#3/2"),
    ("B4/1 D5/1 G4/2 F#4/2", "G3/2 B2/2 D3/2"),
    ("G4+D4+B3/6", "G3/2 D3/2 G2/2", "end"),
]

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "scores"
    # The file names are what the File menu lists, and they sort the shelf.
    k265 = f"{out}/Ah vous dirai-je, Maman"
    write_mxl(
        f"{k265} - 12 Variations.mxl",
        score_xml('12 Variations on "Ah vous dirai-je, Maman", K. 265', "Wolfgang Amadeus Mozart", 0, "major", 2, kv265.measures(), rights="Public domain"),
    )
    write_mxl(
        f"{k265} - Theme.mxl",
        score_xml('Theme of "Ah vous dirai-je, Maman", K. 265', "Wolfgang Amadeus Mozart", 0, "major", 2, kv265.labelled_theme(), rights="Public domain"),
    )
    for n in range(1, 13):
        v = kv265.variation(n)
        write_mxl(
            f"{k265} - Variation {n}.mxl",
            score_xml(
                f'Variation {kv265.ROMAN[n - 1]} on "Ah vous dirai-je, Maman", K. 265',
                "Wolfgang Amadeus Mozart",
                *v.KEY,
                v.TIME[0],
                v.MEASURES,
                rights="Public domain",
                beat_type=v.TIME[1],
            ),
        )
    write_mxl(f"{out}/Ode to Joy.mxl", score_xml("Ode to Joy", "Ludwig van Beethoven", 2, "major", 4, ode))
    write_mxl(f"{out}/Minuet in G major, BWV Anh. 114.mxl", score_xml("Minuet in G major, BWV Anh. 114", "Christian Petzold", 1, "major", 3, minuet, rights="Public domain"))
    write_mxl(f"{out}/Greensleeves.mxl", score_xml("Greensleeves", "Traditional", 0, "minor", 3, greensleeves, implicit_pickup=True))
