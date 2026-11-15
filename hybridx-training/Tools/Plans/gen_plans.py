#!/usr/bin/env python3
"""X Training: turn the plan files into Software/Libs/Core/Header/data/PlanData.hpp.

    python3 gen_plans.py          write the header
    python3 gen_plans.py --check  fail (exit 1) if the header is not what the files make

The plan files are the source; the header is never edited by hand. The
notation is in README.md. Every rule the files must keep is checked here, with
the file and line of anything wrong, so a coach's edit cannot build a plan the
app would place badly (PLANS.md section 4.2) or that breaks C8 (a hard long run
leaves room for one other quality session).
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.normpath(os.path.join(HERE, "..", "..", "Software", "Libs", "Core", "Header",
                                    "data", "PlanData.hpp"))
FILES = ["10k.plan", "half.plan", "vo2max.plan"]

PHASES = {
    "base": "Base", "build": "Build", "peak": "Peak", "specific": "Specific", "taper": "Taper",
    "race": "RaceWeek", "cutback": "CutBack", "baseline": "Baseline", "retest": "Retest",
}
HARD = {"LongFinish", "Tempo", "Cruise", "Intervals", "ThirtyThirty", "Hills", "Reps",
        "Pace10k", "PaceHalf", "TimeTrial5k", "TimeTrial10k", "Race"}
EASY_SLOT = {"Easy", "EasyStrides"}
LONG_SLOT = {"Long", "LongFinish", "TimeTrial5k", "TimeTrial10k", "Race"}


class PlanError(Exception):
    pass


def fail(where, msg):
    raise PlanError(f"{where}: {msg}")


def parse_seconds(tok, where, minutes_default):
    """'90s' -> 90, '2:00' -> 120, '3' -> 180 (minutes) or 3 (seconds)."""
    m = re.fullmatch(r"(\d+)s", tok)
    if m:
        return int(m.group(1))
    m = re.fullmatch(r"(\d+):([0-5]\d)", tok)
    if m:
        return int(m.group(1)) * 60 + int(m.group(2))
    m = re.fullmatch(r"\d+", tok)
    if m:
        return int(tok) * 60 if minutes_default else int(tok)
    fail(where, f"not a time: '{tok}'")


def parse_session(text, where):
    """One slot. Returns (session dict, is_key)."""
    text = text.strip()
    key = text.startswith("*")
    if key:
        text = text[1:].strip()
    words = text.split()
    if not words:
        fail(where, "an empty slot")
    jog = 0
    if "jog" in words:
        i = words.index("jog")
        if i + 2 != len(words):
            fail(where, "'jog' must come last, with one time after it")
        jog = parse_seconds(words[i + 1], where, minutes_default=True)
        words = words[:i]
    code, args = words[0], words[1:]
    s = {"kind": None, "reps": 0, "a": 0, "b": 0, "jog": jog}

    def minutes(i):
        if len(args) <= i or not args[i].isdigit():
            fail(where, f"{code} needs minutes")
        v = int(args[i])
        if not 1 <= v <= 240:
            fail(where, f"{code} {v}: minutes out of range")
        return v

    def reps_by(unit):
        if len(args) != 1:
            fail(where, f"{code} needs one 'NxT' (e.g. 4x4)")
        m = re.fullmatch(r"(\d+)x(.+)", args[0])
        if not m:
            fail(where, f"{code}: '{args[0]}' is not NxT")
        n = int(m.group(1))
        if not 1 <= n <= 30:
            fail(where, f"{code}: {n} reps out of range")
        t = m.group(2)
        if unit == "m":
            mm = re.fullmatch(r"(\d+)m", t)
            if not mm:
                fail(where, f"{code}: '{t}' is not a distance like 400m")
            v = int(mm.group(1))
            if not 100 <= v <= 2000:
                fail(where, f"{code}: {v} m out of range")
        elif unit == "count":
            if not t.isdigit():
                fail(where, f"{code}: '{t}' is not a count")
            v = int(t)
            if not 1 <= v <= 30:
                fail(where, f"{code}: {v} out of range")
        else:
            v = parse_seconds(t, where, minutes_default=True)
            if not 10 <= v <= 3600:
                fail(where, f"{code}: {t} out of range")
        return n, v

    if code == "E":
        s["kind"], s["a"] = "Easy", minutes(0)
    elif code == "E+S":
        s["kind"], s["a"] = "EasyStrides", minutes(0)
    elif code == "L":
        s["a"] = minutes(0)
        if len(args) == 1:
            s["kind"] = "Long"
        elif len(args) == 3 and args[1] == "HM":
            s["kind"], s["b"] = "LongFinish", minutes(2)
            if s["b"] >= s["a"]:
                fail(where, "the fast finish must be shorter than the run")
        else:
            fail(where, "L takes minutes, optionally 'HM <minutes>'")
    elif code == "TEMPO":
        s["kind"], s["a"] = "Tempo", minutes(0)
    elif code in ("CRUISE", "INT", "RP10", "RPHM"):
        s["kind"] = {"CRUISE": "Cruise", "INT": "Intervals", "RP10": "Pace10k",
                     "RPHM": "PaceHalf"}[code]
        s["reps"], s["a"] = reps_by("time")
    elif code == "30/30":
        s["kind"] = "ThirtyThirty"
        s["reps"], s["a"] = reps_by("count")
    elif code == "HILLS":
        s["kind"] = "Hills"
        s["reps"], s["a"] = reps_by("time")
    elif code == "REPS":
        s["kind"] = "Reps"
        s["reps"], s["a"] = reps_by("m")
    elif code in ("TT5", "TT10", "SHAKE", "RACE"):
        if args:
            fail(where, f"{code} takes nothing after it")
        s["kind"] = {"TT5": "TimeTrial5k", "TT10": "TimeTrial10k", "SHAKE": "Shakeout",
                     "RACE": "Race"}[code]
    else:
        fail(where, f"unknown session '{code}'")
    if jog and s["kind"] not in ("Cruise", "Intervals", "Pace10k", "PaceHalf"):
        fail(where, f"'jog' does not apply to {code}")
    return s, key


def parse_week(text, where):
    parts = [p.strip() for p in text.split("|")]
    if len(parts) != 5:
        fail(where, "a week is a phase and four slots, separated by '|'")
    phase = parts[0]
    if phase not in PHASES:
        fail(where, f"unknown phase '{phase}'")
    slots, keys = [], []
    for i, p in enumerate(parts[1:]):
        s, k = parse_session(p, where)
        slots.append(s)
        if k:
            keys.append(i)
    if len(keys) != 1:
        fail(where, "each week marks exactly one key session with '*'")
    if slots[1]["kind"] not in EASY_SLOT:
        fail(where, "the easy slot (the third column) must be E or E+S")
    if slots[3]["kind"] not in LONG_SLOT:
        fail(where, "the long slot must be L, a time trial or RACE")
    if slots[3]["kind"] == "LongFinish":
        hard = [s for s in (slots[0], slots[2]) if s["kind"] in HARD]
        if len(hard) > 1:
            fail(where, "a hard long run leaves room for one other quality session (C8)")
    if slots[keys[0]]["kind"] not in HARD:
        fail(where, "the key session must be a quality session, time trial or race")
    for i in (0, 2):
        if slots[i]["kind"] in ("Long", "LongFinish", "Race"):
            fail(where, "a quality slot cannot hold a long run or the race")
    return {"phase": PHASES[phase], "key": keys[0], "slots": slots}


def parse_file(path):
    name = os.path.basename(path)
    plan = {"file": name, "longs": {}, "floor": [0, 0, 0], "weeks": [], "trial": None,
            "recent": None}
    with open(path, encoding="utf-8") as f:
        for n, raw in enumerate(f, 1):
            where = f"{name}:{n}"
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            word, _, rest = line.partition(" ")
            rest = rest.strip()
            if word == "plan":
                plan["slug"] = rest
            elif word == "id":
                plan["id"] = int(rest)
            elif word == "name":
                if not re.fullmatch(r"[A-Za-z0-9 ]{1,20}", rest):
                    fail(where, "the name is 1-20 letters, digits and spaces")
                plan["name"] = rest
            elif word == "race":
                plan["raceCm"] = int(round(float(rest) * 100))
            elif word == "minweeks":
                plan["minWeeks"] = int(rest)
            elif word == "long":
                lv, start, peak = (int(x) for x in rest.split())
                if lv not in (1, 2, 3) or not 20 <= start < peak <= 150:
                    fail(where, "long <level 1-3> <start> <peak>, 20 <= start < peak <= 150")
                plan["longs"][lv] = (start, peak)
            elif word == "floor":
                vals = [int(x) for x in rest.split()]
                if len(vals) != 3:
                    fail(where, "floor takes three distances in metres")
                plan["floor"] = vals
            elif word == "week":
                plan["weeks"].append(parse_week(rest, where))
            elif word == "trialweek":
                plan["trial"] = parse_week(rest, where)
            elif word == "recentweek":
                plan["recent"] = parse_week(rest, where)
            else:
                fail(where, f"unknown line '{word}'")
    for k in ("slug", "id", "name", "raceCm", "minWeeks"):
        if k not in plan:
            fail(name, f"missing '{k}'")
    if sorted(plan["longs"]) != [1, 2, 3]:
        fail(name, "needs a 'long' line for each of levels 1, 2 and 3")
    weeks = plan["weeks"]
    if not 4 <= len(weeks) <= 16:
        fail(name, "a plan has 4 to 16 weeks")
    if not 1 <= plan["minWeeks"] <= len(weeks):
        fail(name, "minweeks must be between 1 and the plan's length")
    last = weeks[-1]["slots"][3]["kind"]
    if plan["raceCm"] > 0 and last != "Race":
        fail(name, "a plan with a race ends with RACE in the last week's long slot")
    if plan["raceCm"] == 0 and any(s["kind"] == "Race" for w in weeks for s in w["slots"]):
        fail(name, "RACE in a plan without a race distance")
    for i, w in enumerate(weeks[:-1]):
        if any(s["kind"] == "Race" for s in w["slots"]):
            fail(name, f"week {i + 1}: RACE only in the last week")
    return plan


def session_cpp(s):
    return (f"Session{{Kind::{s['kind']}, {s['reps']}, {s['a']}, {s['b']}, {s['jog']}}}")


def week_cpp(w):
    slots = ", ".join(session_cpp(s) for s in w["slots"])
    return f"Week{{Phase::{w['phase']}, {w['key']}, {{{slots}}}}}"


def ident(slug):
    return "k" + "".join(p.capitalize() for p in re.split(r"[^A-Za-z0-9]", slug) if p)


def generate(plans):
    out = []
    out.append("/**")
    out.append(" * @file    PlanData.hpp")
    out.append(" * @brief   The plans, generated from the .plan files in Tools/Plans by gen_plans.py.")
    out.append(" *")
    out.append(" * DO NOT EDIT: change the plan files and run gen_plans.py. The host test")
    out.append(" * plan-data-current fails if this file and the plan files disagree.")
    out.append(" */")
    out.append("")
    out.append("#ifndef XTRAINING_PLAN_DATA_HPP")
    out.append("#define XTRAINING_PLAN_DATA_HPP")
    out.append("")
    out.append('#include "../PlanTypes.hpp"')
    out.append("")
    out.append("namespace XTraining {")
    out.append("namespace PlanData {")
    out.append("")
    for p in plans:
        base = ident(p["slug"])
        out.append(f"// {p['file']}")
        out.append(f"inline constexpr Week {base}Weeks[] = {{")
        for i, w in enumerate(p["weeks"]):
            out.append(f"    {week_cpp(w)},  // week {i + 1}")
        out.append("};")
        for which in ("trial", "recent"):
            if p[which] is not None:
                out.append(f"inline constexpr Week {base}{which.capitalize()}Week = "
                           f"{week_cpp(p[which])};")
        out.append("")
    out.append(f"inline constexpr PlanDef kPlans[] = {{")
    for p in plans:
        base = ident(p["slug"])
        longs = ", ".join(f"LongRange{{{p['longs'][lv][0]}, {p['longs'][lv][1]}}}"
                          for lv in (1, 2, 3))
        floor = ", ".join(str(v) for v in p["floor"])
        trial = f"&{base}TrialWeek" if p["trial"] else "nullptr"
        recent = f"&{base}RecentWeek" if p["recent"] else "nullptr"
        pid = {1: "TenK", 2: "Half", 3: "Vo2max"}[p["id"]]
        out.append(f"    PlanDef{{PlanId::{pid}, \"{p['name']}\", {len(p['weeks'])}, "
                   f"{p['minWeeks']}, {p['raceCm']}u,")
        out.append(f"            {{{longs}}},")
        out.append(f"            {{{floor}}},")
        out.append(f"            {base}Weeks, {trial}, {recent}}},")
    out.append("};")
    out.append("")
    out.append("inline constexpr uint8_t kPlanCount = sizeof(kPlans) / sizeof(kPlans[0]);")
    out.append("")
    out.append("/// The plan with @p id, or null.")
    out.append("inline const PlanDef* find(PlanId id)")
    out.append("{")
    out.append("    for (const PlanDef& p : kPlans) {")
    out.append("        if (p.id == id) {")
    out.append("            return &p;")
    out.append("        }")
    out.append("    }")
    out.append("    return nullptr;")
    out.append("}")
    out.append("")
    out.append("}  // namespace PlanData")
    out.append("}  // namespace XTraining")
    out.append("")
    out.append("#endif  // XTRAINING_PLAN_DATA_HPP")
    return "\n".join(out) + "\n"


def main(argv):
    check = "--check" in argv
    try:
        plans = [parse_file(os.path.join(HERE, f)) for f in FILES]
        ids = [p["id"] for p in plans]
        if sorted(ids) != [1, 2, 3]:
            raise PlanError("the plans' ids must be 1, 2 and 3")
        plans.sort(key=lambda p: p["id"])
        text = generate(plans)
    except PlanError as e:
        print(f"gen_plans: {e}", file=sys.stderr)
        return 2
    if check:
        try:
            with open(OUT, encoding="utf-8") as f:
                current = f.read()
        except OSError:
            current = ""
        if current != text:
            print(f"gen_plans: {OUT} is out of date: run gen_plans.py", file=sys.stderr)
            return 1
        print("gen_plans: PlanData.hpp is current")
        return 0
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        f.write(text)
    print(f"gen_plans: wrote {OUT}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
