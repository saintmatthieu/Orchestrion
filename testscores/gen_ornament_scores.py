"""Generates the ornament test scores (MusicXML) next to this script.

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
         dots=0):
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
    def __init__(self, title, key_fifths=0, first_tempo=60):
        self.title = title
        self.key = key_fifths
        self.first_tempo = first_tempo
        self.bars = []  # (label, rh_xml, lh_xml, extra_directions)

    def bar(self, label, rh, lh=None, tempo_bpm=None):
        """Appends a case: its lead-in bar (carrying the tempo change, if any),
        then the case bar. rh: xml of the right hand (may be shorter than the
        bar: rests fill). lh: xml of the left hand, default a whole note C3."""
        lead_rh, lead_lh = lead_in()
        self.bars.append((None, lead_rh, lead_lh, tempo(tempo_bpm) if tempo_bpm else ""))
        used = duration_of(rh)
        rh = rh + rests_to_fill(used)
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
               "<identification><creator type=\"composer\">Orchestrion test</creator></identification>",
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
