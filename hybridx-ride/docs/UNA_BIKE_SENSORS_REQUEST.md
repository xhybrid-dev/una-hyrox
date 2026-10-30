# Draft request to UNA: cycling sensors for watch apps

Status: **draft, not sent.** The evidence section is filled in from Jon's Gate B0
result (30 October 2026; `NOTES.md` section 3), with the open items marked in
it; Jon checks it before sending. It sits alongside `UNA_ECOSYSTEM_REQUEST.md` (at the repository
root), which covers app-to-app data; this one is about sensors only.

---

**Subject:** Cycling sensors for SDK apps: enabling the reserved accessory kinds

Hi UNA team,

I'm Jon Lee from HybridX. I've built several apps for the UNA Watch with your
SDK (a HYROX race timer, a running app, route navigation, a weekly streak and
others) and I'd like to build an indoor and outdoor cycling app next. The one
thing stopping it is sensors.

**Where the SDK is today (v1.6.2)**

The external-sensor contract (`AccessoryMessages.hpp`, `ExternalSensors.md`)
is well designed: an app asks for an accessory kind, the kernel connects it,
and the readings arrive through the normal sensor pipeline. In v1 it acquires
heart-rate straps only (`Kind::HRM`). The header reserves `CADENCE = 1 << 1`
and `POWER = 1 << 2` "as cycling cadence/power profiles are added later", and
`SensorTypes.hpp` has no speed, cadence or power types.

**What I tested**

A small probe app asked the kernel for each kind in turn (HRM, bit 1, bit 2,
both, all three) and logged every `EventStatus`, with a heart-rate strap (the
control) and a smart turbo trainer that broadcasts power, cadence and speed
over Bluetooth.

Results:

- **Heart-rate strap:** works. Request `0x1` went SEARCHING, CONNECTING,
  CONNECTED in 6.6 s, and `HEART_RATE_EX` reported the external source.
- **Reserved bits:** requests for bit 1, bit 2, both, and all three drew no
  status for any kind but HRM (no SEARCHING, and not even UNAVAILABLE). The
  kernel ignores them.
- **Watch settings:** the watch sees and connects external heart-rate
  devices only; the trainer never appeared.
- *[Still to add before sending: the watch's firmware version; whether the UNA
  phone app offers any cycling-sensor pairing; the bike sensors and BikeErg,
  if Jon tries them.]*

Probe logs are available if useful.

**What I'm asking for**

1. **Enable the reserved accessory kinds** for the standard Bluetooth
   profiles:
   - Cycling Speed and Cadence (service 0x1816): wheel and crank sensors;
   - Cycling Power (service 0x1818): power meters and most smart trainers;
   - Fitness Machine (FTMS, service 0x1826): smart trainers, and indoor
     bikes and ergs, including the Concept2 PM5 on recent firmware. This
     probably wants its own kind bit.
2. **Sensor types an app can subscribe to**, in the same way as heart rate:
   speed, cadence and power (instantaneous watts, and ideally the pedal
   balance where the sensor sends it), with a source field like
   `HEART_RATE_EX`'s.
3. **Pairing in the watch's Settings** for these sensors, as for straps, so
   the app only has to ask for a kind.
4. *Later, if possible:* trainer control over FTMS (setting a target power
   for ERG mode). Reading alone would already be a big step.

**What it would make possible**

- **Indoor rides and turbo sessions:** power, cadence and heart rate, written
  into a proper cycling FIT file with power and cadence records, so Strava and
  Garmin Connect show the full ride.
- **HYROX and hybrid training:** BikeErg and indoor-bike intervals in
  structured sessions, timed and logged on the wrist.
- **Outdoor rides:** speed and distance from a wheel sensor where GPS is poor,
  and power for pacing.
- **Training load:** power-based load (as well as heart-rate based) for
  coaching.

I'm happy to test pre-release firmware or SDK builds on my watch and send
logs back.

Many thanks,
Jon Lee, HybridX
