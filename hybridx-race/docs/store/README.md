# Store artwork for HybridX Race

## Store images (the X family, 9 November 2026)

HybridX Race in the X family's **Classic** colourway. Made by `hybridx-link/Tools/Brand/store_images.py`
(the system, the colours and what is real: `hybridx-link/Tools/Brand/README.md`). Upload them in
this order: the banner first, then the squares.

| File | Size |
|---|---|
| `01-banner.png` | 1920 × 1080, the listing's first image |
| `02-one-button-every-split.png` | 1080 × 1080 |
| `03-every-run-every-station.png` | 1080 × 1080 |
| `04-on-your-marks.png` | 1080 × 1080 |
| `05-the-whole-race.png` | 1080 × 1080 |
| `06-the-x-family.png` | 1080 × 1080 |

- The watch screens are real simulator captures (`Resources/previews/`).
- The watch is UNA's own render, from the SDK's Figma pack.
- The headlines are claims from `hybridx-link/docs/STORE_RELEASE.md`'s checked facts.
- The icon at 1024 px is `hybridx-link/docs/brand/icons/race-1024.png`.

## Earlier artwork (HybridX Race 1.0.0)

For the app's page on apps.unawatch.com, not the watch package.

- `hybridx-race-icon-1024.png`, `-512.png`: the store icon, the brand X mark
  (lemon and cyan arms, Race's station and run colours) over the "HybridX Race"
  wordmark, on brand ink. `-1024-transparent.png` is the same without the
  background. Drawn by `tools/store_icon.py` from the vector mark in
  `promo/lib/brand.mjs`.
- `screens/`: the real app's screens from the simulator, round-masked, 480 px
  (twice the watch's 240). They come from a real-time full race
  (`tools/capture_race_realtime.sh`, about an hour: a 59:59 finish with
  realistic splits) and a short Roxzone run. The times are a simulated example,
  not anyone's result.
- The watch mock-ups (each screen in UNA's watch, 1080 x 1350, captioned and
  plain) are made by `tools/mockup.py` from UNA's renders in the SDK's Figma UI
  kit (`promo/tools/una_mockups.py` extracts them). They are UNA's artwork, so,
  as for the promo films, they are not committed here.
