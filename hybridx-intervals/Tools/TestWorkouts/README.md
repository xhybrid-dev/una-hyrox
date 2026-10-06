# Test workouts

Example workout files in the format of `docs/WORKOUT_FILE.md`, for the
simulator and for a first try on the watch.

| File | What it is |
|---|---|
| `6 x 400 m.json` | 10 min warm-up, 6 x (400 m at 3:50-4:10/km, 90 s rest), 10 min cool-down |
| `5 x 1 km.json`  | warm-up until you press, 5 x (1 km at 3:50-4:05/km, 2 min rest), 10 min cool-down |
| `Tempo 20.json`  | 15 min warm-up in HR zones 1-2, 20 min in zone 4, cool-down until you press |
| `Zone 2 spin.json` | a bike workout: listed on the watch, but not runnable yet (running only) |
| `broken example.json` | a cut-short file: the watch lists it as unreadable |
| `sim-short.json` | 20 s warm-up, 3 x (200 m, 20 s rest), cool-down: for the simulator's fast runner |

**On the watch (USB):** connect it, wait for its drive, and copy the files into
`Apps/HybridXIntervals/Workouts/` (make the folder if the app hasn't yet).
Eject safely, unplug, and open the app.

The file names are what the watch shows only when a file can't be read;
otherwise it shows the `name` inside.
