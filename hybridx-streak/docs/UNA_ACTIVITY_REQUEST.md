# Draft note to UNA: activity summary for third-party watch apps

Status: **draft, not sent.** Jon to check the wording before sending. Evidence
is in NOTES, "The UNA Android app (v2.2.8): what sync does".

---

## Forum post (short, non-technical)

**Title:** A way for watch apps to see recent activities

> Hi all. I'm building a weekly streak app for the UNA Watch. It counts every
> workout you do, from any app, against a weekly target. The catch is that the
> UNA app removes each activity file from the watch once it has synced, and
> watch apps can't see other apps' folders anyway, so my app can't tell what
> you've done.
>
> Could the UNA app leave a small summary on the watch after each sync: date,
> activity type and duration for the last few weeks? Apps could read it and
> show things like "3 of 4 sessions this week" without any extra access.
>
> Thanks, Jon (HybridX)

---

## Full note

**Subject:** Feature request: a recent-activities summary file for watch apps

Hi UNA team,

I'm Jon Lee, of HybridX, building apps for the UNA Watch with your SDK. Thank
you for the APK.

**The need.** HybridX Streak counts a weekly target across every activity the
watch records, from any app. On the watch it cannot see them:
- an app can open only its own folder and `../SharedData/`; I tested reads of
  sibling apps' `Activity/summary.json` by exact path and all were refused;
- the UNA app deletes each `.fit` from the watch after syncing it, so nothing
  remains to count.

**What I found in the UNA app (v2.2.8).** I read its string table only. I did
not decompile it or call any UNA service. It has no Health Connect
integration, and the activity history is held in the app's own storage and
your backend. I have not used or tested any of that, and will not rely on it
without your agreement.

**Request (preferred).** After each sync, the UNA app writes a small file,
for example `/Apps/SharedData/UNA/recent_activities.json`, holding the last
28 days:
- start time (UTC seconds), activity type or source app, duration in
  seconds; distance optional;
- at most about 100 entries, under 4 KB, rewritten each sync (the sync
  already writes settings files over the File Transfer Service).

Watch apps could then read it with the file API they already have. It also
works for iPhone users, and needs no new SDK interface.

**Alternatives that would also serve:**
1. An SDK "activity saved" event or an activity-list API for background
   services.
2. A documented, supported way for a phone app to read the activity list
   (for example Health Connect export).

**Questions:**
1. Is something like this planned or feasible?
2. Does `latest_activity.txt` on the watch stay after sync, and is it meant
   for apps to read?
3. May third-party apps use your backend API? If so, how do we authenticate?

Happy to test a beta.

Thanks,
Jon Lee, HybridX
