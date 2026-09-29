# Draft note to UNA: sending a GPX from the phone to a watch app

Status: **draft, not sent.** Jon to check the sign-off and the size limit
before sending. Kept here so the proposal and the brief stay in step. Updated
29 September 2026 with what the tests showed (NOTES, "Phone delivery over
BLE"). A short forum version comes first; the full note follows.

---

## Forum post (short, non-technical)

**Title:** Sending a GPX route from the phone to a watch app

> Hi all. I'm building a route-following app for the UNA Watch: load a GPX,
> follow the line, and get a buzz if you go off course. The watch side works
> well. The sticking point is getting the route onto the watch. At the moment
> it's a USB cable and copying the file into the app's folder. That's fine for
> me, but not for everyday runners.
>
> Is there, or will there be, a way to send a file such as a GPX from the phone
> to a particular watch app through the UNA app? For example: Share → UNA →
> "Send to HybridX Trail".
>
> If not, is it OK for a separate phone app to send the file to the watch over
> Bluetooth while the UNA app stays connected? I've tried this on Android and
> it works, but I'd rather do it the supported way. There's also no way to do
> it on iPhone without the UNA app.
>
> Thanks, Jon (HybridX)

---

## Full note

**Subject:** Feature request: send a GPX route from the UNA app to a third-party watch app

Hi UNA team,

I'm Jon Lee. I run HybridX, a coaching platform for HYROX athletes, runners and
trail runners, and I'm building apps for the UNA Watch with your SDK. Next I'm
building a simple breadcrumb navigation app, like the older entry-level
Garmins: load a GPX route, follow the line, and get a buzz if you go off
course.

The watch side is built and works with the SDK as it stands. `GPS_LOCATION`,
the magnetometer bearing, `TrackMapBuilder` and `IFileSystem` cover everything
the app needs on the watch. For now routes go on by USB, copied into the app's
folder. That works for me, but it's a laptop job the night before, not
something an athlete can do from their phone at the trailhead.

**What I'm asking**

1. **Can a third-party watch app currently receive a user-chosen file, such as
   a `.gpx`, through the UNA app?** From the docs, the UNA app writes only the
   `configFile` values file into an app's folder, so I believe the answer is
   no, but please correct me.
2. **If not, would you consider adding it?** A suggested design is below.
3. **As a fallback, is it supported for a second phone app to write files
   over the BLE File Transfer Service (0xFEBB)** while the UNA app stays
   connected? On Android it works technically. Through the phone's existing
   bond, a second app connected and listed `/Apps` over FTS while the UNA app
   stayed connected. We'd like to know it won't clash with the UNA app's own
   syncing, and whether you'd rather we didn't. On iPhone we can't do it at
   all without an Apple developer account, so for iPhone users only the UNA
   app can offer this.

**Suggested design**

It follows the same pattern as `configFields`: the manifest declares what the
app accepts, the UNA app writes a file into the app's folder, and the watch app
reads it.

- **Manifest:** an optional key, for example:
  ```json
  "acceptedFiles": [
    { "extension": "gpx", "folder": "Routes", "maxBytes": 2097152, "label": "Routes" }
  ]
  ```
  The key name and fields are only a suggestion. Please use whatever suits
  your schema.
- **Phone:** the UNA app registers as able to open `.gpx` files on iOS and
  Android. A runner exports a route from OS Maps, Komoot, Strava or similar,
  taps Share, then UNA. The UNA app lists the installed watch apps that accept
  `.gpx` ("Send to HybridX Trail") and writes the file to
  `2:/Apps/<AppDir>/Routes/<name>.gpx` using the existing file transfer.
- **Rules:** the same safety rules as `configFile`: a plain filename with no
  path separators or `..`, written only inside the app's own folder, and a size
  limit set in the manifest.
- **Optional:** a way in the UNA app to list and delete an app's received
  files, so old routes don't build up.
- **Watch app:** no new SDK API is needed. The app lists `Routes/` and reads
  the file with `IFileSystem`.

The watch app parses the GPX in small chunks with a fixed-size buffer, so the
UNA app can pass the file through unchanged.

**A small docs note.** `Docs/BLE-Services-Overview.md` lists a Nordic UART
service (`6E400001-…`), but my watch (UNA WATCH 042648) doesn't expose one. It
does expose two custom `554e4100-…` services. It may just be the docs being
ahead of or behind the firmware.

**Why it's worth doing**

Route loading is one of the most-asked-for features for runners and hikers. A
general "send a file to an app" route would also help others: workout plans,
race courses, interval sessions and so on. And it uses what you already have:
the file transfer service and the app-folder rules.

I'm happy to share the watch-side app as a reference, test early builds, or
write this up in more detail.

Many thanks,
Jon Lee
HybridX
