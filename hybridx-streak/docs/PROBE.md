# The Streak Probe: Gate 0, step by step

HybridX Streak only works if one app can read the activity files that other
apps record. Nothing in the SDK proves that the watch allows it (NOTES E.3,
E.9). The **Streak Probe** is a small, throwaway app that tries it on your
watch and writes down what happened. It changes nothing belonging to any
other app.

It takes about ten minutes, spread over two sessions: one before a phone sync
and one after.

---

## What you need

- The watch, its USB cable and a computer.
- **At least one recorded activity** on the watch, from any app (Running,
  Workout, HybridX Race, anything). A 1-minute walk is enough. If the watch has
  never recorded one, the probe can only say "NO FILES".

## Step 1: download the probe

1. On GitHub, open the repository and choose the **Actions** tab.
2. Pick the newest **Watch builds** run with a green tick on the branch.
3. At the bottom of the run page, under **Artifacts**, download **watch-apps**
   and unzip it.
4. Inside is `HXStreakProbe_0.1.0.uapp`. Only use this GitHub-built copy.

## Step 2: install it

1. Connect the watch by USB and wait for its drive to appear. This can take a
   little while.
2. On the watch drive, open `Apps/`, create a folder named exactly
   **`HXStreakProbe`**, and copy the `.uapp` into it.
3. Eject the drive **safely**, then unplug.
4. Power the watch off and on again.

## Step 3: first run (before syncing the phone)

1. Press the top right button and open **Streak Probe**.
2. It works for a few seconds, then shows one screen:
   - a **verdict** at the top (see the table below);
   - how many apps it saw and how many hold activity files;
   - how fast it read the largest file;
   - whether the shared folder worked;
   - the glance area the watch reports.
3. **Take a photo of the screen.**
4. Press **R2** (bottom right) to close it.

| Verdict | Meaning |
|---|---|
| **GO** (lime) | It opened another app's activity file. Automatic counting can work. |
| **NO FILES** | It can look into other apps' folders, but none has an activity yet. Record a short one and run the probe again. |
| **NO READ** | It found activity files but could not open them. |
| **BLOCKED** | The watch keeps each app to its own folder. Automatic counting cannot work as planned (see "If it is not GO"). |

## Step 4: sync the phone, then run it again

1. Let the UNA phone app sync the watch as normal.
2. Open **Streak Probe** again, take a second photo, and close it with R2.

The second run answers a separate question: does the phone app **delete**
activities from the watch once it has synced them? The two runs go on
consecutive lines of `probe-history.txt`, so the file counts can be compared.

## Step 5: send the results back

1. Connect the watch by USB again.
2. Copy these two files off the watch, from `Apps/HXStreakProbe/`:
   - **`probe.txt`**: the full report of the latest run;
   - **`probe-history.txt`**: one line per run.
3. Send both files, plus the two photos, back in this conversation. If copying
   files is awkward, the photos alone are enough to decide Gate 0.

## Step 6: remove the probe (optional)

Delete the `Apps/HXStreakProbe/` folder over USB and power-cycle the watch.
Nothing else needs cleaning up. The probe's only temporary file, in
`Apps/SharedData/`, deletes itself.

---

## What the probe does

The probe checks each of these (details in PLAN §3):

1. **Listing.** It lists its own folder's parent (`..`), plus `/Apps`,
   `2:/Apps` and `/`, to see which routes out of its own folder work.
2. **Activity files.** For every app found, it looks inside
   `Activity/YYYYMM/` for `activity_*.fit`. It counts them, notes the newest,
   and checks for a `.recording` marker (a recording in progress).
3. **Opening a file.** It opens the newest activity file and checks that it
   really is a FIT file (`.FIT` at byte 8). It then times a read of the
   largest one, capped at 2 MB.
4. **Clock and settings.** It records the clock (local time and time zone
   offset) and the system settings.
5. **The glance area.** It asks for the glance area from an ordinary app.
6. **The shared folder.** It writes, reads back and deletes one small file in
   `Apps/SharedData/`, the SDK's own route for sharing between apps.
7. **Renaming.** It writes two temporary files in its own folder and checks
   whether renaming one onto the other is refused, then deletes both.

Every loop is bounded, and every buffer is fixed. It reads no other app's file
beyond the two it opens.

## If it is not GO

Nothing is lost, but the product changes shape. PLAN §3 lists three honest
options:

- ask UNA for read access or an activity-list API;
- ship manual logging, plus automatic counting from HybridX apps that write
  to `SharedData`;
- pause the app.

**No workaround will be attempted.**

## For developers

- **Code:** `Tools/Probe/Software/`
  - the checks: `Libs/Sources/ProbeRunner.cpp`, pure C++ over
    `IFileSystem`;
  - the service: `Libs/Sources/Service.cpp`;
  - the screen: `Apps/Probe-GUI/gui/src/ProbeGui.cpp`.
- **Host tests:** `Tests/Host/ProbeRunnerTest.cpp`, against the
  `TreeFileSystem` fake. They cover a watch where apps can see each other and
  one where the firmware forbids it, plus the cases in between.
- **Simulator:** `Tools/Probe/Software/Apps/Probe-GUI/simulator`. The mock
  file system is rooted at `../../../../../Output/` from the working
  directory, so run it from a scratch tree `<x>/a/b/c/d/e` with fake apps
  in `<x>/Running/Activity/…`.
  - The result is `screens/probe-simulator.png`.
  - The simulator's `..` is plain host-directory traversal, so its GO says
    nothing about the watch.
  - Its rename replaces the destination, which the probe reports as
    "rename replaces".
