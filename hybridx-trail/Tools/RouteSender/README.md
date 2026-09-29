# HybridX Route Sender (Android, test build)

A one-screen Android app that sends a GPX route from the phone to HybridX Trail
on a UNA Watch, over the watch's BLE File Transfer Service. You share or pick a
`.gpx` and tap **Send to watch**. The app finds Trail's folder, makes
`Routes/` if needed, writes the file, and checks it arrived intact.

It exists to prove phone delivery works before anything bigger is built (or
UNA adds it to their own app). Background, and why it works this way:
`hybridx-trail/docs/NOTES.md`, "Phone delivery over BLE".

## How it works

1. **Finds the watch without scanning.** The watch stops advertising while the
   UNA app is connected, so the app picks the **already-paired** device whose
   name contains "UNA" (for example "UNA WATCH 042648"). Android lets a second
   app share that connection. There's no pairing step and no location
   permission. Android 12+ asks once for **Nearby devices**.
2. **Opens FTS** (`0xFEBB`). It reads the version (5 on Jon's watch, meaning
   fast transfer) and turns on replies.
3. **LISTDIR `/Apps`** checks `HybridXTrail` is installed. If it isn't, it
   stops and says so.
4. **LISTDIR `…/Routes`**. If the folder is missing, it runs **MKDIR** and
   lists it again. The routes already there go in the log.
5. **WRITE** `/Apps/HybridXTrail/Routes/<name>.gpx`, from offset 0. On
   version 5 it streams with a 2 KB credit window and resends from the
   watch's acknowledged point on a stall; on version 4 it waits for each
   packet's acknowledgement. Both follow `una-sdk/Docs/BLE-File-Transfer-Service.md`.
6. **DIGEST** (version 5). The watch's CRC-32 and size must match the phone's,
   or the app reports a failure.

**Names:** the file name comes from the phone's name for the file, made safe
for Trail. That means letters, digits, spaces and `- _ ( )`, accents dropped
("Café" becomes "Cafe"), no leading dot, `.gpx` on the end, and at most 47
bytes (Trail's `RouteInfo::file[48]`). Sending a route with the same name
replaces it. Trail lists at most 16 routes; the log warns past that.

**Checks on the file:** it must contain `<gpx` near the start and be under 8 MB.

## Install it on your phone

1. Get the APK:
   - from Claude, as a file in the chat; or
   - from GitHub: **Actions → Route Sender (Android) →** the latest run **→
     Artifacts → route-sender-apk**. It downloads as a zip; open it and tap
     the `.apk` inside.
2. Tap the APK. Android asks to allow installs from that app (Files, Chrome
   or the Claude app); allow it, then tap **Install**. Play Protect may warn
   that it doesn't recognise the developer. Choose **More details → Install
   anyway**. It's a test build signed with a test key.
3. Later builds install over the top, keeping the permission, because every
   build uses the same test key (`debug.keystore`).

## Test plan (Gate: phone delivery)

Do these with the UNA app installed and connected as normal.

1. Open **Route Sender** and tap **Check watch**. Allow **Nearby devices** when
   asked.
   - Expect: "Watch OK: HybridX Trail found, file transfer version 5", with
     the routes already on the watch listed in the log.
2. Tap **Choose GPX file**, pick a route, and tap **Send to watch**.
   - Expect: a progress bar, then "Done. Open HybridX Trail on your watch…"
     and a log line starting `Verified:`. Note the `Written in … s` line.
3. Open **HybridX Trail** on the watch.
   - Expect: the new route in the list, and it loads and draws.
4. In another app (Files, Komoot, Strava, an email), open a GPX and choose
   **Share → HybridX Route Sender**. Send it.
   - Expect: the same result as step 2.
5. Send the same route again.
   - Expect: "Replacing …" in the log, with still only one copy on the watch.
6. Tap **Copy log** and paste the whole log into your reply to Claude.

Things worth noting if they happen:
- whether the UNA app complains or disconnects during or after a send;
- whether Trail needed closing and reopening to see the new route;
- how long a typical route took (the `Written in` line).

## Build it yourself

The build needs no Gradle or Android Studio: `build.sh` runs the host tests,
then compiles, dexes, packages and signs the app.

```bash
hybridx-trail/Tools/RouteSender/build.sh      # -> build/HybridXRouteSender-0.1.0.apk
```

It uses a full Android SDK if `$ANDROID_HOME` has one (CI does). Otherwise it
uses Ubuntu's packages:

```bash
sudo apt-get install android-sdk-platform-23 aapt apksigner zipalign dalvik-exchange
```

CI builds it on every change here (`.github/workflows/route-sender.yml`,
artifact `route-sender-apk`).

## Layout

| Path | What |
|---|---|
| `app/src/.../Fts.java` | FTS wire format: pure Java, host-tested |
| `app/src/.../RouteFile.java` | Watch file names and GPX checks: pure Java, host-tested |
| `app/src/.../WatchLink.java` | The Bluetooth session: connect, list, mkdir, write, digest |
| `app/src/.../MainActivity.java` | The one screen, file picking and sharing |
| `tests/.../HostTests.java` | Host tests, including the bytes from Jon's nRF Connect test |
| `app/AndroidManifest.xml` | Permissions, and the Share and Open-with entries for GPX |
| `make_icon.py` | Draws the launcher icon (Trail's icon, at phone size) |
| `debug.keystore` | **Test-only** signing key (password `android`). A store release needs a new key kept out of git |

## Not in this test build

These are deliberately left out for now:
- deleting routes from the watch;
- iPhone;
- a Play Store listing;
- coping with the UNA app syncing files at the same moment (both apps share
  one FTS channel);
- targeting Android 15's SDK level (35).
