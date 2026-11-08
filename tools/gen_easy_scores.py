#!/usr/bin/env python3
"""Generates the bundled Mozart scores in scores/ as compressed MusicXML (.mxl): Mozart's
12 Variations on "Ah vous dirai-je, Maman", K. 265, whole, and its theme and each variation
on their own, transcribed from a public-domain edition (credited in scores/attributions.json).

Run from the repository root: python3 tools/gen_easy_scores.py
then python3 tools/stamp_bundled_scores.py ../rolls, which writes in their difficulty.

Piano, two staves.

The notation is described in tools/scorenotation.py.
"""
import sys

import kv265
from scorenotation import score_xml, write_mxl

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "scores"
    # The file names are what the File menu lists, and they sort the shelf.
    k265 = f"{out}/Ah vous dirai-je, Maman"
    write_mxl(
        f"{k265} - 12 Variations.mxl",
        score_xml('12 Variations on "Ah vous dirai-je, Maman", K. 265', "Wolfgang Amadeus Mozart", 0, "major", 2, kv265.measures(), rights="Public domain", opus="K. 265"),
    )
    write_mxl(
        f"{k265} - Theme.mxl",
        score_xml('Theme of "Ah vous dirai-je, Maman", K. 265', "Wolfgang Amadeus Mozart", 0, "major", 2, kv265.labelled_theme(), rights="Public domain", opus="K. 265"),
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
                opus="K. 265",
            ),
        )
