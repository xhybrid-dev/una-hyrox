# HybridX Run: race-time predictions (research, 28 October 2026)

Research only: nothing in the app changes. This report compares the published ways
to predict 5 km, 10 km, half-marathon and marathon times, checks which inputs
HybridX Run already has, and recommends what a later, separately planned phase
should build. The standing rule applies: published, citable methods only.

**How the sources were checked.** This session's network allowed web searches
but not the journal sites themselves (PubMed Central, BMC, PLOS, Nature and
Frontiers all refused the connection). So each figure below comes from a
paper's abstract or from the publisher's or repository's summary, as the
search returned it. Each is marked **[abstract]**, **[secondary]** (a calculator
site, blog or white paper quoting the source) or **[computed]** (worked out
here from a published formula). Before anything is coded, the equations
marked "check against the paper" must be read in the original paper. That
step is open question 1.

---

## 1. In brief

- **Use Riegel's power law (T2 = T1 × (D2/D1)^1.06) on the athlete's best
  recent 5 km**, for **10 km and the half marathon**. On recreational runners it
  is well calibrated up to the half marathon (Vickers and Vertosick 2016), its
  formula is published and simple, and its input (a timed 5 km) is the most
  trustworthy number the app has.
- **Do not show a plain Riegel marathon time.** For recreational runners it is
  too fast: at least 10 minutes too fast for half of them (Vickers and
  Vertosick 2016). The published fix (their model with weekly mileage) is the
  right one. But its coefficients could not be read here, and the app sees only
  the runs that Run and Trail record. Until then, the marathon either stays
  blank or shows as "3:31 or slower".
- **Do not predict from the app's VO2max.** Daniels' VDOT tables take an
  "effective" VO2max on Daniels' own oxygen-cost curve. The app's figure comes
  from the ACSM running equation, which puts a higher cost on the same speed:
  21% higher at 5:00 /km (§4.3). Feeding the app's 45.1 into Daniels' tables
  gives a 3:28 marathon. Converting it onto Daniels' scale first gives 4:00.
  That 32-minute gap is a scale mismatch, not a prediction error. Errors in the
  max HR (about 7% of VO2max per 10 bpm, NOTES "Review") come on top of it.
  VO2max can come back later as a cross-check, once the Garmin comparison
  (Gate R) shows how the app's figure reads.
- **Show one time per distance, rounded to its real precision, with the
  source underneath**: "From your 5 km of 22:00 · 12 Oct". **Show nothing**
  when there is no 5 km (or mile) effort in the last six weeks, or when the
  effort came from a treadmill run.

---

## 2. What HybridX Run has, and how far to trust it

| Input | Where it comes from | Reliability for prediction |
|---|---|---|
| Best 1 km, 1 mile, 5 km inside each run | `RunStats` (P2): a ring of 10 m crossings, so a split ends on a 10 m boundary. The tests read 199.9-201.0 s for a true 200.0 s (±0.5%) | **Timing is good; effort is unknown.** These are the fastest stretches *inside* training runs, not all-out races. Mostly they understate fitness, which makes predictions too slow (a safe direction). GPS distance error goes straight into the time; UNA's corrected speed (R6b) helps, but its error on the watch has not been measured. A downhill stretch flatters a best, and `RunStats` does not track elevation for splits |
| All-time personal bests (being added) | Bests carried across runs | **Wrong input for prediction.** A PB from last year describes last year's fitness. Prediction needs a *recent* best, with its date |
| VO2max (ml/kg/min) | ACSM running equation, %HRR ≈ %VO2R, median of steady minutes, weighted mean of the last 5 runs | **Not on Daniels' scale** (§4.3). Its biggest error is the max HR: 10 bpm too high gives about +7% (NOTES "Review"). Firstbeat says the same of Garmin's estimate: a max HR 15 bpm out gives 7-9% [secondary: Firstbeat white paper]. Still untested against the Garmin (Gate R) |
| Weekly distance | `act_run.json` and `act_trail.json` (the activity link): start, duration and distance of each app's last 16 activities | **Partial.** The link counts only HybridX Run's and Trail's runs. Runs in UNA's own Running app, and all non-running training, are missing. So any mileage-based model would *understate* the athlete's volume |
| Mean training pace (8 weeks) | The same link files | Partial, for the same reason |
| Treadmill runs | Cadence-to-stride model, or the console's distance entered after Save | **Exclude.** The distance is estimated, and treadmill pace is not road pace |

---

## 3. The methods compared

| Method | Input | Predicts | Source | Validated on | Reported accuracy | Usable in Run? |
|---|---|---|---|---|---|---|
| **Riegel power law** | One timed effort | Any distance, about 3.5-230 min | Riegel 1981, *American Scientist* 69:285-290 (first in *Runner's World*, 1977) | World and age-group records (fitted to the best performers) | Recreational runners: well calibrated to the half; marathon at least 10 min too fast for half of runners; MSE 381 for the marathon [abstract: Vickers and Vertosick 2016] | **Yes**: 5 km best → 10 km, half |
| **Vickers and Vertosick** | One or two recent races, plus weekly mileage | Marathon (and the others) | Vickers AJ, Vertosick EA 2016, *BMC Sports Sci Med Rehabil* 8:26 | 2,303 recreational runners (an internet survey; self-reported times; 2:1 split into training and validation sets) | Marathon MSE 228 (one race) and 208 (two races), against 381 for Riegel [abstract] | **Later**: needs the coefficients from the paper and a fuller weekly mileage |
| **Daniels and Gilbert (VDOT)** | One race time, or VDOT | Any distance, about 1,500 m to 50 km | Daniels J, Gilbert J 1979, *Oxygen Power: Performance Tables for Distance Runners*; Daniels' *Running Formula* | Fitted to performance and laboratory data from trained and elite runners | No peer-reviewed validation on recreational runners found. A 2025 study of 8,261 marathoners compared it with a half-marathon model and pointed to "durability" as the reason it is less precise for the marathon [abstract: Frontiers in Physiology 2025] | **Only as a race-to-race table** (as Riegel). **Not from the app's VO2max** (§4.3) |
| **Cameron** | One timed effort | 800 m to the marathon | Dave Cameron, 1990s, published online only (not peer-reviewed) | Fitted to the top 10 times at each distance [secondary] | No validation found | **No**: not a citable, peer-reviewed source |
| **Tanda** | 8 weeks of training: mean weekly km K, mean training pace P | Marathon only | Tanda G 2011, *J Hum Sport Exerc* 6(3):511-520 | 22 runners aged 28-54, 46 marathons of 167-216 min | SEE about 4 min [abstract], valid only inside that range | **No for now**: Run sees only part of the training (§2), and the population is small and narrow |
| **Critical speed from training bests** | Best 400 m, 800 m and 5 km efforts inside 16 weeks of training | Marathon | Smyth B, Muniz-Pumares D 2020, *Med Sci Sports Exerc* 52(12):2637-2645 | More than 25,000 recreational marathoners (training logs before big-city marathons) | Error 7.67% (R 0.695) for the best model; the marathon was run at 84.8% ± 13.6% of critical speed (93.0% for 2:30 runners, 78.9% for 6:00) [abstract] | **Of interest**: the only method built on bests *from training* rather than races. The app does not record 400 m or 800 m bests yet |
| **Emig and Peltonen** | Many sessions (speed and duration) | Any race | Emig T, Peltonen J 2020, *Nature Communications* 11:4936 | About 14,000 people, 1.6 million sessions (Polar data) | About 2% mean error for the marathon [secondary: Polar's blog; check the paper's own metric] | **No**: fitting needs far more data and computing than one watch's history |
| **Local matrix completion** | Several of the runner's own performances, plus a large database | Any race | Blythe DAJ, Király FJ 2016, *PLOS ONE* 11(6):e0157257 | 164,746 British runners (thepowerof10) | 3.6 min on elite marathons; about 30% lower RMSE than earlier methods [abstract] | **No**: needs the database. What it *shows* matters, though: each runner has their own exponent |
| **Firstbeat / Garmin race predictor** | Firstbeat's VO2max estimate | 5 km to the marathon | Firstbeat, proprietary | Not published | No peer-reviewed test against race results found [secondary] | **No**: not published |
| **Equations from training and anthropometry** (114 of them) | Various | Marathon | Reviewed by Keogh A et al. 2019, *Int J Sports Physiol Perform* 14(9):1159-1169 | 36 studies | Reported inconsistently: r² for 68 equations, SEE for 19; no single best equation; few validated outside their own sample [abstract] | Background only |

---

## 4. The formulas

### 4.1 Riegel (1981)

    T2 = T1 × (D2 / D1)^b,   b = 1.06

T is time and D is distance. The exponent of 1.06 is the published figure for
running and the one in general use. The 1981 paper fits a separate exponent for
each group of records. Secondary sources quote 1.07732 for men's running records
and 1.08283 for women's; *check against the paper*. The formula is commonly said
to hold for efforts of 3.5 to 230 minutes [secondary].

How b changes the prediction from a 22:00 5 km [computed]:

| b | 10 km | Half | Marathon |
|---|---|---|---|
| 1.06 | 45:52 | 1:41:12 | 3:31:00 |
| 1.07 | 46:11 | 1:42:40 | 3:35:33 |
| 1.08 | 46:31 | 1:44:10 | 3:40:12 |

So the exponent hardly matters at 10 km (under 1.5%) but moves the marathon
by up to 9 minutes. That is why the marathon is where Riegel fails for
recreational runners: the further the jump in distance, the more an
individual's own exponent (Blythe and Király 2016) outweighs the population's.

**Where it breaks down.**
- Long jumps in distance. From a 1 km best, the marathon is a ×42 extrapolation, and a 3-4 minute effort sits at the edge of the 3.5-minute lower limit.
- Recreational runners at the marathon, where it is too fast (Vickers and Vertosick 2016).
- Runners with little endurance training: low weekly mileage widens the gap.

### 4.2 Vickers and Vertosick (2016)

From the abstract:
- Riegel "was well-calibrated for races up to a half-marathon, but
  dramatically underestimated marathon time, giving times at least 10 min too
  fast for half of runners".
- The authors' marathon models, which add typical weekly mileage, had a mean
  squared error of 228 (one prior race) and 208 (two prior races), against 381
  for Riegel. If the unit is minutes squared, those are root-mean-square errors
  of about 15.1, 14.4 and 19.5 minutes. That reading is ours; *check against
  the paper*.
- Weekly mileage and interval training had similar associations with speed at
  every distance. Tempo runs mattered more for shorter races.

**The model's coefficients are not given here.** They are in the paper's
tables and could not be read in this session. Secondary sources disagree about
them, and one forum post says they are not given explicitly in the text.
Under the standing rule, nothing is guessed: open question 1.

### 4.3 Daniels and Gilbert (1979), and why the app's VO2max does not fit it

The two curves (v is speed in m/min; t is race time in minutes):

    VO2 cost      = −4.60 + 0.182258·v + 0.000104·v²          (ml/kg/min)
    fraction held = 0.8 + 0.1894393·e^(−0.012778·t) + 0.2989558·e^(−0.1932605·t)
    VDOT          = VO2 cost(D/t) / fraction held(t)

A prediction solves for the race time t at which the VDOT equals the
athlete's. Checked here [computed]: VDOT 50 gives 19:56, 41:20, 1:31:31 and
3:10:40. Daniels' published table gives 19:57, 41:21, 1:31:35 and 3:10:49, so
the equations reproduce the tables to within 10 seconds.

**The scale mismatch.** The app's VO2max comes from the ACSM equation, and
ACSM puts a higher cost on the same speed than Daniels does [computed]:

| Speed | Pace | ACSM VO2 (3.5 + 0.2v) | Daniels VO2 cost | ACSM / Daniels |
|---|---|---|---|---|
| 134 m/min | 7:28 /km | 30.3 | 21.7 | 1.40 |
| 200 m/min | 5:00 /km | 43.5 | 36.0 | 1.21 |
| 250 m/min | 4:00 /km | 53.5 | 47.5 | 1.13 |
| 300 m/min | 3:20 /km | 63.5 | 59.4 | 1.07 |

Daniels fitted his curve to trained runners, who are economical. ACSM's curve
describes a typical adult on a treadmill. So, for the same runner, the app's
figure reads higher than a VDOT, and most of all for slower runners.

Putting the app's figure into Daniels' tables as it stands therefore predicts
times that are too fast. One consistent conversion takes two steps:
1. Find the speed at which ACSM's cost equals the app's VO2max:
   v = (VO2max − 3.5) / 0.2.
2. Read that speed's cost off Daniels' curve.

| App VO2max | Speed at VO2max | On Daniels' scale | Marathon: converted | Marathon: as it stands |
|---|---|---|---|---|
| 40.0 | 182 m/min | 32.1 | 4:34 | 3:50 |
| 45.1 (Jon's run, 14 Oct) | 208 m/min | 37.8 | 4:00 | 3:28 |
| 50.0 | 232 m/min | 43.4 | 3:35 | 3:11 |
| 55.0 | 258 m/min | 49.2 | 3:13 | 2:56 |

The conversion is our composition of two published equations, not a
published method. It also inherits every bias of the estimate:
- the max HR;
- the HR lag and cardiac drift (P3);
- running economy, which no field estimate can see.

So VO2max is **not** recommended as a source of predictions. The gap between
the two columns is larger than any error the published methods report.

**Where Daniels breaks down even with a race as input.** It was fitted to
well-trained runners. For the marathon, the 2025 Valencia study (8,261
runners, a half and a full marathon in the same year) found a half-marathon
regression explained 85% of the variance, with a mean absolute error of 5.9%.
It suggests that "durability" is why VDOT is less precise at the marathon
[abstract; VDOT's own error figure was not in the abstract].

### 4.4 Tanda (2011)

    Pm = 17.1 + 140.0·e^(−0.0053·K) + 0.55·P

Pm is the marathon pace in s/km, K the mean weekly distance in km, and P the
mean training pace in s/km, both over the 8 weeks before the race.

Population: 22 runners aged 28-54 and 46 marathons of 167-216 minutes. SEE
about 4 min [abstract]. For example, K = 40 km and P = 5:30 /km give 3:39
[computed]. Outside a 2:47-3:36 marathon the equation is extrapolating. The
app also cannot see the K of anyone who logs runs in UNA's own app.

### 4.5 Critical speed from training (Smyth and Muniz-Pumares 2020)

Critical speed (CS) is the slope of the distance-time line through best
efforts at three or more distances from 400 m to 5 km, taken from training. The
best model used 400 m, 800 m and 5 km, with an error of 7.67%. On average the
marathon was run at 84.8% of CS, but the spread was large: ±13.6%, from 93.0%
for fast runners down to 78.9% for slow ones [abstract].

Its value here is the evidence that bests taken from *training* do carry
prediction power. The ±13.6% also shows how much a fixed fraction leaves to
the individual.

### 4.6 Not recommended, and why

- **Cameron**: not peer-reviewed; coefficients could not be verified.
- **Firstbeat**: proprietary.
- **Emig and Peltonen; Blythe and Király**: they need a fitted model or a large
  database. They are sound evidence, not on-watch methods.
- **Purdy points and similar scoring tables**: they score performances against
  each other; they were not validated as predictions.

---

## 5. Recommendation

### 5.1 Method

1. **Riegel, b = 1.06, from the best recent 5 km**, predicting **10 km** and
   the **half marathon**.
   - If there is no 5 km in the window, use the best **mile** (about 6-10 minutes,
     well inside Riegel's range), for 5 km and 10 km only. The half from a mile is
     a ×13 jump.
   - Never use the 1 km.
   - The 5 km line shows the best 5 km itself, labelled as such, not a prediction.
2. **Marathon: nothing, or a lower bound.** Show "3:31 or slower" (Riegel from
   the 5 km). Vickers and Vertosick found Riegel's marathon too fast for most
   recreational runners, so this is the honest reading of the published
   finding. A real figure waits for their mileage model (open question 1), and
   that model needs a weekly mileage the app can see in full.
3. **VO2max: not used** for predictions in this phase (§4.3). Revisit after
   Gate R, perhaps as a check that flags a 5 km best clearly out of line with
   the VO2max ("your best 5 km looks like an easy run").
4. **Recent bests, not all-time PBs.** Keep each run's three bests with the
   run's date (as `Vo2History` keeps its runs), and predict from the best in
   the window. The all-time PBs being added stay a separate display.

### 5.2 On the round face

A **PREDICT** summary face (or a page under the start screen's menu), four
rows and a source line:

    10 KM      46:00
    HALF     1:41
    MARATHON 3:31+
    from 5 km 22:00 · 12 Oct

- **Round to the precision the evidence supports.** The input alone is good to
  about 1-2% (GPS distance; a split ending on a 10 m boundary), and training
  bests are not race efforts. So:
  - 10 km to the nearest half minute;
  - the half to the minute;
  - the marathon to the minute with "+".
  Seconds would claim a precision no source supports.
- **The source line is the honesty.** It names the effort and its age, so the
  athlete can judge for themselves ("that was an easy run").
- **No ± range in this phase.** No source gives a published error band for
  Riegel below the marathon for this population (Vickers and Vertosick report
  only that it is well calibrated). A band would be invented. If one is
  wanted, the spread between the mile-based and 5 km-based predictions is
  internal to the athlete's own data and needs no invented figure; it is open
  question 4.
- **British English, short labels** ("HALF", "MARATHON", "from 5 km"). The
  usual worst-case text-fit check against the bezel arcs applies (as P2's faces).

### 5.3 When to show nothing

| Condition | Shown |
|---|---|
| No 5 km and no mile best in the window | "Run 5 km to see predictions" |
| The newest qualifying best is older than the window | "No recent 5 km" (with the old one's date) |
| The best came from a treadmill run | Not used (an estimated distance) |
| The best came from a run with GPS trouble (a ring restart in the split) | Not used: `RunStats` already restarts its ring on a jump, so such a split never forms |
| Fewer than *n* qualifying runs in the window | Open question 3: one run may be enough for Riegel, since the method needs one effort |

The window should match the VO2max age limit already measured in P3:
`kMaxRunAgeDays` = 42 days, still provisional. Then the app has one idea of
"recent".

---

## 6. Open questions for Jon

1. **The Vickers and Vertosick coefficients.** Can you allow
   `bmcsportsscimedrehabil.biomedcentral.com` (or `pmc.ncbi.nlm.nih.gov`) in
   the session's network settings, or send the PDF? The marathon model, the
   unit of its MSE and Riegel's group exponents should be read from the papers
   before the build phase. The same goes for the Valencia study's VDOT error
   (Frontiers in Physiology 2025, doi 10.3389/fphys.2025.1718298).
2. **The marathon.** Blank, or "3:31+" (a lower bound from Riegel)? Or wait for
   the mileage model, accepting that the app sees only Run's and Trail's runs?
3. **Recency.** Is 42 days right (the same as the VO2max age limit), and is one
   qualifying 5 km in that window enough?
4. **A range.** One time per distance as proposed, or a range from the
   athlete's own mile and 5 km bests?
5. **Treadmill and Trail.** Agreed that treadmill bests never count? Trail
   runs have no `RunStats` bests at all (they are Trail's), so hilly Trail
   efforts stay out for now. Should a later phase change that?
6. **Effort.** Should a 5 km only count if it was hard enough, for example
   its average HR above some fraction of the HR reserve? No published
   threshold was found, so any figure would be your choice, and it would need
   `RunStats` to keep the HR for each split.
7. **VO2max later.** After Gate R, do you want the VO2max-based cross-check
   (§5.1, item 3), and on which scale: the conversion in §4.3, or none?
8. **HYROX.** These are running-race predictions only. A HYROX time cannot
   come from any of these methods, and none was looked for. If you want one,
   it needs its own published source.

---

## 7. Sources

- Riegel PS. Athletic records and human endurance. *American Scientist*
  1981;69(3):285-290.
- Vickers AJ, Vertosick EA. An empirical study of race times in recreational
  endurance runners. *BMC Sports Sci Med Rehabil* 2016;8:26.
  doi:10.1186/s13102-016-0052-y.
- Daniels J, Gilbert J. *Oxygen Power: Performance Tables for Distance
  Runners*. Tempe, AZ: self-published, 1979. Daniels J. *Daniels' Running
  Formula*, 3rd ed. Human Kinetics, 2014.
- Tanda G. Prediction of marathon performance time on the basis of training
  indices. *J Hum Sport Exerc* 2011;6(3):511-520.
- Smyth B, Muniz-Pumares D. Calculation of critical speed from raw training
  data in recreational marathon runners. *Med Sci Sports Exerc*
  2020;52(12):2637-2645.
- Emig T, Peltonen J. Human running performance from real-world big data.
  *Nat Commun* 2020;11:4936. doi:10.1038/s41467-020-18737-6.
- Blythe DAJ, Király FJ. Prediction and quantification of individual athletic
  performance of runners. *PLOS ONE* 2016;11(6):e0157257.
  doi:10.1371/journal.pone.0157257.
- Keogh A, Smyth B, Caulfield B, Lawlor A, Barry D. Prediction equations for
  marathon performance: a systematic review. *Int J Sports Physiol Perform*
  2019;14(9):1159-1169. doi:10.1123/ijspp.2019-0360.
- Performance prediction equation for the Valencia Marathon based on time and
  pacing in the half marathon. *Front Physiol* 2025.
  doi:10.3389/fphys.2025.1718298.
- Molina-Garcia P et al. Validity of estimating the maximal oxygen consumption
  by consumer wearables: a systematic review with meta-analysis and expert
  statement of the INTERLIVE network. *Sports Med* 2022.
  - Exercise-based wearable estimates show little bias at group level but a
    large error for an individual.
- Firstbeat Technologies. Automated fitness level (VO2max) estimation with
  heart rate and speed data (white paper).
  - MAPE about 5%; a max HR 15 bpm out gives 7-9%.
  - Not peer-reviewed.
- The app's own method: ACSM's running equation, Swain and Leutholtz 1997,
  Tanaka et al. 2001 (NOTES "Sources").

The worked numbers in §4 come from a short script, kept out of the
repository. It implements Riegel's and Daniels' formulas exactly as given in
§4.1 and §4.3, plus ACSM's running equation.
