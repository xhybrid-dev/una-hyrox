// Exports the film as H.264 MP4: Playwright steps the timeline frame by frame at the delivery size
// (1920 x 1080, 1080 x 1920 or 1080 x 1080), each frame is read back as PNG, and ffmpeg encodes.
//
//   npm run export -- --ratio 9x16 --fps 60
//   npm run export -- --ratio all
//   npm run export -- --ratio 16x9 --from 27 --to 43 --fps 30 --out out/race.mp4   (a part, for a look)
//
// ffmpeg: $FFMPEG, else `ffmpeg` on the PATH. Output: promo/videos/explainer/hybridx-explainer-<ratio>.mp4

import { spawn } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { openFilm, root } from './harness';
import { FPS, RATIOS, type Ratio } from '../src/config';

const arg = (k: string) => {
  const i = process.argv.indexOf(`--${k}`);
  return i > 0 ? process.argv[i + 1] : undefined;
};

const which = arg('ratio') ?? '16x9';
const ratios = which === 'all' ? (Object.keys(RATIOS) as Ratio[]) : [which as Ratio];
const fps = Number(arg('fps') ?? FPS);
const crf = arg('crf') ?? '17';
const ffmpeg = process.env.FFMPEG ?? 'ffmpeg';

for (const ratio of ratios) {
  if (!(ratio in RATIOS)) throw new Error(`unknown ratio ${ratio}`);
  const film = await openFilm(ratio);
  const from = Number(arg('from') ?? 0);
  const to = Number(arg('to') ?? film.duration);
  const out = path.resolve(root, arg('out') ?? path.join('..', 'videos', 'explainer', `hybridx-explainer-${ratio}.mp4`));
  fs.mkdirSync(path.dirname(out), { recursive: true });
  const frames = Math.round((to - from) * fps);
  // ffmpeg writes into the git-ignored out/ first; the film is moved into place only once it is
  // whole, so a stopped export never leaves half a video beside the finished ones.
  const part = path.join(root, 'out', `${path.basename(out, '.mp4')}.part.mp4`);
  fs.mkdirSync(path.dirname(part), { recursive: true });
  const enc = spawn(ffmpeg, [
    '-y', '-loglevel', 'error', '-f', 'image2pipe', '-framerate', String(fps), '-i', '-',
    '-c:v', 'libx264', '-preset', 'slow', '-crf', crf, '-pix_fmt', 'yuv420p', '-profile:v', 'high',
    '-movflags', '+faststart', part,
  ], { stdio: ['pipe', 'inherit', 'inherit'] });
  const done = new Promise<void>((ok, fail) => enc.on('close', (c) => (c === 0 ? ok() : fail(new Error(`ffmpeg exited ${c}`)))));
  const t0 = Date.now();
  try {
    for (let i = 0; i < frames; i++) {
      // Frame i shows the film at its start time: the same t always gives the same frame.
      const png = await film.draw(from + i / fps);
      if (!enc.stdin.write(png)) await new Promise((r) => enc.stdin.once('drain', r));
      if (i % fps === 0 || i === frames - 1) {
        const el = (Date.now() - t0) / 1000;
        const eta = (el / (i + 1)) * (frames - i - 1);
        process.stdout.write(`\r${ratio}: frame ${i + 1}/${frames}  ${(from + i / fps).toFixed(1)} s  elapsed ${el.toFixed(0)} s  eta ${eta.toFixed(0)} s   `);
      }
    }
  } finally {
    enc.stdin.end();
    await film.close();
  }
  await done;
  fs.copyFileSync(part, out);
  fs.unlinkSync(part);
  console.log(`\n${out}`);
}
