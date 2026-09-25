# The Intervals Probe: Gate P0, step by step

HybridX Intervals only makes sense as a real phone app if a computer can write
a file onto the watch over Bluetooth, and a watch app can read it back.
Nothing in the SDK proves the transport works end to end (`NOTES.md`). The
**Intervals Probe** is a small, throwaway app plus a small Python script that
try it and write down what happened. It changes nothing belonging to any other
app.

It takes about fifteen minutes.

---

## What you need

- The watch and a computer with Bluetooth (macOS, Windows or Linux).
- Python 3.9 or newer on that computer.
- The watch **paired with that computer** via the OS's own Bluetooth settings
  first. This script does not attempt to drive pairing itself — how it's
  triggered is one of the two things this probe exists to find out, so please
  note what you saw for `NOTES.md`.

## Step 1: download the probe

1. On GitHub, open the repository and choose the **Actions** tab.
2. Pick the newest **Watch builds** run with a green tick on the branch.
3. At the bottom of the run page, under **Artifacts**, download **watch-apps**
   and unzip it.
4. Inside is `HXIntervalsProbe_0.1.0.uapp`. Only use this GitHub-built copy.

## Step 2: install it

1. Connect the watch by USB and wait for its drive to appear.
2. On the watch drive, open `Apps/`, create a folder named exactly
   **`HXIntervalsProbe`**, and copy the `.uapp` into it.
3. Eject the drive **safely**, then unplug.
4. Power the watch off and on again.

## Step 3: pair the watch with your computer

Use your computer's own Bluetooth settings (System Settings on macOS, Settings
→ Bluetooth on Windows, your desktop's Bluetooth panel on Linux) to find and
pair with the watch, the same way you'd pair headphones. **Please note down**
whatever you see during this step — a PIN prompt, a confirm-on-watch prompt,
anything — for `NOTES.md`'s open findings.

## Step 4: run the PC-side script

In a terminal, from this folder:

```
cd Tools/Probe/pc
pip install -r requirements.txt
python3 send_plan.py
```

If it can't find the watch by scanning, try naming it or connecting directly:

```
python3 send_plan.py --name "your watch's Bluetooth name"
python3 send_plan.py --address AA:BB:CC:DD:EE:FF
```

The script prints each step as it goes — connecting, creating the `Plans/`
folder, writing a small test file, then listing that folder back as a
self-check. **Copy the full console output** (or a screenshot of the
terminal) — that's the primary record of what the BLE side actually did.

## Step 5: check the watch

1. Press the top right button and open **Intervals Probe**.
2. It works for a moment, then shows one screen: a **verdict**, what it
   means, how many files are in `Plans/`, the newest file's name and size,
   and a preview of its content.
3. **Take a photo of the screen.**
4. Press **R2** (bottom right) to close it.

| Verdict | Meaning |
|---|---|
| **GO** (lime) | A file arrived and was read back — the transport works. |
| **EMPTY** | The `Plans/` folder exists but nothing has arrived yet — re-run `send_plan.py` and try again. |
| **FOLDER FAILED** (red) | The app couldn't create or open its own `Plans/` folder — this would be a bug in the probe itself, not the transport; send this over. |

## Step 6: send the results back

1. Connect the watch by USB again.
2. Copy these two files off the watch, from `Apps/HXIntervalsProbe/`:
   - **`probe.txt`**: the full report of the latest run;
   - **`probe-history.txt`**: one line per run.
3. Send both files, plus the script's console output and the photo, back in
   this conversation.

## Step 7: remove the probe (optional)

Delete the `Apps/HXIntervalsProbe/` folder over USB and power-cycle the watch.
Nothing else needs cleaning up.

---

## What the probe does

- **The watch app** (`Tools/Probe/Software`): on open, lists its own `Plans/`
  folder (creating it first if it doesn't exist yet), counts the files,
  finds the newest by modification time, and reads a safely-printable preview
  of its first bytes. No parsing of the file's structure — that's for a real
  phone app later, out of scope here.
- **`send_plan.py`** (`Tools/Probe/pc`): a standalone script using `bleak`
  (works on macOS/Windows/Linux, no phone or app store needed), implementing
  only the classic BLE File Transfer Service flow
  (`una-sdk/Docs/BLE-File-Transfer-Service.md`): `MKDIR` the app's `Plans/`
  folder (idempotent), `WRITE`/`WRITE_DATA` a small JSON test file, then
  `LISTDIR` as a self-check. The test payload is a trivial stub —
  `{"probe": true, "sentAt": "<ISO time>"}` — not the real workout format,
  which is out of scope for P0.

## If it is not GO

Nothing is lost, but the shape of the product changes: the fallback is
`AppConfig`'s compact single-preset path rather than a full phone-app builder
(`NOTES.md`, Gate P0). **No workaround will be attempted.**

## For developers

- **Watch app code:** `Tools/Probe/Software/`
  - the check: `Libs/Sources/ProbeRunner.cpp`, pure C++ over `IFileSystem`;
  - the service: `Libs/Sources/Service.cpp`;
  - the screen: `Apps/Probe-GUI/gui/src/ProbeGui.cpp`.
- **PC-side client:** `Tools/Probe/pc/send_plan.py`, against the pure
  encode/decode module `Tools/Probe/pc/protocol.py`.
- **Host tests:** `Tests/Host/ProbeRunnerTest.cpp`, against a small in-memory
  `FlatFileSystem` fake.
- **Protocol tests:** `Tools/Probe/pc/test_protocol.py` (stdlib `unittest`) —
  packet encode/decode round-trips against the documented byte offsets. No
  BLE hardware needed; this is the part of P0 verifiable without a watch.
- **Simulator:** `Tools/Probe/Software/Apps/Probe-GUI/simulator`. Its mock
  file system is plain host-directory traversal (rooted at
  `../../../../../Output/` from the working directory, per the SDK's
  `Simulator/Kernel.cpp`), so it proves the watch-side reading logic only —
  it says nothing about the BLE transfer itself.
