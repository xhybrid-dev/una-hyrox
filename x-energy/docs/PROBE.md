# The Energy Probe: Gate SL0, step by step

The Energy Probe is the first X Energy app. It records everything a sleep and energy app
could use, once a minute, all night and all day:
- movement;
- heart rate and beats;
- the watch's temperature;
- whether it is on the wrist;
- the watch's own daily figures;
- the battery.

It also measures what that costs in battery. Some nights it records everything, and some
nights it records nothing but the battery. Comparing the two shows the cost of the full
app. It works nothing out on the watch: the data comes back to us, and the sleep and
energy maths is built on it (`NOTES.md`, "Gate SL0").

It runs by itself, from the moment the watch starts, even when you never open it.

---

## What you need

- The watch, its USB cable and a computer.
- Your Garmin, worn on the other wrist each night, for comparison.
- A note on your phone each morning (step 5).

## Step 1: download and install

1. On GitHub, open the repository and choose the **Actions** tab.
2. Pick the newest **Watch builds** run with a green tick on the branch
   `claude/trusting-knuth-wwpbda`.
3. At the bottom of the run page, under **Artifacts**, download **watch-apps** and unzip
   it. Inside is `EnergyProbe_0.1.0.uapp`. Only use this GitHub-built copy.
4. Connect the watch by USB and wait for its drive to appear.
5. On the watch drive, open `Apps/`, create a folder named exactly **`EnergyProbe`**, and
   copy the `.uapp` into it.
6. Eject the drive **safely** and unplug.
7. **Power the watch off and on again.** The probe starts itself when the watch starts.

## Step 2: check it is logging

Open **Energy Probe** from the app list. It opens on **Now**. **L1** and **L2** turn the
pages, **R1** is the page's action (in amber), and **R2** closes the screen. Closing the
screen does **not** stop the logging.

On **Now**, within a minute or two you should see:
- "Now: Full, day";
- the battery %;
- your heart rate;
- the bottom line in green: "Logging, start 1".

If the bottom line says:
- **"Clock not set"**: sync the watch with the UNA app, then look again.
- **"Sensors failed: 0x…"**: write the number down and carry on. Some sensors may simply
  not exist on the watch, and that is one of the answers we want. The bits are:
  - 0x02 movement;
  - 0x04 heart rate;
  - 0x10 beats;
  - 0x20 temperature;
  - 0x40 the touch and motion events;
  - 0x80 the watch's daily figures.
- **"Write errors"**: tell me. The watch may be short of space.

## Step 3: the sensor check (once, today)

This asks the watch for every sensor the SDK lists and records what each one says. It
answers the biggest open question: whether the watch gives beat-to-beat timing, which a
stress and HRV score needs.

1. Put the watch on and sit still.
2. Go to the last page, **Sensor check**, and press **R1**. It listens for 2 minutes.
3. When it finishes, it shows one line per sensor, six at a time. **R1** shows the next
   six. A photo of each set helps, but the full results are saved to a file anyway.

## Step 4: the nights (nothing to do)

The probe changes what it records at **22:00** and **08:00** by itself. The nights follow
a rotation:

| Night | Records |
|---|---|
| Tonight, Tue 24 Nov | **Full**: every sensor |
| Wed 25 | **Off**: battery only |
| Thu 26 | Full |
| Fri 27 | Off |
| Sat 28 | **Motion**: movement only |
| Sun 29 | **Heart**: heart rate only |
| Mon 30 | **Heart 1 in 5**: heart rate one minute in five |
| Tue 1 Dec | Off |

After that it repeats from Full.

Days always record **Full**, so naps and rest show up. If the battery runs down too fast,
set **Daytime** to Off.

- **Don't charge it overnight.** If you must, the night is marked "charged" and left out
  of the comparison.
- If you sleep later than usual, change **Night window** (R1 cycles 21:00–07:00,
  22:00–08:00, 23:00–09:00 and 00:00–10:00).
- To swap one night, use **Tonight**: R1 cycles Off, Full, Motion, Heart, Heart 1 in 5,
  and back to the rotation. It applies to tonight only.
- **Each morning, look at Nights.** It shows each finished night as: date, mode, battery
  drain in % an hour, and current in mA if the watch reports it. "C" means it was charged
  that night; "*" means the night started late or the watch restarted.

## Step 5: during the week

Each of these answers a question. Note the times.

1. **A morning note**:
   - when you went to bed and turned the light off;
   - roughly when you fell asleep;
   - any time you were awake in the night;
   - when you woke and got up;
   - any nap;
   - caffeine or alcohol late in the day.
2. **One evening, take the UNA off for an hour** and leave it on the bedside table. A
   still watch looks like deep sleep, so the probe has to tell "off the wrist" apart.
3. **A nap of 20 minutes or more** on any day, if you can.
4. **Once, with the probe running, open X Training and HybridX Race** and start a
   session, then discard it. Do they open normally? The probe takes some of the watch's
   memory, and this checks that the other apps still fit.
5. **A normal run** with X Run or X Training one day. Does the heart rate look normal,
   and does the probe keep logging (its row count on Now keeps rising)?

## Step 6: what to send back

After a few nights (all eight is best):

1. Connect the watch by USB. This pauses the probe; it starts again when you unplug.
2. Copy the whole folder **`Apps/EnergyProbe/Logs`** to the computer and send it to me.
   It holds:
   - `min_*.csv`, a row a minute, one file a day;
   - `nights.csv`, each night's battery;
   - `events.csv`, starts, charging and mode changes;
   - `check_*.csv`, the sensor check;
   - `beats_*.csv`, raw beats, only if the watch gave any;
   - `state.txt`.
3. From your Garmin, for each day: sleep (times and stages), resting heart rate, HRV
   status and Body Battery. Screenshots are fine.
4. Your morning notes.

I run `docs/experiments/probe_report.py` on the folder. It gives each mode's battery cost
against the Off nights, what each sensor delivered, the check's answers, and a first look
at each night's sleep window and lowest heart rate.

## Removing it

Over USB, delete the folder `Apps/EnergyProbe`, then restart the watch.
