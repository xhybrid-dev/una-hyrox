#!/usr/bin/env python3
"""What-if replay of a HybridX Run tuning log (vo2_*.csv): what the estimate
would have been under other window rules, how it moves with max/resting HR, and (P3) what
the HR-lag guard and the halves would give.
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
    rows.append(dict(i=int(p[0]),t=int(p[1]),good=int(p[2]),out=p[3],v=float(p[4]),sp=float(p[5]),g=float(p[6]),hr=float(p[7]),hmin=int(p[8]),hmax=int(p[9]),cost=float(p[10]),
                     lag=(int(p[12]) if len(p)>12 else None)))
# P3: the HR-lag flag, as Vo2Run::judge works it out (logs from before P3 lack the column): the
# window before was not the minute before, had no speed, or was more than 10% off this one's pace.
prev=None
for r in rows:
    sp=r['v'] if r['good']>0 else 0.0
    flag=True
    if prev and r['t']-prev[0]==60 and prev[1]>0 and sp>0:
        flag=abs(sp-prev[1])*100.0/prev[1]>10.0
    prev=(r['t'],sp)
    if r['lag'] is None: r['lag']=1 if flag else 0
    r['lag_calc']=1 if flag else 0
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
# ---- P3, measured first: what the guards would do (the app logs the same in its "# what-if" line) ----
lagged=[r for r in kept if r['lag']]
print('P3 after a change of pace (kept):',[r['i'] for r in lagged])
if any(r['lag']!=r['lag_calc'] for r in rows):
    print('  note: the logged lag column and this script disagree on',[r['i'] for r in rows if r['lag']!=r['lag_calc']])
clean=[r for r in kept if not r['lag']]
print('P3 lag guard on: %d windows, median %s'%(len(clean),('%.1f'%med(clean)) if len(clean)>=5 else 'too few'))
qs=sorted(vo2(r) for r in kept)
if qs:
    n=len(qs); print('P3 spread (half the interquartile range): %.1f'%((qs[(3*n)//4]-qs[n//4])/2))

