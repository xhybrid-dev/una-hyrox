// Stills at chosen times, for checking framing, legibility and overlaps.
//
//   npm run shots -- --ratio 16x9 --t 2,4.5,8        → out/shots/16x9-002.00.png ...
//   npm run shots -- --ratio all --scene race          (the scene's key times)

import fs from 'node:fs';
import path from 'node:path';
import { openFilm, root } from './harness';
import { RATIOS, SCENES, type Ratio } from '../src/config';

const arg = (k: string) => {
  const i = process.argv.indexOf(`--${k}`);
  return i > 0 ? process.argv[i + 1] : undefined;
};

/** A few telling moments in each scene. */
const KEY: Record<string, number[]> = {
  intro: [0.8, 2.4, 3.8, 4.8],
  family: [6.4, 8.2, 11.9, 14.3],
  week: [15.6, 16.2, 19.9, 23.8, 26.2],
  race: [27.5, 29.6, 31.9, 35.0, 40.2, 41.0, 42.4],
  streak: [43.5, 44.9, 46.3, 46.9, 48.6, 50.6, 52.4, 54.0],
  outro: [55.8, 57.0, 58.5, 61.0],
};

const ratios = (arg('ratio') ?? '16x9') === 'all' ? (Object.keys(RATIOS) as Ratio[]) : [(arg('ratio') ?? '16x9') as Ratio];
const scene = arg('scene');
const times = arg('t')?.split(',').map(Number) ?? (scene ? KEY[scene] : SCENES.flatMap((s) => KEY[s.id]));
const out = path.join(root, 'out', 'shots');
fs.mkdirSync(out, { recursive: true });

for (const r of ratios) {
  const film = await openFilm(r);
  try {
    for (const t of times) {
      const t0 = Date.now();
      const png = await film.draw(t);
      const file = path.join(out, `${r}-${t.toFixed(2).padStart(6, '0')}.png`);
      fs.writeFileSync(file, png);
      console.log(`${file}  (${Date.now() - t0} ms)`);
    }
  } finally {
    await film.close();
  }
}
