# Store images

Made by `docs/experiments/store_images.py`:

```
UNA_SDK=... python3 knitting/docs/experiments/store_images.py knitting/docs/store
```

| Folder | What | Size |
|---|---|---|
| `previews/` | The four screens as uploaded in the package (`Resources/previews/`) | 480×480 |
| `mockups/` | Each screen on a **teal** and on a **white** watch (`<screen>-teal.png`, `<screen>-white.png`) | 1080×1080 |
| `marketing/` | Five feature cards (teal and white watches alternate) and a wide banner with both | 1080×1080; 1920×1080 |

**What is real, and what is drawn:**
- **The watch screens** are simulator captures (`docs/experiments/store_previews.sh`) of a
  made-up project: 6 rows, Set 3, 22 rows in all. In the "undo" and "reset"
  screens the button is held part-way, as the app shows it.
- **The watch** is UNA's own render, from the SDK's Figma UI Resource Pack
  (`una-sdk/Docs/Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig`; see
  `Docs/figma-ui-kit.rst`). The script reads the teal and white renders from
  `$UNA_SDK` when it runs; none is copied into this repository.
- **Drawn here:** only the backgrounds and the headlines. The headlines make
  only the claims in `SUBMISSION.md`'s checked store text.

**The cards:** Count every row; The next set in one press; Hold to undo a row;
Start again on purpose; Opens where you left off.

**Sizes:** UNA's portal and SDK give no image sizes. The previews match the
ones in the package. The others are common square and 16:9 sizes; resize if the
portal asks for others. The portal's upload takes the package zip, which holds
only the previews: the mock-ups and cards are for the store listing's gallery,
if it has one.
