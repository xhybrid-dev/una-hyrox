#!/usr/bin/env python3
"""X Training: the three plans as a PDF for review.

    python3 hybridx-training/Tools/review_pdf/make_review_pdf.py [out.pdf]

Everything in it comes from the repository, so it cannot drift from the app:
  - the plans as a coach edits them, from Tools/Plans/*.plan;
  - the nine plans as the watch would build them (three runners in each plan), from the
    golden files in Tests/Host/golden/ (written by plan_print, the app's own code, and
    compared with it by the host tests);
  - the paces, session types and level caps, from docs/PLANS.md and Tools/Plans/README.md.

Default output: hybridx-training/docs/PLANS_REVIEW.pdf. Needs reportlab.
"""

import datetime
import os
import re
import subprocess
import sys

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4, landscape
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.platypus import (BaseDocTemplate, Frame, KeepTogether, PageBreak, PageTemplate,
                                Paragraph, Spacer, Table, TableStyle)
from reportlab.platypus.tableofcontents import TableOfContents

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
GOLDEN = os.path.join(ROOT, "Tests", "Host", "golden")
PLANS_DIR = os.path.join(ROOT, "Tools", "Plans")
PLANS_MD = os.path.join(ROOT, "docs", "PLANS.md")
README_MD = os.path.join(PLANS_DIR, "README.md")

PAGE = landscape(A4)
MARGIN = 14 * mm
USABLE_W = PAGE[0] - 2 * MARGIN

TEAL = colors.HexColor("#1f4e5f")
TEAL_LIGHT = colors.HexColor("#e6eff2")
KEY_FILL = colors.HexColor("#fff1c9")
GRID = colors.HexColor("#b9c6cc")
GREY = colors.HexColor("#5a666c")

PLANS = [
    ("10k", "10K plan", "10k.plan", "10 weeks. Race: a 10K."),
    ("half", "Half-marathon plan", "half.plan", "12 weeks. Race: a half marathon."),
    ("vo2max", "VO2max block", "vo2max.plan", "8 weeks. No race: it opens and closes with a 5K time trial."),
]
RUNNERS = [
    "5k-20min-level3-5days",
    "5k-25min-level2-4days",
    "5k-32min-level1-3days",
]
DAYS_LONG = {"Mon": "Monday", "Tue": "Tuesday", "Wed": "Wednesday", "Thu": "Thursday",
             "Fri": "Friday", "Sat": "Saturday", "Sun": "Sunday"}

warnings = []


# --- text ------------------------------------------------------------------------------------

def clean(text):
    """Markdown source text to plain text the built-in PDF fonts can draw."""
    t = text.replace("`", "").replace("**", "")
    t = t.replace("\u2192", " to ").replace("\u2605", "*").replace("\u2212", "-")
    t = re.sub(r"\[(computed|secondary|convention)[^\]]*\]", "", t)
    # References into the design documents mean nothing to a reader of this PDF alone.
    t = re.sub(r"\s*\((?:C\d+|brief [\d.]+|\d\.\d)\)", "", t)
    t = re.sub(r";? section \d+", "", t)
    t = re.sub(r"within \d\.\d", "within the caps", t)
    t = re.sub(r"\(Billat 2001\)", "(Billat 2001)", t)
    t = re.sub(r"\s+", " ", t).strip()
    for ch in t:
        try:
            ch.encode("cp1252")
        except UnicodeEncodeError:
            warnings.append(f"character {ch!r} (U+{ord(ch):04X}) cannot be drawn: {t[:50]!r}")
    return t.encode("cp1252", "replace").decode("cp1252")


def esc(text):
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


# --- reading the sources ---------------------------------------------------------------------

def md_tables(lines, start):
    """The first markdown table at or after line @p start: (rows, index after it)."""
    i = start
    while i < len(lines) and not lines[i].startswith("|"):
        i += 1
    rows = []
    while i < len(lines) and lines[i].startswith("|"):
        cells = [c.strip() for c in lines[i].strip().strip("|").split("|")]
        if not all(re.fullmatch(r":?-{2,}:?", c) for c in cells):
            rows.append(cells)
        i += 1
    return rows, i


def table_after(path, heading):
    lines = open(path, encoding="utf-8").read().split("\n")
    for i, line in enumerate(lines):
        if line.startswith("#") and heading in line:
            rows, _ = md_tables(lines, i + 1)
            if rows:
                return rows
    raise SystemExit(f"{path}: no table after a heading containing {heading!r}")


def questions(path):
    """The numbered open questions of PLANS.md section 9."""
    lines = open(path, encoding="utf-8").read().split("\n")
    start = next(i for i, l in enumerate(lines) if l.startswith("## 9."))
    out = []
    for line in lines[start + 1:]:
        if line.startswith("## "):
            break
        if re.match(r"\d+\. ", line):
            out.append(re.sub(r"^\d+\. ", "", line).strip())
        elif out and line.startswith("   ") and line.strip():
            out[-1] += " " + line.strip()
    return [clean(q) for q in out]


def parse_golden(path):
    lines = open(path, encoding="utf-8").read().split("\n")
    d = {"title": lines[0][2:].strip(), "options": "", "paces": None, "predicted": "", "calendar": "",
         "weeks": [], "summary": None, "totals": ""}
    i = 1
    while i < len(lines):
        line = lines[i]
        if line.startswith("Options:"):
            d["options"] = line
        elif line.startswith("## Paces"):
            d["paces"], i = md_tables(lines, i + 1)
            continue
        elif line.startswith("Predicted from the 5K"):
            d["predicted"] = line
        elif line.startswith("## Calendar"):
            j = i + 1
            while j < len(lines) and not lines[j].strip():
                j += 1
            d["calendar"] = lines[j]
        elif line.startswith("## Week "):
            m = re.match(r"## Week (\d+) \(([^)]*)\), from (.*)", line)
            rows, i = md_tables(lines, i + 1)
            totals = ""
            j = i
            while j < len(lines) and not lines[j].strip():
                j += 1
            if j < len(lines) and lines[j].startswith("Week:"):
                totals = lines[j]
                i = j
            d["weeks"].append({"n": int(m.group(1)), "phase": m.group(2), "from": m.group(3),
                               "rows": rows[1:], "totals": totals})
        elif line.startswith("## The whole plan"):
            d["summary"], i = md_tables(lines, i + 1)
            j = i
            while j < len(lines) and not lines[j].strip():
                j += 1
            d["totals"] = lines[j] if j < len(lines) else ""
            continue
        i += 1
    return d


def parse_plan_file(path):
    plan = {"weeks": [], "long": {}, "floor": None, "trial": None, "recent": None, "name": "", "race": 0}
    for raw in open(path, encoding="utf-8"):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        word, _, rest = line.partition(" ")
        rest = rest.strip()
        if word in ("week", "trialweek", "recentweek"):
            parts = [p.strip() for p in rest.split("|")]
            w = {"phase": parts[0], "slots": parts[1:]}
            if word == "week":
                plan["weeks"].append(w)
            else:
                plan["trial" if word == "trialweek" else "recent"] = w
        elif word == "long":
            lv, a, b = rest.split()
            plan["long"][int(lv)] = (int(a), int(b))
        elif word == "floor":
            plan["floor"] = [int(x) for x in rest.split()]
        elif word == "name":
            plan["name"] = rest
        elif word == "race":
            plan["race"] = float(rest)
    return plan


# --- styles ----------------------------------------------------------------------------------

base = getSampleStyleSheet()
S = {
    "title": ParagraphStyle("title", parent=base["Title"], fontName="Helvetica-Bold", fontSize=28, leading=32,
                            textColor=TEAL, alignment=0, spaceAfter=4),
    "subtitle": ParagraphStyle("subtitle", parent=base["Normal"], fontSize=13, leading=17, textColor=GREY),
    "h1": ParagraphStyle("h1", parent=base["Heading1"], fontName="Helvetica-Bold", fontSize=18, leading=22,
                         textColor=TEAL, spaceBefore=0, spaceAfter=6),
    "h2": ParagraphStyle("h2", parent=base["Heading2"], fontName="Helvetica-Bold", fontSize=13, leading=16,
                         textColor=TEAL, spaceBefore=10, spaceAfter=4),
    "cover2": ParagraphStyle("cover2", parent=base["Heading2"], fontName="Helvetica-Bold", fontSize=13,
                             leading=16, textColor=TEAL, spaceBefore=10, spaceAfter=4),
    "h3": ParagraphStyle("h3", parent=base["Heading3"], fontName="Helvetica-Bold", fontSize=10.5, leading=13,
                         textColor=colors.black, spaceBefore=8, spaceAfter=3),
    "body": ParagraphStyle("body", parent=base["Normal"], fontSize=9.5, leading=13),
    "small": ParagraphStyle("small", parent=base["Normal"], fontSize=8.5, leading=11, textColor=GREY),
    "bullet": ParagraphStyle("bullet", parent=base["Normal"], fontSize=9.5, leading=13, leftIndent=12,
                             bulletIndent=2, spaceAfter=2),
    "cell": ParagraphStyle("cell", parent=base["Normal"], fontSize=7.6, leading=9.4),
    "cellb": ParagraphStyle("cellb", parent=base["Normal"], fontName="Helvetica-Bold", fontSize=7.6, leading=9.4),
    "cellc": ParagraphStyle("cellc", parent=base["Normal"], fontSize=8, leading=10, alignment=TA_CENTER),
    "head": ParagraphStyle("head", parent=base["Normal"], fontName="Helvetica-Bold", fontSize=7.8, leading=9.6,
                           textColor=colors.white),
    "headc": ParagraphStyle("headc", parent=base["Normal"], fontName="Helvetica-Bold", fontSize=7.8, leading=9.6,
                            textColor=colors.white, alignment=TA_CENTER),
}


def P(text, style="body"):
    return Paragraph(text, S[style])


def cell(text, style="cell"):
    t = esc(clean(text))
    t = t.replace("(key)", "<b>(key)</b>")
    return Paragraph(t, S[style])


def make_table(rows, widths, header=True, center_from=None, key_rows=(), spans=(), zebra=True):
    data = []
    for r, row in enumerate(rows):
        out = []
        for c, text in enumerate(row):
            if header and r == 0:
                out.append(cell(text, "headc" if center_from is not None and c >= center_from else "head"))
            elif center_from is not None and c >= center_from:
                out.append(cell(text, "cellc"))
            else:
                out.append(cell(text))
        data.append(out)
    t = Table(data, colWidths=widths, repeatRows=1 if header else 0)
    st = [("VALIGN", (0, 0), (-1, -1), "TOP"),
          ("GRID", (0, 0), (-1, -1), 0.4, GRID),
          ("LEFTPADDING", (0, 0), (-1, -1), 3.5), ("RIGHTPADDING", (0, 0), (-1, -1), 3.5),
          ("TOPPADDING", (0, 0), (-1, -1), 2.5), ("BOTTOMPADDING", (0, 0), (-1, -1), 2.5)]
    if header:
        st.append(("BACKGROUND", (0, 0), (-1, 0), TEAL))
    if zebra:
        for r in range(1 if header else 0, len(rows)):
            if (r % 2) == 0:
                st.append(("BACKGROUND", (0, r), (-1, r), colors.HexColor("#f4f7f8")))
    for r in key_rows:
        st.append(("BACKGROUND", (0, r), (-1, r), KEY_FILL))
    for sp in spans:
        st.append(("SPAN",) + sp)
    t.setStyle(TableStyle(st))
    return t


def pct(widths_mm):
    total = sum(widths_mm)
    return [USABLE_W * w / total for w in widths_mm]


# --- document --------------------------------------------------------------------------------

class Doc(BaseDocTemplate):
    def __init__(self, path, **kw):
        super().__init__(path, pagesize=PAGE, leftMargin=MARGIN, rightMargin=MARGIN, topMargin=14 * mm,
                         bottomMargin=14 * mm, title="X Training: the three plans, for review",
                         author="HybridX", subject="Draft training plans for review", **kw)
        frame = Frame(self.leftMargin, self.bottomMargin, self.width, self.height, id="f", leftPadding=0,
                      rightPadding=0, topPadding=0, bottomPadding=0)
        self.addPageTemplates([PageTemplate(id="main", frames=[frame], onPage=self.footer)])
        self._n = 0

    def beforeDocument(self):
        # multiBuild runs several passes: the bookmark keys must be the same each time, or the
        # table of contents never settles.
        self._n = 0

    def footer(self, canv, doc):
        canv.saveState()
        canv.setFont("Helvetica", 7.5)
        canv.setFillColor(GREY)
        canv.drawString(MARGIN, 7 * mm, "X Training: the three plans, for review")
        canv.drawRightString(PAGE[0] - MARGIN, 7 * mm, f"Page {doc.page}")
        canv.restoreState()

    def afterFlowable(self, flowable):
        if not isinstance(flowable, Paragraph):
            return
        name = flowable.style.name
        level = {"h1": 0, "h2": 1, "h3": 2}.get(name)
        if level is None:
            return
        text = flowable.getPlainText()
        key = f"h{self._n}"
        self._n += 1
        self.canv.bookmarkPage(key)
        self.canv.addOutlineEntry(text, key, level=level, closed=level > 0)
        if level < 2:
            self.notify("TOCEntry", (level, text, self.page, key))


def git_hash():
    try:
        return subprocess.check_output(["git", "-C", ROOT, "rev-parse", "--short", "HEAD"],
                                       stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return "unknown"


def runner_title(title):
    """'Half marathon plan: 5K 25:00, level 2, Tue Wed Fri Sun, long run Sun' to a heading."""
    m = re.match(r"(.*) plan: 5K (\d+:\d+), level (\d), ([A-Za-z ]+), long run (\w+)", title)
    if not m:
        return title
    _, five, level, days, long_day = m.groups()
    names = ", ".join(DAYS_LONG[d] for d in days.split())
    n = len(days.split())
    return (f"Runner: 5K {five}, level {level}, {n} runs a week ({names}); long run on "
            f"{DAYS_LONG[long_day]}")


def build(out):
    story = []
    today = datetime.date.today()
    stamp = f"{today.day} {today.strftime('%B %Y')}"

    # -- cover ------------------------------------------------------------------------------
    story += [Spacer(1, 22 * mm), P("X Training", "title"),
              P("The three training plans, for review", "subtitle"), Spacer(1, 4 * mm),
              P(f"{stamp}. Generated from commit {git_hash()} of the repository. Draft: the content is "
                f"yours to correct.", "small"), Spacer(1, 8 * mm)]
    story += [P("What this document is", "cover2"),
              P("X Training is the running app for the UNA watch that coaches you through a built-in plan: a "
                "10K plan, a half-marathon plan and a VO2max block. You enter a recent 5K (and optionally a "
                "half) on the phone; the watch works out the paces, the calendar and each day's workout, and "
                "talks you through it step by step."),
              Spacer(1, 2 * mm),
              P("This document shows what the review needs: <b>the plans as a coach edits them</b>, and "
                "<b>the nine plans as the watch would actually build them</b> (each plan for three different "
                "runners), day by day, with every step and its target pace. The nine are produced by the app's "
                "own code, so what you read here is what a runner would be given."),
              Spacer(1, 4 * mm)]
    toc = TableOfContents()
    toc.levelStyles = [
        ParagraphStyle("toc0", fontName="Helvetica-Bold", fontSize=10.5, leading=15, leftIndent=0,
                       firstLineIndent=0),
        ParagraphStyle("toc1", fontName="Helvetica", fontSize=9, leading=12.5, leftIndent=14, firstLineIndent=0,
                       textColor=GREY),
    ]
    story += [PageBreak(), P("Contents", "cover2"), toc, PageBreak()]

    # -- what to look at --------------------------------------------------------------------
    story.append(P("1. What to look at", "h1"))
    story.append(P("You are reviewing the content as a coach. The code that builds the sessions is tested; "
                   "whether the sessions are <i>right</i> is your call.", "body"))
    story.append(Spacer(1, 3 * mm))
    for b in [
        "<b>Progression.</b> Does each plan build the way you would? Look at the long run, the quality minutes "
        "and the cut-back weeks (every fourth week) in the weekly summaries.",
        "<b>The key sessions</b> (shaded, marked <b>(key)</b>): is each week's key session the right one to "
        "protect when a runner can only run three days, or misses a session?",
        "<b>Easy days.</b> Easy steps alert only for \"too fast\", never \"too slow\". Right for a coach?",
        "<b>Levels.</b> Level 1 has no fast repetition sessions (hills instead), shorter intervals and smaller "
        "caps; level 3 has more. The three runners shown are the extremes and the middle.",
        "<b>Time trials and the taper.</b> A 5K time trial ends each cut-back week (the 10K plan has a second "
        "one two weeks out; the half has a 10K four weeks out). The last week is race week.",
        "<b>The half-marathon long runs.</b> They are scaled so the longest reaches at least 16, 18 or 20 km "
        "(levels 1, 2, 3): the slowest runner's long runs are longer in minutes for that reason.",
    ]:
        story.append(Paragraph(b, S["bullet"], bulletText="•"))
    story.append(P("Questions for you", "h2"))
    for q in questions(PLANS_MD):
        story.append(Paragraph(esc(q), S["bullet"], bulletText="•"))
    story.append(P("Caveats", "h2"))
    for b in [
        "The training-pace fractions reproduce the Daniels table rows I could find (fitness 40, 45, 51), to "
        "within 3 s/km, but those rows came from secondary sources: Daniels' book itself was not read. Ten rows "
        "from the book are still wanted as test data before a store release.",
        "Times and distances are estimates at the middle of each step's pace band. Two runs of the same session "
        "will differ.",
        "The Quality column in the weekly summaries counts the race itself in race week, which is why that "
        "week's figure is large.",
        "The plans are not medical advice. The app and the store text must say so (stop with pain, skip when ill).",
    ]:
        story.append(Paragraph(esc(b), S["bullet"], bulletText="•"))
    story.append(PageBreak())

    # -- paces and session types -------------------------------------------------------------
    story.append(P("2. How the paces and sessions work", "h1"))
    story.append(P("From the 5K alone", "h2"))
    story.append(P("The paces come from the runner's own 5K, by the Daniels-Gilbert equations (the ones behind "
                   "Daniels' tables), never from a goal time. Per kilometre:", "body"))
    story.append(Spacer(1, 2 * mm))
    rows = table_after(PLANS_MD, "2.3 Examples")
    story.append(make_table(rows, pct([9, 17, 11, 11, 12, 10, 12, 13, 12, 12]), center_from=1))
    story.append(Spacer(1, 2 * mm))
    story.append(P("For slower runners threshold pace meets 10K pace: their 10K lasts over an hour, and threshold "
                   "is roughly one-hour race pace. A half-marathon time, if the runner has one, sets half-marathon "
                   "pace and the easy paces; threshold, 10K, interval and repetition stay with the 5K.", "small"))
    story.append(P("Pace bands and alerts", "h2"))
    story.append(make_table(table_after(PLANS_MD, "2.5 Bands and alerts"), pct([34, 14, 24, 26])))
    story.append(PageBreak())

    story.append(P("The session types", "h2"))
    story.append(P("Each session is a recipe, written as minutes at an intensity rather than a distance, so "
                   "that a 20:00 and a 32:00 runner get the same stimulus. Warm-ups (15 minutes; 10 at level 1) "
                   "and cool-downs (10 minutes) are added by the app.", "body"))
    story.append(P("In the table: WU is the warm-up, CD the cool-down; t is minutes, n the number of reps, s the "
                   "number of sets, d a distance; T, I, 10K and HM are threshold, interval, 10K and half-marathon "
                   "pace; a jog is an easy recovery.", "small"))
    story.append(Spacer(1, 2 * mm))
    story.append(make_table(table_after(PLANS_MD, "3.1 Recipes"), pct([12, 17, 56, 24])))
    story.append(PageBreak())

    story.append(P("Levels and caps", "h2"))
    story.append(P("The plan files are written for level 2 and four runs a week. The app makes levels 1 and 3 "
                   "from them, applies these caps whatever the file says, and places the sessions on the days the "
                   "runner chooses (3 to 6 run days).", "body"))
    story.append(P("The three levels", "h3"))
    story.append(make_table(table_after(PLANS_MD, "4.1 Levels"), pct([24, 25, 25, 26])))
    story.append(P("Limits per session", "h3"))
    caps = table_after(PLANS_MD, "3.3 Limits per session")
    spans = [((1, len(caps) - 1), (3, len(caps) - 1))] if len(caps[-1]) < 4 else []
    caps = [r + [""] * (4 - len(r)) for r in caps]
    story.append(make_table(caps, pct([28, 24, 24, 24]), spans=spans))
    story.append(PageBreak())

    # -- the plans ---------------------------------------------------------------------------
    legend = table_after(README_MD, "The notation")
    for n, (slug, title, planfile, blurb) in enumerate(PLANS, start=3):
        plan = parse_plan_file(os.path.join(PLANS_DIR, planfile))
        story.append(P(f"{n}. {title}", "h1"))
        story.append(P(esc(blurb), "body"))
        story.append(Spacer(1, 2 * mm))
        story.append(P("The plan as it is written (level 2, four runs a week)", "h2"))
        story.append(P("This is the source a coach edits (<font face='Courier'>Tools/Plans/" + planfile
                       + "</font>). The shaded cell in each row is the week's key session. The notation is "
                       "explained at the end of this document.", "small"))
        story.append(Spacer(1, 2 * mm))
        hdr = ["Week", "Phase", "First quality", "Easy run", "Second quality", "Long run"]
        rows = [hdr]
        key_cells = []
        extra = []
        if plan["recent"] or plan["trial"]:
            pass
        for i, w in enumerate(plan["weeks"], start=1):
            row = [str(i), w["phase"]] + [s.lstrip("*").strip() for s in w["slots"]]
            rows.append(row)
            for c, s in enumerate(w["slots"]):
                if s.startswith("*"):
                    key_cells.append((c + 2, len(rows) - 1))
        t = make_table(rows, pct([7, 11, 26, 14, 28, 26]), zebra=True)
        t.setStyle(TableStyle([("BACKGROUND", (c, r), (c, r), KEY_FILL) for c, r in key_cells]
                              + [("FONTNAME", (c, r), (c, r), "Helvetica-Bold") for c, r in key_cells]))
        story.append(t)
        for label, w in (("When the runner's 5K is not recent, week 1 is instead", plan["trial"]),
                         ("When it is recent, week 1 is instead", plan["recent"])):
            if w:
                story.append(Spacer(1, 1.5 * mm))
                story.append(P(esc(label) + ": " + esc(" | ".join(s.lstrip("*").strip() for s in w["slots"])),
                               "small"))
        longs = ", ".join(f"level {lv}: {a} to {b} min" for lv, (a, b) in sorted(plan["long"].items()))
        note = f"Long runs by level (start of the plan to its peak): {longs}."
        if plan["floor"]:
            note += (f" The peak long run is also scaled to reach at least {plan['floor'][0] // 1000}, "
                     f"{plan['floor'][1] // 1000} and {plan['floor'][2] // 1000} km (levels 1, 2, 3).")
        story.append(Spacer(1, 1.5 * mm))
        story.append(P(esc(note), "small"))
        story.append(PageBreak())

        for run in RUNNERS:
            g = parse_golden(os.path.join(GOLDEN, f"{slug}-{run}.md"))
            story.append(P(esc(f"{title}: " + runner_title(g["title"]).replace("Runner: ", "")), "h2"))
            info = [g["options"], g["calendar"]]
            story.append(P(esc(clean(" ".join(x for x in info if x))), "small"))
            story.append(Spacer(1, 2 * mm))
            paces = g["paces"]
            story.append(make_table([["Paces per km"] + paces[0],
                                     [""] + paces[1]], pct([13, 13, 11, 10, 11, 10, 11, 11, 10]),
                                    center_from=1, zebra=False))
            story.append(Spacer(1, 1 * mm))
            story.append(P(esc(clean(g["predicted"])), "small"))
            story.append(Spacer(1, 2 * mm))
            story.append(P("Easy, warm-up and cool-down steps alert for \"too fast\" only. A step marked "
                           "\"(fast: about ...)\" or \"(judged at the end)\" has a pace shown but never an alert "
                           "while it runs.", "small"))
            story.append(Spacer(1, 3 * mm))
            for w in g["weeks"]:
                head = P(esc(f"Week {w['n']} ({w['phase']}), from {w['from']}"), "h3")
                rows = [["Day", "Session", "Steps", "About"]]
                key_rows = []
                for r in w["rows"]:
                    steps = r[2].replace(" (easy: too fast only)", "")
                    rows.append([r[0], r[1], steps, r[3]])
                    if "(key)" in r[1]:
                        key_rows.append(len(rows) - 1)
                tbl = make_table(rows, pct([13, 36, 105, 21]), key_rows=key_rows, zebra=False)
                foot = P(esc(clean(w["totals"])), "small")
                story.append(KeepTogether([head, tbl, Spacer(1, 1 * mm), foot]))
            story.append(P("The whole plan", "h3"))
            story.append(make_table(g["summary"], pct([10, 22, 10, 16, 18, 18]), center_from=2))
            story.append(Spacer(1, 1.5 * mm))
            story.append(P(esc(clean(g["totals"])), "small"))
            story.append(PageBreak())

    # -- notation ----------------------------------------------------------------------------
    story.append(P("Notation in the plan files", "h1"))
    story.append(P("A plan file has a header, then one line per week: a phase and four slots separated by "
                   "<font face='Courier'>|</font>. Exactly one session in a week is marked "
                   "<font face='Courier'>*</font> (the key session). Rep lengths can be <font face='Courier'>"
                   "4:30</font> or <font face='Courier'>90s</font>; an optional <font face='Courier'>jog 2:00"
                   "</font> after a rep session sets the recovery.", "body"))
    story.append(Spacer(1, 3 * mm))
    story.append(make_table(legend, pct([20, 56, 24])))
    story.append(Spacer(1, 4 * mm))
    story.append(P("To change a plan, edit the <font face='Courier'>.plan</font> file and tell me, or run "
                   "<font face='Courier'>gen_plans.py</font> yourself (<font face='Courier'>"
                   "Tools/Plans/README.md</font> has the steps). The generator refuses an edit that breaks a "
                   "rule (a week without a key session, a hard long run beside two quality sessions, and so on).",
                   "body"))

    doc = Doc(out)
    doc.multiBuild(story)


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "docs", "PLANS_REVIEW.pdf")
    build(out)
    for w in sorted(set(warnings)):
        print("warning:", w, file=sys.stderr)
    print("wrote", out)
