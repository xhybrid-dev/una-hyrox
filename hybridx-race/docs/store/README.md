# Store artwork for HybridX Race

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
