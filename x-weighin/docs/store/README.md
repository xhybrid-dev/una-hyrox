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
- **The watch** is UNA's own render, from the SDK's Figma UI Resource Pack
  (`una-sdk/Docs/Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig`; see
  `Docs/figma-ui-kit.rst`).
  - It comes in black, teal and white, each 2000×2000 with the screen
    transparent. The opening is a 734 px circle at (999.5, 1010.5), measured
    from the alpha channel.
  - The script reads the renders from `$UNA_SDK` when it runs; none is copied
    into this repository.
  - The mock-ups and cards use black; the banners add teal and white behind.
- **Drawn here:** only the backgrounds and the headlines. The headlines make
  only the claims in `SUBMISSION.md`'s checked store text.

**Sizes:** UNA's portal and SDK give no image sizes. The previews match the ones
in the packages. The others are common square and 16:9 sizes; resize if the
portal asks for others.
