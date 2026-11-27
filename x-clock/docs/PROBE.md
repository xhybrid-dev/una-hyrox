# The Clock Probe: Gate F0, step by step

The Clock Probe is a watch face that does nothing but find things out. It answers, on your own watch,
the questions that decide whether the X Clock face can be built the way we want:

- Does the watch accept and show a face built the way our apps are (LVGL)?
- Can a face read the files the HybridX apps leave in the shared folder, and what exactly is in them?
- Does a wrist raise reach a face, and is the face being redrawn while the screen is dark?
- Does it stop your other apps opening?

It also keeps a log you copy back. It reads, never changes, the family's files. It does not read
X Weigh-in's weight log, and it does not record your height or weight.

You wear it as your watch face for a few days. It runs only while it is your face.

---

## What you need

- The watch, its USB cable and a computer.
- X Run, X Training, Race and the other HybridX apps installed, and a few sessions saved this week,
  so there is something to read. (X Training with a plan set up, if you use it.)

## Step 1: download and install

1. On GitHub, open the repository and choose the **Actions** tab.
2. Pick the newest **Watch builds** run with a green tick on the branch `claude/brave-cannon-bjrzzn`.
3. At the bottom of the run page, under **Artifacts**, download **watch-apps** and unzip it. Inside is
   `ClockProbe_0.1.0.uapp`. Only use this GitHub-built copy.
4. Connect the watch by USB and wait for its drive to appear.
5. On the watch drive, open `Apps/`, create a folder named exactly **`ClockProbe`**, and copy the
   `.uapp` into it.
6. Eject the drive **safely** and unplug.
7. **Power the watch off and on again.** The watch finds faces when it starts.

## Step 2: choose it as your face

On the watch: **Settings, then Watch Face, then Clock Probe**.

**If Clock Probe is not in the list, or it is and the screen stays blank or the watch restarts, stop
and tell me exactly what you saw.** That is the most important answer there is: it means a face
cannot be built with LVGL, and we would build it another way.

## Step 3: the first look (a minute)

The screen is one face. Take a photo of it.

- **The rim** is a ring of seven segments, Monday at twelve o'clock, going clockwise. Each is graded by
  that day's training load from your HybridX apps: dark grey for nothing, amber for some, yellow for a
  lot. A white mark sits over today. This is a first draft: the grading is a guess, and part of what the
  probe is for is finding out what "a lot" is.
- **The time** is at the top.
- **Seven rows**, each with a light (green is good, amber has a catch, red failed, grey is not known yet):

| Row | Says | Green means |
|---|---|---|
| 1 | `Files 7 of 12` | It found the family's files, and read them |
| 2 | `List ok 2 + 10` | It could list the shared folder and the HybridX folder in it |
| 3 | `Log write ok` | It could write its own log |
| 4 | `Wrist 3, 12s ago` | Wrist raises heard today, and the last one |
| 5 | `Ticks 10.0/s, R2` | How fast the face is being redrawn, and how often it has been resumed (R) |
| 6 | `4 x 4 min 4:18/km` | Today's session from X Training (or "Plan: no file" if it has not run) |
| 7 | `VO2 52.3 (+0.4)` | X Run's VO2max and its change, with the trend as bars on the right |

The probe is cut off from the rest of the watch's files, as every app is, so **row 1 can only say whether
a file exists**, not which apps are installed.

## Step 4: the tests (do them over the first day or two)

Each says what it answers. Write down what you see.

**A. Wrist raises (10 times).** The probe listens for a wrist raise on some days and not on others, to
measure what listening costs the battery:

| Date | Listens |
|---|---|
| Fri 27 Nov | yes |
| Sat 28 Nov | no |
| Sun 29 Nov | yes |
| Mon 30 Nov | no |
| Tue 1 Dec | yes |
| Wed 2 Dec | no |
| Thu 3 Dec | yes |

On a "yes" day, rest your arm so the screen goes dark, then raise your wrist to look. Do it ten times,
over an hour or more, not all together. Each time, notice:
- did the screen light **by itself**, or only when you pressed a button?
- was the time **already right** when it lit, or did it update a moment later?
- does the `Wrist` row's count go up?

*Answers: does a face hear a raise, and does it wake with it.*

To make a day "yes" or "no" yourself: put a text file called `mode.txt` in `Apps/ClockProbe/` containing
just `on` or `off` (or `auto`, the default). It is read at start and at midnight.

**B. Does it stop your other apps opening?** With Clock Probe as your face, open each of these in turn,
get to their main menu, and start a session in each and discard it:
**X Training, Race, X Workout, X Trail (with a route loaded), X Run, X Streak.**
Note any that will not open, open slowly, or restart the watch. *This is the memory question: a face
stays loaded while another app runs, and the watch will not start an app that does not fit.*

**C. Does the face catch up after an app?** Do a real, short session in X Run (or any HybridX app) and
**save** it. Leave the app. *Without restarting anything*, look at the face: does the ring show it, and does
the `Files` row still read green? If you have X Training, open it once, leave, and look at the `Plan` row.
*This is the refresh-on-resume question: the face has no timer for it.*

**D. Switching faces.** Choose another face, wait a minute, choose Clock Probe again. The probe starts
again when it is chosen, and stops logging while it is not.

## Step 5: wear it

Wear it as your face for **three days at least, across a "yes" and a "no" day**. Charge as you normally
do; the log marks it. Take a photo of the screen at a few different times of day.

## Step 6: what to send back

1. Connect the watch by USB. (This stops the probe cleanly; it starts again when you unplug and choose it.)
2. Copy the whole folder **`Apps/ClockProbe/Logs`** to the computer and send it to me. It holds:
   - `events.csv`: starts, scans, wrist raises, the face waking and sleeping, the watch's settings;
   - `scan_*.txt`: **the shared folder as the watch sees it, and the real contents of each family
     file.** This is your training data: what you saved, your plan and your VO2max. It is what I need to
     see, and sending it is your choice;
   - `min_*.csv`: a row a minute, with the battery gauge and the face's ticking;
   - `days.csv`: your watch's own steps, active minutes and floors once an hour;
   - `state.txt`.
3. Your notes from step 4, and the photos.

I will turn the logs into answers with `x-clock/docs/experiments/probe_report.py`.

## If something looks wrong

- **`Log write FAILED`** (row 3, red): the face cannot write its own folder. Send the photo; the screen
  carries enough to answer without the log.
- **`Files 0 of 12`** (row 1, red) with **`List: no folder`**: a face cannot see the shared folder, or
  no HybridX app has saved anything yet. Save a session in X Run, leave the app, and look again.
- **`Plan: no file`**: X Training writes that file when it opens, so open it once. (`Plan: none set`
  means it has no plan.)
- The probe stops when you choose a different face. That is how faces work, not a fault.
