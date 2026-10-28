"""Generates the ornament and articulation test scores (MusicXML) next to
this script.

Run: python3 testscores/gen_ornament_scores.py

Piano, two staves, 4/4, divisions 8 (32nd = 1, 16th = 2, 8th = 4, quarter = 8,
half = 16, whole = 32). Each case takes two bars: a lead-in bar of four
crotchets in both hands, so that the tempo estimate has settled, then the
labelled case bar, where the right hand has the ornamented note followed by a
plain note and the left hand a whole note.
"""
import os

DIV = 8
TYPES = {1: "32nd", 2: "16th", 4: "eighth", 8: "quarter", 16: "half", 32: "whole"}
STEP_ALTER = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}


def pitch_xml(p):
    """p = 'C5', 'Eb4', 'F#5'."""
    step = p[0]
    alter = 0
    rest = p[1:]
    while rest and rest[0] in "#b":
        alter += 1 if rest[0] == "#" else -1
        rest = rest[1:]
    octave = int(rest)
    s = f"<pitch><step>{step}</step>"
    if alter:
        s += f"<alter>{alter}</alter>"
    s += f"<octave>{octave}</octave></pitch>"
    return s


def note(p, dur, staff=1, voice=None, ornaments="", tie=None, chord=False, accidental=None,
         dots=0, arpeggiate=None, articulations=""):
    voice = voice or (1 if staff == 1 else 5)
    typ = TYPES[dur if not dots else dur * 2 // 3]
    s = "<note>"
    if chord:
        s += "<chord/>"
    s += pitch_xml(p)
    s += f"<duration>{dur}</duration>"
    if tie in ("start", "both"):
        s += '<tie type="start"/>'
    if tie in ("stop", "both"):
        s += '<tie type="stop"/>'
    s += f"<voice>{voice}</voice><type>{typ}</type>"
    if dots:
        s += "<dot/>"
    if accidental:
        s += f"<accidental>{accidental}</accidental>"
    s += f"<staff>{staff}</staff>"
    notations = ""
    if tie in ("start", "both"):
        notations += '<tied type="start"/>'
    if tie in ("stop", "both"):
        notations += '<tied type="stop"/>'
    if ornaments:
        notations += f"<ornaments>{ornaments}</ornaments>"
    if arpeggiate:
        notations += arpeggiate
    if articulations:
        notations += f"<articulations>{articulations}</articulations>"
    if notations:
        s += f"<notations>{notations}</notations>"
    s += "</note>"
    return s


def grace(p, staff=1, slash=False, after=False, typ="eighth", chord=False):
    voice = 1 if staff == 1 else 5
    attrs = ' slash="yes"' if slash else ""
    if after:
        attrs += ' steal-time-previous="20"'
    s = f"<note>{'<chord/>' if chord else ''}<grace{attrs}/>{pitch_xml(p)}"
    s += f"<voice>{voice}</voice><type>{typ}</type><staff>{staff}</staff></note>"
    return s


def rest(dur, staff=1):
    voice = 1 if staff == 1 else 5
    return f"<note><rest/><duration>{dur}</duration><voice>{voice}</voice><type>{TYPES[dur]}</type><staff>{staff}</staff></note>"


def words(text, staff=1):
    return (f'<direction placement="above"><direction-type><words>{text}</words>'
            f"</direction-type><staff>{staff}</staff></direction>")


def tempo(bpm):
    return (f'<direction placement="above"><direction-type><metronome><beat-unit>quarter</beat-unit>'
            f"<per-minute>{bpm}</per-minute></metronome></direction-type><sound tempo=\"{bpm}\"/></direction>")


def rests_to_fill(used, staff=1):
    """Rests completing a bar of 32 from `used`, largest first."""
    out = []
    left = 32 - used
    assert left >= 0, used
    for d in (16, 8, 4, 2, 1):
        while left >= d:
            out.append(rest(d, staff))
            left -= d
    return "".join(out)


def duration_of(xml):
    """Sum of the <duration> of non-grace notes in an xml fragment (crude)."""
    import re
    total = 0
    for m in re.finditer(r"<note>(.*?)</note>", xml):
        body = m.group(1)
        if "<grace" in body or "<chord/>" in body:
            continue
        d = re.search(r"<duration>(\d+)</duration>", body)
        total += int(d.group(1))
    return total


def lead_in():
    """A bar of four crotchets in each hand, to get into the tempo."""
    rh = "".join(note("C5", 8) for _ in range(4))
    lh = "".join(note("C3", 8, staff=2) for _ in range(4))
    return rh, lh


class Score:
    def __init__(self, title, key_fifths=0, first_tempo=60, staff_modes=None):
        self.title = title
        self.key = key_fifths
        self.first_tempo = first_tempo
        self.staff_modes = staff_modes  # e.g. "notes,notes": Orchestrion's staff modes
        self.bars = []  # (label, rh_xml, lh_xml, extra_directions)

    def bar(self, label, rh, lh=None, tempo_bpm=None, rh2=None):
        """Appends a case: its lead-in bar (carrying the tempo change, if any),
        then the case bar. rh: xml of the right hand (may be shorter than the
        bar: rests fill). lh: xml of the left hand, default a whole note C3.
        rh2: xml of a second right-hand voice (voice 2), a whole bar of it."""
        lead_rh, lead_lh = lead_in()
        self.bars.append((None, lead_rh, lead_lh, tempo(tempo_bpm) if tempo_bpm else ""))
        used = duration_of(rh)
        rh = rh + rests_to_fill(used)
        if rh2:
            assert duration_of(rh2) == 32
            rh += "<backup><duration>32</duration></backup>" + rh2
        if lh is None:
            lh = note("C3", 32, staff=2)
        else:
            lh = lh + rests_to_fill(duration_of(lh), staff=2)
        self.bars.append((label, rh, lh, ""))

    def xml(self):
        out = ['<?xml version="1.0" encoding="UTF-8"?>',
               '<!DOCTYPE score-partwise PUBLIC "-//Recordare//DTD MusicXML 4.0 Partwise//EN" "http://www.musicxml.org/dtds/partwise.dtd">',
               '<score-partwise version="4.0">',
               f"<work><work-title>{self.title}</work-title></work>",
               "<identification><creator type=\"composer\">Orchestrion test</creator>"
               + (f'<miscellaneous><miscellaneous-field name="orchestrion-staff-modes">{self.staff_modes}'
                  "</miscellaneous-field></miscellaneous>" if self.staff_modes else "")
               + "</identification>",
               '<part-list><score-part id="P1"><part-name>Piano</part-name></score-part></part-list>',
               '<part id="P1">']
        case = 0
        for i, (label, rh, lh, extra) in enumerate(self.bars, start=1):
            out.append(f'<measure number="{i}">')
            if i == 1:
                out.append(f"<attributes><divisions>{DIV}</divisions><key><fifths>{self.key}</fifths></key>"
                           "<time><beats>4</beats><beat-type>4</beat-type></time><staves>2</staves>"
                           '<clef number="1"><sign>G</sign><line>2</line></clef>'
                           '<clef number="2"><sign>F</sign><line>4</line></clef></attributes>')
                out.append(tempo(self.first_tempo))
            if extra:
                out.append(extra)
            if label is not None:
                case += 1
                out.append(words(f"{case}. {label}"))
            out.append(rh)
            out.append("<backup><duration>32</duration></backup>")
            out.append(lh)
            if i == len(self.bars):
                out.append('<barline location="right"><bar-style>light-heavy</bar-style></barline>')
            out.append("</measure>")
        out.append("</part></score-partwise>")
        return "\n".join(out) + "\n"

    def write(self, path):
        with open(path, "w") as f:
            f.write(self.xml())
        print("wrote", path, len(self.bars) // 2, "cases")


TR = "<trill-mark/>"
TURN = "<turn/>"
INV_TURN = "<inverted-turn/>"
MORDENT = "<mordent/>"           # lower mordent (MuseScore: mordent)
SHORT_TRILL = "<inverted-mordent/>"  # pralltriller (MuseScore: short trill)

outdir = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------- 1. trills
s = Score("Ornaments 1 - Trills")
s.bar("tr on a 16th: no room for three 32nds: C D C compressed, then D", note("C5", 2, ornaments=TR) + note("D5", 2) + note("E5", 4))
s.bar("tr on an 8th: 32nds", note("C5", 4, ornaments=TR) + note("D5", 4))
s.bar("tr on a quarter", note("C5", 8, ornaments=TR) + note("D5", 8))
s.bar("tr on a half: cycles until the next key", note("C5", 16, ornaments=TR) + note("D5", 16))
s.bar("tr + termination written as graces before D: they resolve the trill, 32nds", note("C5", 16, ornaments=TR)
      + grace("B4", typ="16th") + grace("C5", typ="16th") + note("D5", 16))
s.bar("tr + Nachschlag as graces after C: 32nds at the leaving gesture",
      note("C5", 16, ornaments=TR) + grace("B4", after=True, typ="16th") + grace("C5", after=True, typ="16th") + note("D5", 16))
s.bar("tr then turn: the turn waits for the trill note to end", note("C5", 8, ornaments=TR) + note("D5", 8, ornaments=TURN) + note("E5", 8))
s.bar("tr then release into a rest", note("C5", 8, ornaments=TR) + rest(8) + note("D5", 8))
s.bar("tr with a wavy line over tied notes (nominal = half + quarter)",
      note("C5", 16, ornaments=TR + '<wavy-line type="start"/>', tie="start")
      + note("C5", 8, tie="stop", ornaments='<wavy-line type="stop"/>') + note("D5", 8))
s.bar("tr on a two-note chord", note("C5", 8, ornaments=TR) + note("E5", 8, chord=True) + note("D5", 8))
s.bar("tr with a sharp: upper note D sharp", note("C5", 8, ornaments=TR + "<accidental-mark>sharp</accidental-mark>") + note("D5", 8))
s.bar("tr on B: upper note a semitone away", note("B4", 8, ornaments=TR) + note("C5", 8))
s.bar("tr in the left hand (E3, right hand plain)", note("C5", 8) + note("D5", 8),
      lh=note("E3", 8, staff=2, ornaments=TR) + note("D3", 8, staff=2))
s.bar("both hands trill together", note("C5", 8, ornaments=TR) + note("D5", 8),
      lh=note("E3", 8, staff=2, ornaments=TR) + note("D3", 8, staff=2))
s.bar("plain quarter to end", note("C5", 8) + note("D5", 8))
s.write(os.path.join(outdir, "ornaments-1-trills.musicxml"))

# ------------------------------------------------------- 2. turns & mordents
s = Score("Ornaments 2 - Turns and mordents")
s.bar("turn on an 8th: no room for five 32nds: C D C B C evenly, into D", note("C5", 4, ornaments=TURN) + note("D5", 4))
s.bar("turn on a quarter: 32nds, then hold", note("C5", 8, ornaments=TURN) + note("D5", 8))
s.bar("turn on a half: 32nds, then hold", note("C5", 16, ornaments=TURN) + note("D5", 16))
s.bar("inverted turn on an 8th: C B C D C", note("C5", 4, ornaments=INV_TURN) + note("D5", 4))
s.bar("mordent on an 8th: C B C as 32nds, then hold", note("C5", 4, ornaments=MORDENT) + note("D5", 4))
s.bar("mordent on a half: 32nds, then hold", note("C5", 16, ornaments=MORDENT) + note("D5", 16))
s.bar("short trill on a 16th: 3 notes", note("C5", 2, ornaments=SHORT_TRILL) + note("D5", 2) + note("E5", 4))
s.bar("short trill on an 8th: C D C as 32nds, then hold", note("C5", 4, ornaments=SHORT_TRILL) + note("D5", 4))
s.bar("short trill on a half: C D C as 32nds, then hold", note("C5", 16, ornaments=SHORT_TRILL) + note("D5", 16))
s.bar("turn on a 32nd: too short, plays plain", note("C5", 1, ornaments=TURN) + note("D5", 1) + note("E5", 2) + note("F5", 4))
s.bar("turn with a flat below: lower note B flat", note("C5", 4, ornaments=TURN + '<accidental-mark placement="below">flat</accidental-mark>') + note("D5", 4))
s.bar("acciaccatura + turn on the same note", grace("B4", slash=True) + note("C5", 8, ornaments=TURN) + note("D5", 8))
s.bar("turn on E: upper F is a semitone, lower D a tone", note("E5", 4, ornaments=TURN) + note("F5", 4))
s.bar("mordent in the left hand", note("C5", 8) + note("D5", 8),
      lh=note("E3", 8, staff=2, ornaments=MORDENT) + note("D3", 8, staff=2))
s.bar("plain quarter to end", note("C5", 8) + note("D5", 8))
s.write(os.path.join(outdir, "ornaments-2-turns-and-mordents.musicxml"))

# ------------------------------------------------------------ 3. grace notes
s = Score("Ornaments 3 - Grace notes")
s.bar("acciaccatura B, then C (a 64th)", grace("B4", slash=True) + note("C5", 8) + note("D5", 8))
s.bar("two acciaccaturas A B, then C (64ths)", grace("A4", slash=True) + grace("B4", slash=True) + note("C5", 8) + note("D5", 8))
s.bar("appoggiatura D (8th) before a half C: its written value, an 8th", grace("D5") + note("C5", 16) + note("D5", 16))
s.bar("appoggiatura D written as a 16th before a half C: a 16th", grace("D5", typ="16th") + note("C5", 16) + note("D5", 16))
s.bar("three acciaccaturas G A B, then C (64ths)", grace("G4", slash=True) + grace("A4", slash=True) + grace("B4", slash=True) + note("C5", 8) + note("D5", 8))
s.bar("three unslashed 16th graces G B E (Chopin bar 13): 64ths", grace("G4", typ="16th") + grace("B4", typ="16th") + grace("E5", typ="16th") + note("G5", 8) + note("D5", 8))
s.bar("grace after C (Nachschlag D): a 64th at the leaving gesture", note("C5", 16) + grace("D5", after=True) + note("E5", 16))
s.bar("two graces after C (D E): 64ths, then F", note("C5", 16) + grace("D5", after=True, typ="16th") + grace("E5", after=True, typ="16th") + note("F5", 16))
s.bar("grace chord B+D before chord C+E", grace("B4", slash=True) + grace("D5", slash=True, chord=True)
      + note("C5", 8) + note("E5", 8, chord=True) + note("D5", 8))
s.bar("acciaccatura in the left hand", note("C5", 8) + note("D5", 8),
      lh=grace("D3", staff=2, slash=True) + note("E3", 8, staff=2) + note("D3", 8, staff=2))
s.bar("acciaccatura before a tied note (nominal = half + half)", grace("B4", slash=True) + note("C5", 16, tie="start") + note("C5", 16, tie="stop"))
s.bar("plain quarter to end", note("C5", 8) + note("D5", 8))
s.write(os.path.join(outdir, "ornaments-3-grace-notes.musicxml"))

# ------------------------------------------------------------------ 4. tempo
# Ornament notes are 32nds (crushed graces 64ths) at the performer's tempo,
# floored at 40 ms, so the same figures scale with the tempo.
s = Score("Ornaments 4 - Tempo", first_tempo=40)
for bpm in (40, 90, 160):
    ms32 = 7500 / bpm  # a 32nd, in ms
    s.bar(f"q = {bpm}: turn on an 8th (five notes of {int(3750 / bpm)} ms)", note("C5", 4, ornaments=TURN) + note("D5", 4),
          tempo_bpm=bpm if bpm != 40 else None)
    s.bar(f"q = {bpm}: tr on a quarter (32nds of {ms32:.0f} ms)", note("C5", 8, ornaments=TR) + note("D5", 8))
    s.bar(f"q = {bpm}: short trill on an 8th (C D C as 32nds of {ms32:.0f} ms)", note("C5", 4, ornaments=SHORT_TRILL) + note("D5", 4))
    s.bar(f"q = {bpm}: three 16th graces (64ths of {ms32 / 2:.0f} ms)", grace("G4", typ="16th") + grace("B4", typ="16th") + grace("E5", typ="16th") + note("G5", 8) + note("D5", 8))
    too_fast = 1875 / bpm < 40  # five notes in a 16th
    s.bar(f"q = {bpm}: turn on a 16th (five notes of {1875 / bpm:.0f} ms{': too fast, plays plain' if too_fast else ''})",
          note("C5", 2, ornaments=TURN) + note("D5", 2) + note("E5", 4))
s.bar("play these bars faster or slower than written: the ornaments should follow", note("C5", 8, ornaments=TR) + note("D5", 8, ornaments=TURN) + note("E5", 8, ornaments=MORDENT) + note("F5", 8, ornaments=SHORT_TRILL))
s.bar("same again", note("C5", 8, ornaments=TR) + note("D5", 8, ornaments=TURN) + note("E5", 8, ornaments=MORDENT) + note("F5", 8, ornaments=SHORT_TRILL))
s.bar("plain quarter to end", note("C5", 8) + note("D5", 8))
s.write(os.path.join(outdir, "ornaments-4-tempo.musicxml"))

# ------------------------------------------------- 5. trills, followed
# Both staves in the Notes mode, the follower following them merged: what
# matters is what else is struck while the right hand trills.
def triad(root, third, fifth, dur):
    return (note(root, dur, staff=2) + note(third, dur, staff=2, chord=True)
            + note(fifth, dur, staff=2, chord=True))


s = Score("Ornaments 5 - Trills, followed", staff_modes="notes,notes")
s.bar("tr on a whole note over a whole-note chord: nothing struck meanwhile",
      note("D5", 32, ornaments=TR), lh=triad("G2", "B2", "D3", 32))
s.bar("tr on a whole note over a changing chord on each beat (G C D G)",
      note("D5", 32, ornaments=TR),
      lh=triad("G2", "B2", "D3", 8) + triad("C3", "E3", "G3", 8) + triad("D3", "F#3", "A3", 8)
      + triad("G2", "B2", "D3", 8))
s.bar("tr on a whole note over the same chord on each beat: only the timing tells them apart",
      note("D5", 32, ornaments=TR),
      lh="".join(triad("G2", "B2", "D3", 8) for _ in range(4)))
s.bar("tr on a half, then D, over 8th-note chords",
      note("D5", 16, ornaments=TR) + note("C5", 16),
      lh="".join(triad("G2", "B2", "D3", 4) for _ in range(8)))
s.bar("tr in the upper voice while the lower voice of the same hand moves in quarters",
      note("D5", 32, ornaments=TR),
      rh2="".join(note(p, 8, voice=2) for p in ("B4", "A4", "G4", "A4")))
s.bar("plain quarter to end", note("C5", 8) + note("D5", 8))
s.write(os.path.join(outdir, "ornaments-5-trills-followed.musicxml"))

# ------------------------------------------------------------- 6. tremolos
def tremolo(marks=3, unmeasured=False):
    """A single-note tremolo: `marks` slashes (3 = 32nds), or unmeasured (a
    buzz roll to MuseScore, which takes it only without a count)."""
    if unmeasured:
        return '<tremolo type="unmeasured">0</tremolo>'
    return f'<tremolo type="single">{marks}</tremolo>'


def tremolo_pair(first, second, dur, marks=3, staff=1):
    """A two-note tremolo alternating `first` and `second` (pitches, or lists
    of them for chords) for `dur`: both written at that value, each lasting
    half of it."""
    voice = 1 if staff == 1 else 5
    out = []
    for pitches, kind in ((first, "start"), (second, "stop")):
        pitches = pitches if isinstance(pitches, list) else [pitches]
        for i, p in enumerate(pitches):
            s = "<note>" + ("<chord/>" if i else "") + pitch_xml(p)
            s += f"<duration>{dur // 2}</duration><voice>{voice}</voice><type>{TYPES[dur]}</type>"
            s += "<time-modification><actual-notes>2</actual-notes><normal-notes>1</normal-notes></time-modification>"
            s += f"<staff>{staff}</staff>"
            if i == 0:
                s += f'<notations><ornaments><tremolo type="{kind}">{marks}</tremolo></ornaments></notations>'
            s += "</note>"
            out.append(s)
    return "".join(out)


s = Score("Ornaments 6 - Tremolos")
s.bar("single-note tremolo on a half (C repeated as 32nds), then D",
      note("C5", 16, ornaments=tremolo()) + note("D5", 16))
s.bar("single-note tremolo on a chord (C E G repeated as 32nds), then D",
      note("C5", 16, ornaments=tremolo()) + note("E5", 16, chord=True) + note("G5", 16, chord=True)
      + note("D5", 16))
s.bar("two-note tremolo C-E over a half (alternating as 32nds), then D",
      tremolo_pair("C5", "E5", 16) + note("D5", 16))
s.bar("two-note tremolo between octaves C4 and C5 over a whole note (broken octaves, 16ths)",
      tremolo_pair("C4", "C5", 32, marks=2))
s.bar("two-note tremolo between chords C-E and G-C over a half, then D",
      tremolo_pair(["C5", "E5"], ["G5", "C6"], 16) + note("D5", 16))
s.bar("unmeasured tremolo on a half: as many as the player likes, then D",
      note("C5", 16, ornaments=tremolo(unmeasured=True)) + note("D5", 16))
s.bar("two-note tremolo in the left hand (C3-G3 over a whole note) under right-hand quarters",
      "".join(note(p, 8) for p in ("E5", "D5", "C5", "D5")),
      lh=tremolo_pair("C3", "G3", 32, staff=2))
s.bar("plain quarter to end", note("C5", 8) + note("D5", 8))
s.write(os.path.join(outdir, "ornaments-6-tremolos.musicxml"))

# ------------------------------------------------------------ 7. arpeggios
def arpeggiate(direction=None, number=1):
    """The arpeggio mark of a note of an arpeggiated chord: `direction` up,
    down or None (a plain wavy line), or "bracket" for a non-arpeggio."""
    if direction == "bracket":
        return f'<non-arpeggiate type="bottom" number="{number}"/>'
    d = f' direction="{direction}"' if direction else ""
    return f'<arpeggiate number="{number}"{d}/>'


def chord(pitches, dur, staff=1, mark=None):
    """A chord of `pitches`, every note carrying `mark` (see arpeggiate)."""
    return "".join(note(p, dur, staff=staff, chord=i > 0, arpeggiate=mark)
                   for i, p in enumerate(pitches))


C_MAJOR = ["C4", "E4", "G4", "C5"]
s = Score("Ornaments 7 - Arpeggios")
s.bar("arpeggio upwards on a chord (C E G C, 64ths at your tempo), then D",
      chord(C_MAJOR, 16, mark=arpeggiate("up")) + note("D5", 16))
s.bar("arpeggio downwards, then D",
      chord(C_MAJOR, 16, mark=arpeggiate("down")) + note("D5", 16))
s.bar("plain arpeggio (a wavy line): upwards, then D",
      chord(C_MAJOR, 16, mark=arpeggiate()) + note("D5", 16))
s.bar("arpeggio across both staves (C3 G3 + E4 G4 C5): the left hand plays it all, then D",
      chord(["E4", "G4", "C5"], 16, mark=arpeggiate("up")) + note("D5", 16),
      lh=chord(["C3", "G3"], 16, staff=2, mark=arpeggiate("up")) + note("C3", 16, staff=2))
s.bar("non-arpeggio bracket: the notes together, then D",
      chord(C_MAJOR, 16, mark=arpeggiate("bracket")) + note("D5", 16))
s.bar("arpeggiated quarter-note chords in a row (C, F, G, C): release early to cut the roll",
      chord(["C4", "E4", "G4", "C5"], 8, mark=arpeggiate("up"))
      + chord(["C4", "F4", "A4", "C5"], 8, mark=arpeggiate("up"))
      + chord(["B3", "D4", "G4", "B4"], 8, mark=arpeggiate("up"))
      + chord(["C4", "E4", "G4", "C5"], 8, mark=arpeggiate("up")))
s.bar("plain quarter to end", note("C5", 8) + note("D5", 8))
s.write(os.path.join(outdir, "ornaments-7-arpeggios.musicxml"))

# ======================================================== articulations
# How long the machine (the automatic player, a Follow staff, the automatic
# playing of a Notes staff) holds an articulated chord: a staccato half its
# written value, a staccatissimo a quarter; with an accent, a marcato or a
# tenuto, the average of the marks (counted as the whole note): an accented
# staccato or a portato three quarters, an accented staccatissimo five eighths.
# Never less than a 64th; a staccato on a note tied on is held. Each case plays
# the articulated notes, then the same notes plain, to hear them against each
# other.
STACC = "<staccato/>"
STACCISS = "<staccatissimo/>"   # MuseScore: staccatissimo
SPICC = "<spiccato/>"           # MuseScore: staccatissimo stroke
PORTATO = "<detached-legato/>"  # MuseScore: tenuto-staccato, one mark
TENUTO = "<tenuto/>"
ACCENT = "<accent/>"
MARCATO = "<strong-accent/>"


def run(pitches, dur, staff=1, voice=None, articulations=""):
    return "".join(note(p, dur, staff=staff, voice=voice, articulations=articulations)
                   for p in pitches)


def then_plain(pitches, dur, articulations, staff=1):
    """The notes articulated, then the same notes plain."""
    return (run(pitches, dur, staff=staff, articulations=articulations)
            + run(pitches, dur, staff=staff))


def st_chord(pitches, dur, articulations="", staff=1):
    return "".join(note(p, dur, staff=staff, chord=i > 0,
                        articulations=articulations if i == 0 else "")
                   for i, p in enumerate(pitches))


# ------------------------------------------ 1. staccatos (Rhythm, automatic)
s = Score("Articulations 1 - Staccatos")
s.bar("staccato 8ths, then plain: half of each", then_plain(["C5", "D5", "E5", "F5"], 4, STACC))
s.bar("staccato quarters, then plain", then_plain(["C5", "D5"], 8, STACC))
s.bar("staccatissimo quarters, then plain: a quarter of each", then_plain(["C5", "D5"], 8, STACCISS))
s.bar("staccatissimo stroke (spiccato) quarters, then plain", then_plain(["C5", "D5"], 8, SPICC))
s.bar("portato (one mark) quarters, then plain: three quarters of each", then_plain(["C5", "D5"], 8, PORTATO))
s.bar("portato (tenuto + staccato marks) quarters, then plain", then_plain(["C5", "D5"], 8, TENUTO + STACC))
s.bar("accent + staccato, marcato + staccato, then plain: three quarters",
      run(["C5"], 8, articulations=ACCENT + STACC) + run(["D5"], 8, articulations=MARCATO + STACC)
      + run(["C5", "D5"], 8))
s.bar("accent + staccatissimo quarters, then plain: five eighths", then_plain(["C5", "D5"], 8, ACCENT + STACCISS))
s.bar("tenuto quarters, then plain: both held in full", then_plain(["C5", "D5"], 8, TENUTO))
s.bar("staccato quarter tied to a quarter: held in full; then a staccato half: half",
      note("C5", 8, tie="start", articulations=STACC) + note("C5", 8, tie="stop")
      + run(["D5"], 16, articulations=STACC))
s.bar("staccatissimo 32nds, then plain: floored at a 64th",
      then_plain(["C5", "D5", "E5", "F5"], 1, STACCISS) + rest(8) + rest(16))
s.bar("staccato chords (C E G), then plain",
      st_chord(["C4", "E4", "G4"], 8, STACC) + st_chord(["D4", "F4", "A4"], 8, STACC)
      + st_chord(["C4", "E4", "G4"], 8) + st_chord(["D4", "F4", "A4"], 8))
s.bar("left-hand staccato 8ths under a whole note (as in the Rondo alla turca)",
      note("E5", 32), lh=run(["A2", "C3", "E3", "C3"], 4, staff=2, articulations=STACC)
      + run(["A2", "C3", "E3", "C3"], 4, staff=2))
s.bar("graces A C E into an accented staccato 8th, then staccato 8ths (Rondo alla turca, "
      "bar 25, left hand)",
      note("E5", 32),
      lh=grace("A2", staff=2, typ="32nd") + grace("C3", staff=2, typ="32nd") + grace("E3", staff=2, typ="32nd")
      + run(["A3"], 4, staff=2, articulations=STACC + ACCENT)
      + run(["A3", "A3", "A3"], 4, staff=2, articulations=STACC)
      + run(["A3", "A3", "A3", "A3"], 4, staff=2))
s.bar("mordent on a staccato quarter: the last of its notes let go early; then plain",
      note("C5", 8, ornaments=MORDENT, articulations=STACC) + note("D5", 8, ornaments=MORDENT)
      + run(["C5", "D5"], 8))
s.bar("staccato quarters over a half note in another voice of the hand: the hand lets go "
      "of both together, the half at the staccato's release",
      run(["G4", "A4", "G4", "A4"], 8, articulations=STACC),
      rh2=note("E4", 16, voice=2) + note("E4", 16, voice=2))
s.bar("plain quarter to end", note("C5", 8) + note("D5", 8))
s.write(os.path.join(outdir, "articulations-1-staccatos.musicxml"))

# ---------------------------------- 2. staccatos on a Follow staff (left)
# Play the right hand: the left follows, letting go of its articulated
# chords early.
s = Score("Articulations 2 - Staccatos, followed", staff_modes="rhythm,follow")
s.bar("left-hand staccato 8ths, then plain, under right-hand quarters",
      run(["C5", "D5", "E5", "F5"], 8),
      lh=then_plain(["C3", "E3", "G3", "E3"], 4, STACC, staff=2))
s.bar("left-hand staccatissimo quarters, then plain",
      run(["C5", "D5", "E5", "F5"], 8), lh=then_plain(["C3", "G3"], 8, STACCISS, staff=2))
s.bar("left-hand portato quarters, then plain",
      run(["C5", "D5", "E5", "F5"], 8), lh=then_plain(["C3", "G3"], 8, PORTATO, staff=2))
s.bar("plain quarter to end", note("C5", 8) + note("D5", 8))
s.write(os.path.join(outdir, "articulations-2-staccatos-followed.musicxml"))

# ------------------------------- 3. staccatos on Notes staves (automatic)
# Press Space: the automatic player plays the Notes staves' notes as keys.
s = Score("Articulations 3 - Staccatos, Notes mode", staff_modes="notes,notes")
s.bar("staccato 8ths, then plain", then_plain(["C5", "D5", "E5", "F5"], 4, STACC))
s.bar("staccatissimo quarters, then plain", then_plain(["C5", "D5"], 8, STACCISS))
s.bar("portato quarters, then plain", then_plain(["C5", "D5"], 8, PORTATO))
s.bar("left-hand staccato 8ths under a whole note",
      note("E5", 32), lh=then_plain(["A2", "C3", "E3", "C3"], 4, STACC, staff=2))
s.bar("plain quarter to end", note("C5", 8) + note("D5", 8))
s.write(os.path.join(outdir, "articulations-3-staccatos-notes-mode.musicxml"))
