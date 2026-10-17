# Draft email to UNA: opening up how watch apps share data

Status: **draft, not sent.** Jon to check it before sending. It covers the
asks in `hybridx-streak/docs/UNA_ACTIVITY_REQUEST.md` and
`hybridx-trail/docs/UNA_GPX_REQUEST.md` at a higher level; those two hold the
technical detail if UNA asks for it.

---

**Subject:** Helping watch apps work together: two ideas from a developer

Hi UNA team,

I'm Jon Lee from HybridX, a coaching platform for HYROX athletes and runners.
Over the past few months I've built several apps for the UNA Watch with your
SDK: a HYROX race timer, a weekly training streak, route navigation, structured
interval workouts, a weight tracker and a running app with a live VO2max
estimate. The SDK has been a pleasure to work with, so thank you.

Building them showed one theme. Each app on the watch is a sealed box. That's
right for safety, but it means apps can't build on each other or on what the
athlete does elsewhere. Today information only moves in two ways: settings go
from the phone into an app, and activity files go from the watch to the phone,
after which the UNA app removes them from the watch. Two small additions would
open up a lot.

**1. A shared activity summary on the watch**

After each activity, a short summary (date, type, duration, and perhaps
distance and heart rate) would be written to the shared folder on the watch,
which every app can already read. This could be done by each app as it
finishes, or by the UNA app after each sync.

My streak app could then count every workout against a weekly target,
whichever app recorded it. A training-load or recovery app could look across
the whole week. My other apps already put their own results there (VO2max,
weigh-ins), so this would make it a common habit rather than a one-off.

It would also help if the UNA app could copy that shared folder to the phone
when it syncs. At the moment, results that aren't in the activity file (such
as my VO2max estimate) never leave the watch.

**2. Sending files from the phone to a watch app**

Some apps need more than settings: a GPX route for navigation, a workout plan,
a race course. At the moment the only way in is a USB cable and a laptop,
which isn't realistic for most athletes.

The idea is simple: in the UNA app, Share a file (for example a GPX from
Komoot, Strava or OS Maps), pick "Send to HybridX Trail", and the UNA app puts
it in that app's folder on the watch. Each app would say in its manifest which
file types it accepts, much as it lists its settings today. The watch side
needs nothing new.

On Android I've shown a separate phone app can do this over Bluetooth, but I'd
much rather use a supported route, and there's no way to do it on iPhone
without the UNA app.

**Questions**

1. Are either of these planned, or something you'd consider?
2. Until then, are you happy for apps to share data through the shared folder,
   and for an Android app to send files over Bluetooth while the UNA app is
   connected?

I've written up more detailed suggestions for both and I'm happy to share
them, test early builds, or have a call. I also have a short list of smaller
technical questions (motion sensor settings, glance drawing, FIT strength
sets) that I can send separately.

Many thanks,
Jon Lee
HybridX

---

## Forum post (short, no reference to our apps)

**Title:** Feature ideas: letting watch apps share data and receive files

> A few ideas that would let third-party watch apps do much more:
>
> 1. **A shared activity summary.** After each activity, a short summary (date,
>    type, duration, distance, heart rate) is saved to the watch's shared
>    folder, so any app can see the whole week's training, not just its own.
>    Streak, training-load and recovery apps would all benefit.
> 2. **Shared data reaching the phone.** The UNA app copies the shared folder
>    to the phone when it syncs, so results an app works out on the watch
>    aren't stuck there.
> 3. **Sending files to a watch app.** Share a file in the UNA app (a GPX
>    route, a workout plan, a race course) and pick which watch app receives
>    it. Each app would list the file types it accepts, as it lists its
>    settings today. Right now the only way in is a USB cable, and on iPhone
>    there's no other route.
>
> Is anything like this planned? Thanks.
