# The Bike Probe: Gate B0, step by step

Can the UNA Watch read a smart trainer, bike speed, cadence or power sensors,
or a Concept2 BikeErg? The SDK says it can only connect heart-rate straps
today, with spare slots "reserved" for cadence and power. The **Bike Probe**
asks the watch for each of those slots in turn and writes down exactly what
the watch answers. It changes nothing belonging to any other app.

It takes about twenty minutes, indoors, with the devices beside you.

---

## What you need

- The watch, its USB cable and a computer.
- Your heart-rate strap (the control: it must work, or nothing else means
  anything).
- The smart trainer, the bike speed/cadence/power sensors, and the BikeErg,
  whichever you have to hand.
- Your phone with the UNA app, for step 3.

## Step 1: download and install the probe

1. On GitHub, open the repository and choose the **Actions** tab.
2. Pick the newest **Watch builds** run with a green tick on the branch.
3. At the bottom of the run page, under **Artifacts**, download **watch-apps**
   and unzip it. Inside is `RideProbe_0.1.0.uapp`. Only use this GitHub-built
   copy.
4. Connect the watch by USB and wait for its drive to appear.
5. On the watch drive, open `Apps/`, create a folder named exactly
   **`RideProbe`**, and copy the `.uapp` into it.
6. Eject the drive **safely**, unplug, and power the watch off and on again.

## Step 2: switch on external heart rate

The watch only looks for a sensor if external heart rate is switched on in
its own settings, and a strap is remembered.

1. In the watch's **Settings**, find the external heart-rate (or sensors)
   option and switch it on.
2. If the watch has never paired your strap, pair it there now.
3. Note what the menu offers. **Does it list anything besides heart rate**
   (speed, cadence, power, a bike or a trainer)? Write it down either way.

## Step 3: look in the UNA phone app

1. Open the UNA app and look through its device and sensor settings.
2. **Does it offer cycling sensors or trainers?** Write down what you find,
   with a screenshot if there is anything.

The SDK can't tell us this part, so it matters as much as the probe.

## Step 4: run the probe, one device at a time

Open **Bike Probe** from the app list. The screen shows:

- the title, **BIKE PROBE**;
- in amber, what **R1** will ask for next;
- the heart-rate line: where the heart rate is coming from (**wrist** or
  **strap**), and the strap's bpm when there is one;
- the log, newest at the bottom: each request, and each answer as
  "seconds since the request, the slot, the state, the device's name".

The states the watch can answer are: **UNAVAILABLE** (switched off, or nothing
remembered), **IDLE**, **SEARCHING**, **CONNECTING**, **CONNECTED** and
**LOST**.

Each press of **R1** asks for the next slot, in this order, then starts again:

| Press | Asks for | |
|---|---|---|
| 1 | HRM (control) | heart-rate strap |
| 2 | bit 1 (cadence?) | the reserved cadence slot |
| 3 | bit 2 (power?) | the reserved power slot |
| 4 | bits 1+2 | both reserved slots |
| 5 | bits 0+1+2 | heart rate and both reserved slots |

**L1** and **L2** scroll the log (newer, older). **R2** closes the probe.

Do these four rounds. In each, wake only the device named, press R1 five
times, **waiting about 20 seconds after each press**, and note the states you
see:

1. **The strap only.** Put the strap on (wet the contacts). Press 1 should
   reach **CONNECTED** with the strap's name, and the heart-rate line should
   say **strap**. If it doesn't, stop: check step 2 before going on.
2. **The smart trainer.** Strap off. Power the trainer and pedal so it is
   awake. Close and reopen the probe, then the five presses.
3. **The bike sensors.** Trainer off. Spin the wheel and the cranks to wake
   the speed, cadence and power sensors. Close and reopen, then the five
   presses.
4. **The BikeErg.** Sensors asleep. Turn the PM5 on and switch its Bluetooth
   on (the PM5's connect screen). Close and reopen, then the five presses.

## Step 5: copy the logs

Each opening of the probe writes one log.

1. Connect the watch by USB and wait for its drive.
2. Copy everything in `Apps/RideProbe/Logs/` (files named
   `probe_<number>.csv`) to your computer.
3. Eject the drive **safely**.
4. Send me the CSV files, your notes from steps 2 and 3, and anything you
   noticed on screen.

## What I expect, and what it would mean

- **Most likely:** press 1 reaches CONNECTED with the strap, and presses 2 to
  5 answer UNAVAILABLE or IDLE, or nothing at all, with every bike device.
  That confirms the SDK: heart rate only. The request to UNA
  (`UNA_BIKE_SENSORS_REQUEST.md`) then goes with this evidence, and a bike
  app either waits or works on GPS and heart rate alone.
- **A surprise:** a reserved slot reaches SEARCHING or CONNECTED with a bike
  device. The firmware is ahead of the SDK. An app still can't read the data
  until UNA adds sensor types for it, so this goes to UNA too.
- **The heart-rate line says strap with no strap on:** the trainer or the PM5
  is passing on a heart rate. That's heart rate only, not power or cadence.

To remove the probe afterwards, delete the `Apps/RideProbe` folder over USB.
