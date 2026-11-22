// Checks that a frame depends on its time alone: draws a set of times, then the same times again
// in a different order (with other frames between), and compares the pixels byte for byte.
//
//   npm run check:determinism -- --ratio 16x9

import crypto from 'node:crypto';
import { openFilm } from './harness';
import type { Ratio } from '../src/config';

const i = process.argv.indexOf('--ratio');
const ratio = (i > 0 ? process.argv[i + 1] : '16x9') as Ratio;
const times = [2.4, 9.2, 19.9, 31.9, 41.6, 46.3, 52.4, 58.9];
const film = await openFilm(ratio);
const hash = (b: Buffer) => crypto.createHash('sha1').update(b).digest('hex').slice(0, 12);
try {
  const first = new Map<number, string>();
  for (const t of times) first.set(t, hash(await film.draw(t)));
  let bad = 0;
  for (const t of [...times].reverse()) {
    await film.draw(t + 3.3); // something else in between
    const h = hash(await film.draw(t));
    const ok = h === first.get(t);
    if (!ok) bad++;
    console.log(`${ok ? 'same' : 'DIFF'}  t=${t}  ${first.get(t)} ${h}`);
  }
  console.log(bad ? `determinism: ${bad} frame(s) differ` : 'determinism: every frame identical');
  process.exitCode = bad ? 1 : 0;
} finally {
  await film.close();
}
