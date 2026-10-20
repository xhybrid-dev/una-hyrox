#!/usr/bin/env python3
"""Tune Trail Quest's balance (Q4): move each stage's spirit stamina softening
(kSpiritPct in data/Stages.hpp) and each boss's own staminaPct (data/Species.hpp)
towards the casual-win targets of docs/BALANCE.md, one step per iteration, then
print the table. Rewrites those two files; review the diff and re-run
balance_sim 10000 and run_sim afterwards (and fix any stragglers by hand: it
tunes stages and bosses, not single regular spirits).

    python3 trailquest/docs/experiments/tune_balance.py [samples=4000] [iterations=1]

Needs the host tests built (cmake -S trailquest/Tests/Host -B trailquest/build-tests).
"""
import csv, re, subprocess, sys, pathlib, tempfile
TQ = pathlib.Path(__file__).resolve().parents[2]
SPEC = TQ / 'Software/Libs/Core/Header/data/Species.hpp'
STG = TQ / 'Software/Libs/Core/Header/data/Stages.hpp'
OUT = pathlib.Path(tempfile.gettempdir()) / 'trailquest_tune_bal.csv'
N = sys.argv[1] if len(sys.argv) > 1 else '4000'
ITER = int(sys.argv[2]) if len(sys.argv) > 2 else 1
REG, BOSS = 87.0, 75.0

def run():
    subprocess.run(['cmake','--build',str(TQ/'build-tests'),'--target','balance_sim','-j8'],check=True,capture_output=True)
    subprocess.run([str(TQ/'build-tests/balance_sim'),N,'--csv',str(OUT)],check=True,capture_output=True)
    return list(csv.DictReader(open(OUT)))

def get_stage_pcts():
    m = re.search(r'kSpiritPct\[kStageCount\] = \{([^}]*)\}', STG.read_text())
    return [int(x) for x in m.group(1).split(',')]

def set_stage_pcts(v):
    s = STG.read_text()
    s = re.sub(r'(kSpiritPct\[kStageCount\] = \{)[^}]*(\})', r'\1 ' + ', '.join(map(str,v)) + r' \2', s)
    STG.write_text(s)

def get_pct(name):
    m = re.search(r'\{ "%s", [^\n]*\n\s+\d, Affinity::\w+, (\d+),' % re.escape(name), SPEC.read_text())
    return int(m.group(1))

def set_pct(name, v):
    s = SPEC.read_text()
    s, n = re.subn(r'(\{ "%s", [^\n]*\n\s+\d, Affinity::\w+, )(\d+)(,)' % re.escape(name), lambda m: m.group(1)+str(v)+m.group(3), s)
    assert n == 1, name
    SPEC.write_text(s)

for it in range(ITER):
    rows = run()
    sp = get_stage_pcts()
    for st in range(1, 5):
        regs = [int(r['casual_win_pct']) for r in rows if int(r['stage']) == st+1 and r['boss'] == '0']
        mean = sum(regs)/len(regs)
        sp[st] = max(55, min(115, round(sp[st] + 0.5*(mean-REG))))
    set_stage_pcts(sp)
    for r in rows:
        if r['boss'] == '1':
            w = int(r['casual_win_pct'])
            p = get_pct(r['spirit'])
            set_pct(r['spirit'], max(60, min(220, round(p + 0.6*(w-BOSS)))))
    print('iter', it, 'stage pcts', sp, flush=True)
rows = run()
for r in rows:
    print('%-24s st%s casual %s%% turns %.1f good %s%% menu %s%% %s' % (r['spirit'], r['stage'], r['casual_win_pct'], int(r['casual_turns_x10'])/10, r['good_win_pct'], r['menu_win_pct'], r['check']))
