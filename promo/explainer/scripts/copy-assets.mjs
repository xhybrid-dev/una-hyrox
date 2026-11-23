// Copies what the explainer shows from elsewhere in the repo into public/gen/ (git-ignored, so
// nothing is duplicated in git):
//   icons/<app>.png        the family's store icons, 1024 px (hybridx-link/docs/brand/icons)
//   previews/<app>/<n>.png the apps' real simulator screens, 480 px (each app's Resources/previews)
//   type/*.woff            Space Grotesk 500 and 700 for the overlay (SIL OFL, @fontsource)

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, '..', '..', '..');
const GEN = path.join(HERE, '..', 'public', 'gen');

const ICONS = ['race', 'run', 'trail', 'ride', 'workout', 'training', 'streak'];
const PREVIEWS = {
  run: ['hybridx-run', '3-vo2max.png'],
  trail: ['hybridx-trail', '3-map.png'],
  ride: ['hybridx-ride', '2-riding.png'],
  workout: ['hybridx-workout', '5-circuit-station.png'],
  training: ['hybridx-training', '1-today.png'],
};

function copy(from, to) {
  fs.mkdirSync(path.dirname(to), { recursive: true });
  fs.copyFileSync(from, to);
}

for (const id of ICONS) {
  copy(path.join(ROOT, 'hybridx-link', 'docs', 'brand', 'icons', `${id}-1024.png`), path.join(GEN, 'icons', `${id}.png`));
}
for (const [id, [app, file]] of Object.entries(PREVIEWS)) {
  copy(path.join(ROOT, app, 'Resources', 'previews', file), path.join(GEN, 'previews', `${id}.png`));
}
// Simulator captures the screen renderer is checked against (scripts/screen-check.ts).
const REFS = {
  'race-menu': ['hybridx-race', 'Resources/previews/1-menu.png'],
  'race-marks': ['hybridx-race', 'Resources/previews/2-on-your-marks.png'],
  'race-run': ['hybridx-race', 'Resources/previews/3-race-run.png'],
  'race-split': ['hybridx-race', 'Resources/previews/5-split.png'],
  'race-finished': ['hybridx-race', 'docs/screens/phase4-15-finished.png'],
  'ride-saved': ['hybridx-ride', 'Resources/previews/6-saved.png'],
  'streak-home': ['hybridx-streak', 'Resources/previews/1-home.png'],
  'streak-summit': ['hybridx-streak', 'docs/screens/13-summit-screen.png'],
  'streak-load': ['hybridx-streak', 'docs/screens/real/27-training-load.png'],
};
for (const [id, [app, file]] of Object.entries(REFS)) {
  copy(path.join(ROOT, app, file), path.join(GEN, 'ref', `${id}.png`));
}
const fsg = path.join(HERE, '..', 'node_modules', '@fontsource', 'space-grotesk', 'files');
for (const w of [500, 700]) {
  copy(path.join(fsg, `space-grotesk-latin-${w}-normal.woff`), path.join(GEN, 'type', `space-grotesk-${w}.woff`));
}
console.log(`copy-assets: ${ICONS.length} icons, ${Object.keys(PREVIEWS).length} previews, 2 type faces`);
