#!/usr/bin/env node
// Render a poster still for each film, and a strip of all three, to
// videos/posters/. Rendered straight from the films (not decoded from the
// MP4s), so they're lossless until the final JPEG.

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { createCanvas, loadImage } from '@napi-rs/canvas';
import { W, H } from './lib/core.mjs';
import { setupFonts } from './lib/gfx.mjs';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const OUT = path.join(HERE, 'videos', 'posters');

// A frame that says what each film is about.
const PICKS = { race: 31.0, streak: 48.6, trail: 97.6 };

setupFonts();
fs.mkdirSync(OUT, { recursive: true });
const canvas = createCanvas(W, H);
const ctx = canvas.getContext('2d');
const strip = createCanvas(W * 3 / 2, H / 2);
const sctx = strip.getContext('2d');
let i = 0;
for (const [id, t] of Object.entries(PICKS)) {
  const film = (await import(path.join(HERE, 'films', `${id}.mjs`))).default;
  ctx.setTransform(1, 0, 0, 1, 0, 0);
  ctx.globalAlpha = 1;
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, W, H);
  film.draw(ctx, t);
  const f = path.join(OUT, `hybridx-${id}.jpg`);
  const jpeg = await canvas.encode('jpeg', 90);
  fs.writeFileSync(f, jpeg);
  // Draw the strip from the encoded poster: Skia snapshots a reused canvas
  // lazily, so drawing the canvas itself would pick up the next film.
  sctx.drawImage(await loadImage(jpeg), i * (W / 2), 0, W / 2, H / 2);
  console.log(f);
  i++;
}
fs.writeFileSync(path.join(OUT, 'hybridx-films.jpg'), await strip.encode('jpeg', 88));
console.log(path.join(OUT, 'hybridx-films.jpg'));
