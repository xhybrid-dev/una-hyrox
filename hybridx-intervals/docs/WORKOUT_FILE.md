# The workout file (version 1)

One workout per file, JSON, at `Apps/HybridXIntervals/Workouts/<name>.json` on
the watch. The phone sender writes it over BLE (Trail's Route Sender proved the
transport: `hybridx-trail/docs/NOTES.md`, "Route Sender run 2"); over USB you
can drop the file in by hand. The watch app reads it with
`Intervals::parseWorkout` (`Software/Libs/Core/Sources/WorkoutParser.cpp`).

## Example

```json
{
  "v": 1,
  "name": "6 x 400 m",
  "sport": "run",
  "steps": [
    { "type": "time", "value": 600, "intensity": "warmup" },
    { "type": "dist", "value": 400, "intensity": "active",
      "target": { "kind": "pace", "low": 230, "high": 250 } },
    { "type": "time", "value": 90, "intensity": "rest" },
    { "type": "repeat", "from": 1, "count": 6 },
    { "type": "time", "value": 600, "intensity": "cooldown" }
  ]
}
```

This is `Tests/Host/fixtures/6x400_pace.json`, which the host tests parse and
run through the engine, so the example cannot drift from the code.

## Top level

| Key | Type | Notes |
|---|---|---|
| `v` | integer | **Required.** Must be `1`. Write it first: a reader sees a newer version before it trips on anything else |
| `name` | string | **Required.** 1 to 31 bytes (`Workout::kNameChars` is 32, with the end marker). ASCII or UTF-8. Only `\"`, `\\` and `\/` are accepted as escapes |
| `sport` | `"run"` or `"bike"` | **Required** |
| `steps` | array | **Required.** 1 to 20 steps (`Workout::kMaxSteps`) |

## A step

| Key | Applies to | Notes |
|---|---|---|
| `type` | all | **Required.** `time`, `dist`, `open` or `repeat` |
| `value` | `time`, `dist` | **Required.** Whole seconds (1 to 86,400) or whole metres (1 to 100,000). The watch converts to ms and cm |
| `intensity` | not `repeat` | `active` (the default if absent), `rest`, `warmup`, `cooldown` |
| `target` | not `repeat` | Absent means open (no target) |
| `from` | `repeat` | **Required.** Index (from 0) of the first step to repeat. Must be before this step |
| `count` | `repeat` | **Required.** Number of times to run the block, 1 to 999 |

- An `open` step has no end of its own; it ends when the athlete presses to
  advance (assumed, not yet confirmed on the platform; NOTES "P2").
- A `repeat` step runs the steps from `from` up to just before itself, `count`
  times in all, then carries on. One repeat block at a time: a block that
  contains another `repeat` is refused.

## A target

| Key | Notes |
|---|---|
| `kind` | **Required.** `open`, `pace`, `hrzone` or `hrbpm`. There is no power or cadence target: the watch cannot sense either |
| `low`, `high` | **Required** unless `open`. Whole numbers with `low <= high`, in the kind's units |

| Kind | Units | Range |
|---|---|---|
| `pace` | seconds per km. `low` is the **faster** bound, `high` the slower | 1 to 3,600 |
| `hrzone` | zone number from the watch's own zones | 1 to 7 |
| `hrbpm` | beats per minute | 30 to 250 |

So "4:00 to 4:10 per km" is `"low": 240, "high": 250`.

## Rules the reader enforces

- Numbers are whole, non-negative and small. `-5`, `1.5`, `1e2`, `"60"` and
  `007` are all errors, never rounded or guessed.
- Unknown **keys** are ignored (so a newer sender can add things). Unknown
  **names** for `type`, `intensity`, `kind` or `sport` are errors, not
  defaults.
- The file may start with a UTF-8 byte order mark, and keys may come in any
  order.
- The file is at most 4,096 bytes (`kMaxWorkoutFileBytes`). Bigger is refused.
- The whole file is checked before any of it is used; a bad file is never
  half-loaded.

## Errors (`ParseError`)

`TooLarge`, `Syntax`, `BadVersion`, `MissingField`, `BadValue`, `TooManySteps`,
`NameTooLong`, and `Invalid` (it parsed, but the repeat structure is wrong: the
`ValidationError` says how). The result also carries the byte offset where the
reader stopped, for the log.

## File names

The same rules as Trail's routes, so the phone sender can share its code:
letters, digits, spaces and `- _ ( )`, no leading dot, `.json` on the end, at
most 47 bytes.

## What is not in it

FIT does not carry these targets (the SDK defines only "open"), so a recorded
activity shows every step as open, and the real targets live only in this file
(NOTES, P1.1).
