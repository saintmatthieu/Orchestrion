"""Mozart's 12 Variations on "Ah vous dirai-je, Maman" (Twinkle, Twinkle, Little Star), K. 265, in the
notation of scorenotation.py: a note-for-note transcription of the Breitkopf & Haertel complete
edition (Mozarts Werke, Serie 21 No. 6, 1878), repeats and endings as printed.
"""

# The theme: its slurs, the mf and the turns written out as grace notes in bars 15 and 23.
_theme_a = [
    ("@mf C5/2 C5/2", "C3/2 C4/2"),
    ("G5/2 G5/2", "E4/2 C4/2"),
    ("A5/2 A5/2", "F4/2 C4/2"),
    ("G5/2 G5/2", "E4/2 C4/2"),
    ("F5/2 F5/2", "D4/2 B3/2"),
    ("E5/2 E5/2", "C4/2 A3/2"),
]
THEME = (
    _theme_a
    + [
        ("D5/2 (D5/1.5 E5/0.5", "F3/2 G3/2"),
        ("C5/4)", "C3/4", "end"),
        ("G5/2 G5/2", "E4/2 G3/2", "start"),
        ("F5/2 F5/2", "D4/2 G3/2"),
        ("E5/2 E5/2", "C4/2 G3/2"),
        ("D5/2 D5/2", "B3/2 G3/2"),
        ("G5/2 G5/2", "E4/2 G3/2"),
        ("F5/2 F5/2", "D4/2 G3/2"),
        ("(E5/2 ^F5/0.5 ^E5/0.5 ^D5/0.5 E5/1.5 F5/0.5", "C4/2 (C4/1.5 D4/0.5"),
        ("E5/2 D5/2)", ("C4/2 B3/2)", "G3/4")),
    ]
    + [(rh.replace("@mf ", ""), lh) for rh, lh in _theme_a]
    + [
        ("(D5/2 ^E5/0.5 ^D5/0.5 ^C5/0.5 D5/1.5 E5/0.5", "F3/2 G3/2"),
        ("C5/4)", "C3/4", "end"),
    ]
)

# The variations, one module each (var_01 ... var_12): MEASURES, written once with the print's repeat
# signs and endings, the first measure labelled and carrying any key or time change.
_VARIATIONS = [f"var_{n:02d}" for n in range(1, 13)]
ROMAN = ["I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII"]


def variation(n):
    """
    Variation n (1-12) as a module with KEY and TIME (in force at its first measure) and MEASURES.
    """
    import importlib

    return importlib.import_module(f"{__name__}.{_VARIATIONS[n - 1]}")


def labelled_theme():
    """
    The theme with its "TEMA" label, as it opens the work.
    """
    first, *rest = THEME
    return [(first[0], first[1], {"label": "TEMA"})] + rest


def measures():
    """
    The whole work: the theme, labelled, then the variations in order.
    """
    result = labelled_theme()
    for n in range(1, 13):
        result += variation(n).MEASURES
    return result
