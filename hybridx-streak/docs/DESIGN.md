# HybridX Streak — Design

**The Summit climb.** Every week you hit your target is one step up a
mountain. Reach the top and you have climbed it; the next, taller mountain is
waiting. A missed week can break the streak, but it never knocks you down the
mountain: the climb counts every week you have ever achieved.

Jon's choices, 24 September 2026:

| | Choice |
|---|---|
| Look | **Summit climb** |
| Colours | **Teal and lime** |
| Voice | **Encouraging coach** |
| Rules | Every PLAN §12 recommendation (S1-S15) |

Screens as they stand are in `screens/`, captured from the simulator and cut to
the watch's round face. `screens/streak-walkthrough.mp4` is a captioned
walkthrough of how it works, and `screens/streak-demo.mp4` shows the
animations without captions. To capture them again:
`UNA_SDK=… RECORD=1 docs/experiments/capture_screens.sh` and
`docs/experiments/walkthrough.sh`.

---

## 1. The canvas

- The display is a **240 × 240 round** disc. The simulator shows a square;
  its corners do not exist on the watch.
- Keep content **inside radius 105** of the centre (120, 120). The white and
  amber arcs at radius 113 are the SDK's button hints (`SDK::LVGL::Buttons`).
- The display has **64 colours**, 2 bits per channel. There are no gradients or
  alpha blends, so everything is drawn with flat colours and hard edges.
- The GUI draws at 10 fps, so animations are short and have few frames.
- There are no `LV_SYMBOL` glyphs. Icons are drawn from triangles, lines and
  circles, or they are A8 images tinted at run time.

## 2. Palette (`gui/theme/Theme.hpp`, `Palette::`)

All values are `SDK::GUI::Color` names.

| Role | Colour | Where |
|---|---|---|
| Sky | BLACK | every background |
| Stars | GRAY_DARK, a few GRAY | the night sky over the mountain |
| Far range | GRAY_DARK | distant peaks behind the mountain (STEEL_DARK read as purple) |
| Rock, sunlit face | TEAL | left face |
| Rock, shaded face | TEAL_DARK | right face, shoulders |
| Snow | WHITE lit, GRAY shaded | the cap |
| Trail still to climb | GRAY | 1 px line, and step dots on short climbs |
| **Win** | **LIME** | climbed trail, the streak number, "Week complete!", the flag, done pips |
| Text | WHITE | headlines |
| Soft text | GRAY | the mountain line, the coach line |
| At risk | YELLOW_DARK (amber) | the coach line when the week is in danger |
| Shields | CYAN | the shield glyph, bonus pips, shield counts |
| Sunrise | YELLOW_DARK sun, LEMON rays | Fresh start |

Lime is kept for **you did it**. If lime is on the screen, something was
earned.

## 3. Type

Poppins, from Race's committed font files, cut down to these faces:

| Face | Size | Use |
|---|---|---|
| SemiBold | 30 | the streak number |
| SemiBold | 25 | "Summit!"-scale titles, "Your first week", "Fresh start." |
| SemiBold | 20 | fall-back headline when the words are long |
| Medium | 18 | "week streak", "this week", "Week complete!" |
| Regular | 16 | body text on the shield screen |
| Regular | 14 | the mountain line, the coach line, small print |
| SemiBold | 60 | digits only; kept for later |
| Italic | 18 | kept for later |

The headline pairs a big lime number with its words on one shared baseline
("**7** week streak"), centred together. No line may be wider than 196 px, and
coach lines are at most 21 characters (`kMaxCoachChars`), because the disc
narrows fast below the centre.

## 4. Home

From the top:

1. **The scene** (`SummitScene`): stars, the far range, the mountain with a
   distinct silhouette per climb, snow, the switchback trail and a flag on the
   summit.
2. **The climber**: a lime dot in a white ring, with a slow pulsing halo, at
   the step you have reached. The trail behind it is lime; the trail ahead is
   grey. Short climbs (8 steps or fewer) also show a dot per step.
3. **The mountain line**, y 119: "Snowdon · 5 weeks to go". A repeat ascent
   reads "Everest again · …".
4. **The headline**, baseline y 163: "**7** week streak", "Your first week"
   (trial) or "Start a new streak".
5. **This week**, y 172: one pip per session against the target, lime when
   done and grey rings when not, plus up to 3 cyan bonus pips, then
   "this week".
6. **The coach line**, y 197. It is grey normally, amber when at risk and lime
   when the week is banked.

## 5. The ladder

Progress up the mountains uses **cumulative weeks achieved** (PLAN S10). The
streak is shown separately, and a reset never takes you down the mountain.

| Climb | Summit at (weeks) | Steps | Silhouette | Snow |
|---|---|---|---|---|
| Arthur's Seat | 4 | 4 | Crag | none |
| Snowdon | 12 | 8 | Pyramid | 18% |
| Ben Nevis | 26 | 14 | Massif | 24% |
| Mont Blanc | 52 | 26 | Dome | 44% |
| Everest | 104 | 52 | Spire | 32% |

After Everest, Everest repeats every 52 weeks ("Everest again"). The table is
`Core/Summits.hpp`; the shapes are `gui/summit/SummitGeometry.cpp`.

**Session badges** (total sessions, from S3 onwards): Trailhead 10, Ridge
Walker 50, Centurion 100, Mountaineer 250.

## 6. Moments

Every moment has a look, a line and a buzz. Only the service may drive the
motor, so the GUI sends `Celebrate{moment}` and the service plays it.

| Moment | On screen | Haptic |
|---|---|---|
| **Session found** | A toast replaces the coach line: "+1 Run · 32 min". The new pip pops. | SHARP_TICK |
| **Step up** (target reached) | "Week complete!" in lime. The climber glides one step up the trail (1.1 s, ease in-out) and the trail behind it turns lime. Then an 8-ray lime burst. | SHORT_DOUBLE_CLICK_STRONG, backlight 5 s |
| **Summit** | Full screen. "Summit!", a big peak with a waving lime flag, 16 pieces of falling confetti, "Ben Nevis · 26 weeks", "Next: Mont Blanc". | PULSING_STRONG × 2, 250 ms apart, backlight 5 s |
| **Shield offer** (missed week, shields left) | A cyan shield around a small mountain. "Life happens." R1 green tick to spend, R2 white cross to decline. | none |
| **Shield spent** | "Streak saved." in lime | SOFT_BUMP |
| **Fresh start** (streak reset) | A sun rises over the hills (1.6 s). "Fresh start." and "Your climb is safe: 6 of 14 on Ben Nevis. A new streak starts today." | none |

Buttons follow UNA's convention: R1 means yes or go (a tick beside it), R2
means no or back (a cross beside it). L1 and L2 move between things.

## 6a. The menus (phase S3)

The screens behind Home use the SDK's `WheelMenu`, the same widget as the UNA
apps' menus. The selected line is in SemiBold 25 (the activity apps use 30,
which is too wide for "Log a session"), the lines around it in Medium 18,
and hints in Italic 18. An Italic 18 title sits over a short rule.

| Screen | What it holds | Keys |
|---|---|---|
| Home | as §4; plays the service's moments in order | L1/L2/R1 menu · R2 exit |
| Menu | This week ("3 of 3 counted") · Log a session · Trophy case · Settings | L1/L2 move · R1 open · R2 home |
| This week | each session: kind, then minutes and app ("35 min · Running"), or why it doesn't count ("6 min · under 10", "Goal: Runs only", "One a day · Tue", "Excluded · Cycling"). Counting hints are lime, others grey | R1: leave out / count again / undo |
| Leave out? | "Leave this out?" / "Count it again?" / "Undo this log?", with a tick and a cross | R1 yes · R2 no |
| Log a session | Run, Strength, Ride, Walk, Row, Hybrid, Workout, Other, then Today / Yesterday (Yesterday only while it is this week) | R1 next · R2 back |
| Trophy case | the five summits ("Climbed · 12 weeks" or "7 of 12 weeks"), the four badges, best week, longest streak, all sessions | L1/L2 · R2 back |
| Settings | Weekly target ("3 now · 4 next week" while a change waits), Week starts, What counts, Shortest, One per day (toggle) | R1 choose · R2 back |
| Set the time | shown when the clock is unset | R2 exit |

App folders are named as the athlete knows them. `HybridXRace` is "HybridX",
because "HybridX Race" clips on the wheel's lower line.

## 7. Voice: the encouraging coach

- **Warm, short, on your side.** "Week banked. Rest up." not "Target
  achieved."
- **Say what to do next, with numbers.** "1 more · 3 days left". "2 more in
  2 days. Go!"
- **Never scold.** A missed week is "Life happens.", then a choice. A reset
  is "Fresh start.", and it tells you what you kept.
- **Celebrate plainly.** "Week complete!", "Summit!". Only these two get an
  exclamation mark, plus the at-risk "Go!".
- **British English**, sentence case, a middle dot (·) between facts, numbers
  as digits.
- **No trademarks** in the UI: the class of session is "Hybrid", not HYROX.
- **Fit the disc.** A coach line is at most 21 characters. If it does not fit,
  rewrite it; never shrink the font.

## 8. The glance

It reads the app's public copy and brings the week up to now. It never saves.
What it shows depends on the area the watch gives it (`screens/glance-preview.png`
is a mock-up drawn from the real layout code):

| Layout | When | What |
|---|---|---|
| Full | 8+ controls, 170+ px wide, 59+ px tall | a line-drawn mountain with a green flag; "7 week streak" (SemiBold 20, green); "2 of 3 this week" (Medium 18, white; green when met); a coach line (Medium 10): "Snowdon: 5 weeks to go", "2 more in 2 days" (amber), "Week banked. Rest up." (green), "Open to use a shield" (amber) |
| Compact | fewer controls, or a narrow area | the first two lines, centred |
| Tiny | under 46 px tall | "7 wk streak · 2/3" |

Special states: "First week"; "New streak"; before the app is first opened,
"HybridX Streak / Open it to start"; with the clock unset, "Set the time / in
the UNA app". The glance has only 16 colours and no lime, so green stands in.

## 9. Budgets

- The LVGL pool (40 KB) peaks at **62%** across every real screen, against
  Race's 91% (47% for the S0 demo). A screen is deleted before the next is
  built, so two menus are never in the pool together; together they reached
  89%.
- No screen allocates in the 1 Hz path. Animations use `lv_anim` on existing
  objects, and the scene redraws only when its data changes.
