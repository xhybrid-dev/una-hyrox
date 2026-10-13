# Store images

Two sets, one per store listing. They are made by
`docs/experiments/store_images.py`:

```
UNA_SDK=... python3 x-weighin/docs/experiments/store_images.py x-weighin/build-tests/glance_preview x-weighin/docs/store
```

| Folder | What | Size |
|---|---|---|
| `app/previews/` | The five screens as uploaded in the package (`Resources/previews/`) | 480×480 |
| `app/mockups/` | Each screen on a watch | 1080×1080 |
| `app/marketing/` | Four feature cards; a wide banner | 1080×1080; 1920×1080 |
| `glance/previews/` | The three glance previews in its package (`Resources/Glance/previews/`) | 480×480 |
| `glance/mockups/` | Each on a watch | 1080×1080 |
| `glance/marketing/` | Three feature cards; a wide banner | 1080×1080; 1920×1080 |

**What is real, and what is drawn:**
- **The app's screens** are simulator captures (`capture_screens.sh`) from a
  made-up 40-day log.
- **The glance** is drawn from the real layout code's output, because the
  simulator cannot run glances. It sits between the glances list's separator
  lines, as on the watch (NOTES, Gate W3).
- **Drawn here:** only the watch body, the backgrounds and the headlines.
  - The watch is a plain round sports watch with the four button labels, and
    no maker's logo.
  - The headlines make only the claims in `SUBMISSION.md`'s checked store text.

**Sizes:** UNA's portal and SDK give no image sizes. The previews match the ones
in the packages. The others are common square and 16:9 sizes; resize if the
portal asks for others.
