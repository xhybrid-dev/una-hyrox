#!/usr/bin/env python3
"""What-if replay of a HybridX Run tuning log (vo2_*.csv): what the estimate
would have been under other window rules, and how it moves with max/resting HR.
Not the app's code: the app's own numbers are in the log's last line.

    python3 -I vo2_log_whatif.py [--max-hr N] [--rest-hr N] vo2_YYYYMMDDTHHMMSS.csv
"""
import statistics as st, sys
args=sys.argv[1:]
MAX=182; REST=41
if "--max-hr" in args:
    k=args.index("--max-hr"); MAX=int(args[k+1]); del args[k:k+2]
if "--rest-hr" in args:
    k=args.index("--rest-hr"); REST=int(args[k+1]); del args[k:k+2]
f=args[0]
rows=[]
for l in open(f):
    if l.startswith('#') or l.startswith('window'): continue
    p=l.strip().split(',')
    if len(p)<11: continue
    rows.append(dict(i=int(p[0]),t=int(p[1]),good=int(p[2]),out=p[3],v=float(p[4]),sp=float(p[5]),g=float(p[6]),hr=float(p[7]),hmin=int(p[8]),hmax=int(p[9]),cost=float(p[10])))
def vo2(r,mx=MAX):
    hrr=(r['hr']-REST)/(mx-REST)
    return 3.5+(r['cost']-3.5)/hrr
def med(rs,mx=MAX): return st.median(vo2(r,mx) for r in rs)
kept=[r for r in rows if r['out']=='kept']
print('kept',len(kept),'median %.1f'%med(kept))
vs=[vo2(r) for r in kept]
print('per-window range %.1f-%.1f  mean %.1f sd %.1f'%(min(vs),max(vs),st.mean(vs),st.pstdev(vs)))
h=len(kept)//2
print('first half %.1f  second half %.1f'%(med(kept[:h]),med(kept[h:])))
# scenario: grade by window mean instead of any second
post=[r for r in rows if r['t']>300 and r['good']>=55]
def ok_mean_grade(r): return -3<=r['g']<=10
s2=[r for r in post if ok_mean_grade(r) and r['sp']<=8 and (r['hmax']-r['hmin'])<=10]
print('mean-grade rule: %d windows, median %.1f'%(len(s2),med(s2)), 'added',[r['i'] for r in s2 if r['out']!='kept'])
s3=[r for r in post if ok_mean_grade(r) and r['sp']<=8 and (r['hmax']-r['hmin'])<=12]
print('mean-grade + HR range 12: %d windows, median %.1f'%(len(s3),med(s3)), 'added',[r['i'] for r in s3 if r['out']!='kept'])
print('grade-rejected:',[(r['i'],r['g'],r['v']) for r in rows if r['out']=='grade'])
print('unsteady:',[(r['i'],r['sp'],r['hmax']-r['hmin']) for r in rows if r['out']=='unsteady'])
print('gaps:',[(r['i'],r['good']) for r in rows if r['out']=='gaps'])
for mx in (175,179,182,185,190,195): print('max HR',mx,'-> %.1f'%med(kept,mx))
print('rest HR -> 50:', '%.1f'%st.median(3.5+(r['cost']-3.5)/((r['hr']-50)/(MAX-50)) for r in kept))
print('mean grade of kept %.2f, spread %.1f to %.1f'%(st.mean(r['g'] for r in kept),min(r['g'] for r in kept),max(r['g'] for r in kept)))
