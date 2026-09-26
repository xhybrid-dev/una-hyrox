# The Trail Probe: Gate T0, step by step

HybridX Trail depends on things only a real watch can show: that a GPX
copied over USB reaches the app, how long a big one takes to read, how much
memory there is to hold a route, how long GPS takes to find you, and whether
the compass gives a usable bearing. The **Trail Probe** is a small, throwaway
app that checks all of them and writes down what happened. It changes nothing
belonging to any other app.

It takes about twenty minutes, most of it outdoors waiting for GPS.

---

## What you need

- The watch, its USB cable and a computer.
- A GPX route. Any will do: export one from OS Maps, Komoot, Strava or
  similar. **A long one is best** (a marathon or an ultra), because read time
  and memory are what we're measuring. Ideally, also pick a route that starts
  near where you'll stand for step 5, so the "off route" figure means
  something.
- To be outside, with a view of the sky, for steps 4 and 5.

## Step 1: download the probe

1. On GitHub, open the repository and choose the **Actions** tab.
2. Pick the newest **Watch builds** run with a green tick on the branch.
3. At the bottom of the run page, under **Artifacts**, download **watch-apps**
   and unzip it.
4. Inside is `HXTrailProbe_0.1.0.uapp`. Only use this GitHub-built copy.

## Step 2: install it and run it once

1. Connect the watch by USB and wait for its drive to appear. This can take a
   little while: running apps save their data first.
2. On the watch drive, open `Apps/`, create a folder named exactly
   **`HXTrailProbe`**, and copy the `.uapp` into it.
3. Eject the drive **safely**, then unplug.
4. Power the watch off and on again.
5. Press the top right button, open **Trail Probe**, and check it says
   **NO ROUTE**. That run created the `Routes/` folder for you.
6. Press **R2** (bottom right) to close it.

## Step 3: copy your GPX over USB

1. Connect the watch by USB again and wait for its drive.
2. Open `Apps/HXTrailProbe/Routes/` and copy your GPX file into it.
3. Eject the drive **safely**, then unplug.
4. **Don't power-cycle the watch this time.** Whether the app can see a newly
   copied file without one is one of the things we're finding out.

## Step 4: go outside, open the probe and read the route

The GPS clock starts when the probe's screen opens, so do this outside, with
a clear view of the sky.

1. Open **Trail Probe** again.
2. The top half of the screen is the route:

| Verdict | Meaning |
|---|---|
| **GO** (lime) | The GPX was found and read into a route. Below it: the route's name, its length, the number of points kept and how long reading took. |
| **NO ROUTE** | `Routes/` has no `.gpx` in it. If you did copy one, **power-cycle the watch and open the probe again**, and tell me which of the two it took. |
| **UNREADABLE** | A `.gpx` is there but no route came out of it. Send me the file (or tell me where it was exported from). |
| **FOLDER FAILED** (red) | The app couldn't create or open `Routes/`: a bug in the probe, not your file. Send the results over. |

3. **Take a photo of the screen.**

## Step 5: stay outside for three minutes

Leave the probe open. The bottom half of the screen updates every second for
three minutes:

- **GPS:** "searching" with a count of seconds, then "fix in N s" and the
  GPS's own accuracy estimate.
- **Compass:** a bearing in degrees if the compass is calibrated, or "not
  calibrated". While it shows a bearing, **point your forearm at something
  whose direction you know** (a road, the sun at noon, a phone compass app
  held flat) and note roughly how close it is. Try with your arm held level,
  then raised as if reading the watch.
- **Off route:** once there's a fix, how far you are from the route.
- **Largest block:** the memory measurement from the start.

After three minutes the footer says **Run N saved**. **Take a second photo**,
then press **R2**. (Pressing R2 earlier is fine; the probe saves what it has.)

## Step 6: send the results back

1. Connect the watch by USB.
2. Copy these two files off the watch, from `Apps/HXTrailProbe/`:
   - **`probe.txt`**: the full report of the latest run;
   - **`probe-history.txt`**: one line per run.
3. Send both files and the two photos back in this conversation, and tell me:
   - whether step 4 needed a power cycle;
   - roughly how the compass compared with the known direction, level and
     raised;
   - what device or site the GPX came from.

The report gives your position to three decimal places (about 100 m), so it
says roughly where you tested, not exactly.

## Step 7: remove the probe (optional)

Delete the `Apps/HXTrailProbe/` folder over USB and power-cycle the watch.
Nothing else needs cleaning up.

---

## What the probe does

- **On opening** (the service, before the screen appears):
  1. creates `Routes/` if it isn't there, lists it, and skips anything that
     isn't a `.gpx`, including the hidden `._name.gpx` files a Mac leaves on
     USB drives;
  2. reads the newest `.gpx` in 512-byte chunks through the **real app's GPX
     reader** (`Software/Libs/Core`), thinning it into at most 2,000 points
     evenly spread along the route, and times it;
  3. finds the largest single block of memory it can allocate (up to
     512 KB), freeing it at once;
  4. saves `probe.txt`.
- **While the screen is open** (up to three minutes): connects GPS, the
  magnetometer and the accelerometer, and shows what they give, with a line in
  the report every 15 seconds. At the end, or when closed, it saves
  `probe.txt` again and adds a line to `probe-history.txt`.

## If it is not GO

- **NO ROUTE even after a power cycle:** USB delivery into an app's folder
  doesn't work as the SDK describes. That would be a question for UNA, and the
  phone route (`UNA_GPX_REQUEST.md`) becomes the only way in.
- **UNREADABLE:** a GPX variant the reader doesn't handle. That's fixable in
  the core with the file as a new test.

**No workaround will be attempted** without talking it through first, the same
rule as Streak's and Intervals' probes.

## For developers

- **Watch app:** `Tools/Probe/Software/`
  - the route check: `Libs/Sources/ProbeRunner.cpp`, pure C++ over `IFileSystem`;
  - the service (memory, sensors, report): `Libs/Sources/Service.cpp`;
  - the screen: `Apps/Probe-GUI/gui/src/ProbeGui.cpp`.
- **The app's core it uses:** `Software/Libs/Core/` (`GpxReader`,
  `RouteBuilder`, `RouteMath`, `GeoPoint`).
- **Host tests:** `Tests/Host/`, 37 cases over the core and the probe's route
  check, against an in-memory `FlatFileSystem` fake.
- **Test routes:** `Tools/TestRoutes/make_test_gpx.py <folder>` writes a
  1.2 MB 10 km loop track and a short OS Maps-style route.
- **Simulator:** `Tools/Probe/Software/Apps/Probe-GUI/simulator`. Its file
  system is the folder `Tools/Probe/Software/Output/`, so put GPX files in
  `Output/Routes/` and run it from `build/bin`. It proves the reading side
  only: the simulator has no magnetometer, its GPS circles a stadium somewhere
  else, and its memory is a PC's.
