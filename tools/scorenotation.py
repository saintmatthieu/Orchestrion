"""A compact text notation for two-staff piano scores, written out as compressed MusicXML (.mxl).

A score is a list of measures. A measure is (rh, lh) or (rh, lh, opts): rh and lh are a voice
string or a tuple of voice strings (one per voice on that staff, voices 1-4 and 5-8). opts is either
a repeat string ("start", "end", "start-end") or a dict with any of:
  repeat     "start", "end" or "start-end"
  ending     volta number ("1", "2"), optionally suffixed: "1" starts and ends on this measure,
             "1-start" starts here, "1-stop" ends here; "-open" variants have no closing hook:
             "2-open" starts and ends here, "2-stop-open" ends here
  key        (fifths, mode), e.g. (-3, "minor")
  time       (beats, beat_type), e.g. (3, 4)
  label      bold text above the staff at the measure start (e.g. "VAR. I")
  tempo      text above the staff at the measure start (e.g. "Adagio")
  barline    right barline style when not a repeat (e.g. "light-light")

A voice is a space-separated list of tokens:
  <pitches>/<dur>   a note (C4, F#4, Bb3), a chord (C3+E3+G3), a rest (r), a whole-bar rest (R, with
                    the bar's length) or an invisible rest (s, for a voice or staff the print leaves
                    empty); <dur> is in eighths:
                    0.125=64th, 0.25=32nd, 0.375=dotted 32nd, 0.5=16th, 0.75=dotted 16th, 1=eighth,
                    1.5=dotted eighth, 2=quarter, 3=dotted quarter, 4=half, 6=dotted half, 8=whole.
                    Modifiers: a leading ^ makes a grace note, a leading ( starts a slur and a trailing
                    ) ends it, a trailing ~ ties into the next note (~_ curving under, ~^ over;
                    by default a chord's top tie curves over and its bottom one under), and each
                    !<mark> adds a mark:
                    an ornament (trill-mark, turn, inverted-turn, mordent, inverted-mordent), an
                    articulation (staccato, staccatissimo, accent, tenuto), fermata or arpeggiate.
  3[ ... ]          a triplet: the notes inside are written at their notated value and last 2/3 of it.
  |                 breaks the beam before the next note.
  @<dynamic>        a dynamic (p, pp, mf, f, ff, fp, sf, sfz) at this point.
  @cresc @dim @end  start a crescendo or diminuendo hairpin, end it.
  @w:<text>         words below the staff (underscores for spaces), @W:<text> above it.
  %G %F             change the staff's clef to treble or bass.
"""
import zipfile
from fractions import Fraction
from xml.sax.saxutils import escape

DIVISIONS = 48  # per quarter: fits 64ths and triplet 32nds
TYPES = {
    Fraction(1, 8): ("64th", 0),
    Fraction(1, 4): ("32nd", 0),
    Fraction(3, 8): ("32nd", 1),
    Fraction(1, 2): ("16th", 0),
    Fraction(3, 4): ("16th", 1),
    Fraction(1): ("eighth", 0),
    Fraction(3, 2): ("eighth", 1),
    Fraction(2): ("quarter", 0),
    Fraction(3): ("quarter", 1),
    Fraction(4): ("half", 0),
    Fraction(6): ("half", 1),
    Fraction(8): ("whole", 0),
}
BEAM_LEVELS = {"eighth": 1, "16th": 2, "32nd": 3, "64th": 4}
ORNAMENTS = {"trill-mark", "turn", "inverted-turn", "delayed-turn", "mordent", "inverted-mordent"}
ARTICULATIONS = {"staccato", "staccatissimo", "accent", "tenuto", "strong-accent"}
DYNAMICS = {"pp", "p", "mp", "mf", "f", "ff", "fp", "sf", "sfz", "fz"}
TRIPLET = Fraction(2, 3)


def ticks(eighths):
    t = eighths * DIVISIONS / 2
    if t.denominator != 1:
        raise ValueError(f"duration {eighths} eighths is not a whole number of divisions")
    return int(t)


def parse(voice):
    """
    Returns the voice's events: notes (dicts with a "pitches" key) and, in place, the directions and
    clef changes between them (dicts with a "direction" or "clef" key).
    """
    events = []
    tuplet = None
    beam_break = False
    for tok in voice.split():
        if tok == "|":
            beam_break = True
            continue
        if tok == "3[":
            tuplet = []
            continue
        if tok == "]":
            notes = [e for e in tuplet if "pitches" in e]
            notes[0]["tuplet"] = "start"
            notes[-1]["tuplet"] = "stop"
            events += tuplet
            tuplet = None
            continue
        target = events if tuplet is None else tuplet
        if tok.startswith("@"):
            target.append({"direction": tok[1:]})
            continue
        if tok.startswith("%"):
            target.append({"clef": tok[1:]})
            continue
        grace = slur_start = False
        while tok[0] in "^(":
            grace |= tok[0] == "^"
            slur_start |= tok[0] == "("
            tok = tok[1:]
        tie = slur_stop = False
        tie_orientation = ""
        while tok[-1] in "~)_^":
            if tok[-1] in "_^":
                if tok[-2] != "~":
                    raise ValueError(f"a tie direction follows a tie, in {voice!r}")
                tie_orientation = "under" if tok[-1] == "_" else "over"
            tie |= tok[-1] == "~"
            slur_stop |= tok[-1] == ")"
            tok = tok[:-1]
        tok, *marks = tok.split("!")
        pitches, dur = tok.split("/")
        for m in marks:
            if m not in ORNAMENTS | ARTICULATIONS | {"fermata", "arpeggiate"}:
                raise ValueError(f"unknown mark !{m}")
        written = Fraction(dur)
        if written not in TYPES:
            raise ValueError(f"unsupported duration {dur} in {voice!r}")
        target.append(
            {
                "pitches": None if pitches in ("r", "R", "s") else pitches.split("+"),
                "measure_rest": pitches == "R",
                "hidden": pitches == "s",
                "written": written,
                "dur": written * (TRIPLET if tuplet is not None else 1),
                "in_tuplet": tuplet is not None,
                "tuplet": "",
                "tie": tie,
                "tie_orientation": tie_orientation,
                "marks": marks,
                "grace": grace,
                "slur": ("start" if slur_start else "") + ("stop" if slur_stop else ""),
                "break": beam_break,
            }
        )
        beam_break = False
    if tuplet is not None:
        raise ValueError(f"unterminated triplet in {voice!r}")
    return events


def length(events):
    return sum((e["dur"] for e in events if "pitches" in e and not e["grace"]), Fraction(0))


def midi(p):
    step, rest = p[0], p[1:]
    alter = rest.count("#") - rest.count("b")
    return 12 * (int(rest.strip("#b")) + 1) + "C D EF G A B".index(step) + alter


def pitch_xml(p):
    step, rest = p[0], p[1:]
    alter = rest.count("#") - rest.count("b")
    octave = rest.strip("#b")
    a = f"<alter>{alter}</alter>" if alter else ""
    return f"<pitch><step>{step}</step>{a}<octave>{octave}</octave></pitch>"


def beams_of(notes, beat):
    """
    Beams consecutive notes shorter than a quarter (graces apart from the others), breaking at a |
    token and, in a run with 16ths, shorter notes or triplets, at each beat: level 1 over the whole run, each deeper
    level over the notes short enough for it, with a hook for a lone note.
    """
    beams = {i: [] for i in range(len(notes))}
    level_of = lambda i: BEAM_LEVELS.get(TYPES[notes[i]["written"]][0], 0) if notes[i]["pitches"] else 0
    runs, run, starts = [], [], {}
    pos = Fraction(0)
    for i, e in enumerate(notes):
        starts[i] = pos
        if run and (not level_of(i) or notes[run[-1]]["grace"] != e["grace"] or e.get("break")):
            runs.append(run)
            run = []
        if level_of(i):
            run.append(i)
        if not e["grace"]:
            pos += e["dur"]
    runs.append(run)
    split = []
    for run in runs:
        if not any(level_of(i) > 1 or notes[i]["in_tuplet"] for i in run):
            split.append(run)
            continue
        part = []
        for i in run:
            if part and not notes[i]["grace"] and starts[i] % beat == 0:
                split.append(part)
                part = []
            part.append(i)
        split.append(part)
    for run in split:
        if len(run) < 2:
            continue
        for lvl in range(1, 5):
            seq = []
            for k, i in enumerate(run + [None]):
                if i is not None and BEAM_LEVELS[TYPES[notes[i]["written"]][0]] >= lvl:
                    seq.append(k)
                    continue
                if len(seq) == 1 and lvl > 1:
                    beams[run[seq[0]]].append((lvl, "backward hook" if seq[0] else "forward hook"))
                elif len(seq) > 1:
                    for j, kk in enumerate(seq):
                        beams[run[kk]].append((lvl, "begin" if j == 0 else "end" if j == len(seq) - 1 else "continue"))
                seq = []
    return beams


def direction_xml(d, staff, voice):
    if d in DYNAMICS:
        placement, content = "below", f"<dynamics><{d}/></dynamics>"
    elif d in ("cresc", "dim"):
        placement, content = "below", f'<wedge type="{"crescendo" if d == "cresc" else "diminuendo"}"/>'
    elif d == "end":
        placement, content = "below", '<wedge type="stop"/>'
    elif d[:2] in ("w:", "W:"):
        placement = "below" if d[0] == "w" else "above"
        content = f"<words>{escape(d[2:].replace('_', ' '))}</words>"
    else:
        raise ValueError(f"unknown direction @{d}")
    return (
        f'<direction placement="{placement}"><direction-type>{content}</direction-type>'
        f"<voice>{voice}</voice><staff>{staff}</staff></direction>"
    )


def notes_xml(events, staff, voice, tied_in, beat):
    notes = [e for e in events if "pitches" in e]
    beams = beams_of(notes, beat)
    xml = []
    n = 0
    for e in events:
        if "direction" in e:
            xml.append(direction_xml(e["direction"], staff, voice))
            continue
        if "clef" in e:
            sign, line = ("G", 2) if e["clef"] == "G" else ("F", 4)
            xml.append(f'<attributes><clef number="{staff}"><sign>{sign}</sign><line>{line}</line></clef></attributes>')
            continue
        typ, dots = TYPES[e["written"]]
        # A chord's ties curve away from it, the top one over and the bottom one
        # under: left to itself, MuseScore curves a tie into another voice the
        # way that voice's stems point, and a chord's two may cross.
        pitches = e["pitches"] or []
        top = max(pitches, key=midi) if len(pitches) > 1 else None
        bottom = min(pitches, key=midi) if len(pitches) > 1 else None
        for k, p in enumerate(e["pitches"] or [None]):
            ties, notations, ornaments, articulations = "", "", "", ""
            if p and tied_in.get((staff, p)):
                ties += '<tie type="stop"/>'
                notations += '<tied type="stop"/>'
            if p and e["tie"]:
                ties += '<tie type="start"/>'
                orientation = e["tie_orientation"] or ("over" if p == top else "under" if p == bottom else "")
                notations += f'<tied type="start" orientation="{orientation}"/>' if orientation else '<tied type="start"/>'
            if k == 0:
                if "start" in e["slur"]:
                    notations += f'<slur type="start" number="{voice}"/>'
                if "stop" in e["slur"]:
                    notations += f'<slur type="stop" number="{voice}"/>'
                if e["tuplet"]:
                    notations += f'<tuplet type="{e["tuplet"]}" bracket="no"/>'
                for m in e["marks"]:
                    if m in ORNAMENTS:
                        ornaments += f"<{m}/>"
                    elif m in ARTICULATIONS:
                        articulations += f"<{m}/>"
                    elif m == "fermata":
                        notations += '<fermata type="upright"/>'
                if ornaments:
                    notations += f"<ornaments>{ornaments}</ornaments>"
                if articulations:
                    notations += f"<articulations>{articulations}</articulations>"
            if "arpeggiate" in e["marks"]:
                notations += "<arpeggiate/>"
            beam = "".join(f'<beam number="{lvl}">{v}</beam>' for lvl, v in beams[n]) if k == 0 else ""
            xml.append(
                ('<note print-object="no">' if e["hidden"] else "<note>")
                + ("<grace/>" if e["grace"] else "")
                + ("<chord/>" if k else "")
                + (pitch_xml(p) if p else '<rest measure="yes"/>' if e["measure_rest"] else "<rest/>")
                + ("" if e["grace"] else f"<duration>{ticks(e['dur'])}</duration>")
                + f"{ties}<voice>{voice}</voice>"
                + ("" if e["measure_rest"] else f"<type>{typ}</type>" + "<dot/>" * dots)
                + ("<time-modification><actual-notes>3</actual-notes><normal-notes>2</normal-notes></time-modification>" if e["in_tuplet"] else "")
                + f"<staff>{staff}</staff>"
                + beam
                + (f"<notations>{notations}</notations>" if notations else "")
                + "</note>"
            )
        if not e["grace"]:
            for p in e["pitches"] or []:
                tied_in[(staff, p)] = e["tie"]
        n += 1
    return "".join(xml)


def ending_xml(ending, location):
    number, _, kind = ending.partition("-")
    if location == "left" and kind in ("", "start", "open"):
        return f'<ending number="{number}" type="start"/>'
    if location == "right" and kind in ("", "stop"):
        return f'<ending number="{number}" type="stop"/>'
    if location == "right" and kind in ("open", "stop-open"):
        return f'<ending number="{number}" type="discontinue"/>'
    return ""


def score_xml(title, composer, fifths, mode, beats, measures, implicit_pickup=False, rights="Public domain (CC0 1.0)", beat_type=4):
    parts = []
    tied_in = {}
    for i, measure in enumerate(measures):
        rh, lh = measure[0], measure[1]
        opts = measure[2] if len(measure) > 2 else {}
        if isinstance(opts, str):
            opts = {"repeat": opts}
        repeat = opts.get("repeat", "")
        ending = opts.get("ending", "")
        number = i if implicit_pickup else i + 1
        attrs = ""
        if i == 0:
            attrs += f"<divisions>{DIVISIONS}</divisions>"
        if i == 0 or "key" in opts:
            fifths, mode = opts.get("key", (fifths, mode))
            attrs += f"<key><fifths>{fifths}</fifths><mode>{mode}</mode></key>"
        if i == 0 or "time" in opts:
            beats, beat_type = opts.get("time", (beats, beat_type))
            attrs += f"<time><beats>{beats}</beats><beat-type>{beat_type}</beat-type></time>"
        if i == 0:
            attrs += (
                '<staves>2</staves><clef number="1"><sign>G</sign><line>2</line></clef>'
                '<clef number="2"><sign>F</sign><line>4</line></clef>'
            )
        full = Fraction(beats * 8, beat_type)
        beat = Fraction(8, beat_type) * (3 if beat_type == 8 else 1)
        expected = None if implicit_pickup and i == 0 else full
        body = ""
        for staff, voices in ((1, rh), (2, lh)):
            for v, voice in enumerate((voices,) if isinstance(voices, str) else voices):
                events = parse(voice)
                n = length(events)
                expected = expected or n
                if n != expected:
                    raise ValueError(f"{title}: measure {number} staff {staff} voice {v + 1} has {n} eighths, expected {expected}")
                if body:
                    body += f"<backup><duration>{ticks(n)}</duration></backup>"
                body += notes_xml(events, staff, 1 + v + 4 * (staff - 1), tied_in, beat)
        head = ""
        if "start" in repeat or ending:
            head += (
                '<barline location="left">'
                + ("<bar-style>heavy-light</bar-style>" if "start" in repeat else "")
                + (ending_xml(ending, "left") if ending else "")
                + ('<repeat direction="forward"/>' if "start" in repeat else "")
                + "</barline>"
            )
        if attrs:
            head += f"<attributes>{attrs}</attributes>"
        for key, extra in (("label", ' font-weight="bold"'), ("tempo", "")):
            if key in opts:
                head += (
                    f'<direction placement="above"><direction-type><words{extra}>{escape(opts[key])}</words>'
                    "</direction-type><staff>1</staff></direction>"
                )
        right = ending_xml(ending, "right") if ending else ""
        if repeat.endswith("end"):
            barline = f'<barline location="right"><bar-style>light-heavy</bar-style>{right}<repeat direction="backward"/></barline>'
        elif i == len(measures) - 1:
            barline = f'<barline location="right"><bar-style>light-heavy</bar-style>{right}</barline>'
        elif "barline" in opts or right:
            style = f"<bar-style>{opts['barline']}</bar-style>" if "barline" in opts else ""
            barline = f'<barline location="right">{style}{right}</barline>'
        else:
            barline = ""
        implicit = ' implicit="yes"' if implicit_pickup and i == 0 else ""
        parts.append(f'<measure number="{number}"{implicit}>{head}{body}{barline}</measure>')
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<!DOCTYPE score-partwise PUBLIC "-//Recordare//DTD MusicXML 4.0 Partwise//EN" '
        '"http://www.musicxml.org/dtds/partwise.dtd">\n'
        '<score-partwise version="4.0">'
        f"<work><work-title>{escape(title)}</work-title></work>"
        f'<identification><creator type="composer">{escape(composer)}</creator>'
        f"<rights>{escape(rights)}</rights></identification>"
        '<part-list><score-part id="P1"><part-name>Piano</part-name><part-abbreviation>Pno.</part-abbreviation>'
        '<score-instrument id="P1-I1"><instrument-name>Piano</instrument-name>'
        "<instrument-sound>keyboard.piano</instrument-sound></score-instrument></score-part></part-list>"
        '<part id="P1">' + "".join(parts) + "</part></score-partwise>\n"
    )


def write_mxl(path, xml):
    container = (
        '<?xml version="1.0" encoding="UTF-8"?>\n<container><rootfiles>'
        '<rootfile full-path="score.xml" media-type="application/vnd.recordare.musicxml+xml"/>'
        "</rootfiles></container>\n"
    )
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr(zipfile.ZipInfo("mimetype"), "application/vnd.recordare.musicxml", compress_type=zipfile.ZIP_STORED)
        z.writestr("META-INF/container.xml", container)
        z.writestr("score.xml", xml)
