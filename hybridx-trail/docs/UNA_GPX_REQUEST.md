# Draft note to UNA: sending a GPX from the phone to a watch app

Status: **draft, not sent.** Jon to check the sign-off and the size limit
before sending. Kept here so the proposal and the brief stay in step.

---

**Subject:** Feature request: send a GPX route from the UNA app to a third-party watch app

Hi UNA team,

I'm Jon Lee. I run HybridX, a coaching platform for HYROX athletes, runners and
trail runners, and I'm building apps for the UNA Watch with your SDK. Next I'm
building a simple breadcrumb navigation app, like the older entry-level
Garmins: load a GPX route, follow the line, and get a buzz if you go off
course.

The watch side looks fully doable with the SDK as it stands. `GPS_LOCATION`,
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
3. **As a fallback, can a second phone app connect to a paired watch and write
   files over the BLE File Transfer Service (0xFEBB)** while the watch stays
   paired with the UNA app? Is that allowed and supported on iOS and Android?

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
