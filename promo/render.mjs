#!/usr/bin/env node
// Render a film.
//
//   node render.mjs race                  the whole film, in parallel parts,
//                                         with its soundtrack if one exists
//   node render.mjs race --still 12.5     one frame to out/stills/
//   node render.mjs race --sheet 5        a contact sheet, one frame per 5 s
//   node render.mjs race --from 20 --to 40 --preview
//                                         a quick, half-size clip of a range
//   node render.mjs race --cues           print the audio cue sheet (JSON)
//
// Frames are pure functions of time, so parts render independently and are
// joined without re-encoding.

import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { createCanvas } from '@napi-rs/canvas';
import { W, H, FPS } from './lib/core.mjs';
import { setupFonts } from './lib/gfx.mjs';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const OUT = path.join(HERE, 'out');
const VIDEOS = path.join(HERE, 'videos');

export function ffmpegPath() {
  if (process.env.FFMPEG) return process.env.FFMPEG;
  const guess = '/usr/local/lib/python3.11/dist-packages/imageio_ffmpeg/binaries/ffmpeg-linux-x86_64-v7.0.2';
  if (fs.existsSync(guess)) return guess;
  return 'ffmpeg';
}

function args() {
  const a = process.argv.slice(2);
  const o = { film: a[0] };
  for (let i = 1; i < a.length; i++) {
    const k = a[i].replace(/^--/, '');
    const v = a[i + 1] && !a[i + 1].startsWith('--') ? a[++i] : true;
    o[k] = v;
  }
  return o;
}

async function loadFilm(id) {
  const mod = await import(path.join(HERE, 'films', `${id}.mjs`));
  return mod.default;
}

function x264Args(file, fps, w, h, crf = 18) {
  return [
    '-y', '-loglevel', 'error',
    '-f', 'rawvideo', '-pix_fmt', 'rgba', '-s', `${w}x${h}`, '-r', String(fps), '-i', '-',
    '-vf', 'scale=out_color_matrix=bt709:out_range=tv,format=yuv420p',
    '-c:v', 'libx264', '-preset', 'slow', '-tune', 'animation', '-crf', String(crf),
    '-x264-params', 'keyint=120:min-keyint=60',
    '-colorspace', 'bt709', '-color_primaries', 'bt709', '-color_trc', 'bt709', '-color_range', 'tv',
    '-movflags', '+faststart', file,
  ];
}

async function renderRange(film, f0, f1, file, { scale = 1, fps = FPS, crf = 18 } = {}) {
  setupFonts();
  const w = Math.round(W * scale), h = Math.round(H * scale);
  const canvas = createCanvas(w, h);
  const ctx = canvas.getContext('2d');
  const ff = spawn(ffmpegPath(), x264Args(file, fps, w, h, crf), { stdio: ['pipe', 'inherit', 'inherit'] });
  const t0 = Date.now();
  for (let f = f0; f < f1; f++) {
    const t = f / fps;
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.globalAlpha = 1;
    ctx.globalCompositeOperation = 'source-over';
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, w, h);
    if (scale !== 1) ctx.scale(scale, scale);
    film.draw(ctx, t);
    const buf = canvas.data();
    if (!ff.stdin.write(Buffer.from(buf.buffer, buf.byteOffset, buf.byteLength))) {
      await new Promise((r) => ff.stdin.once('drain', r));
    }
    if ((f - f0) % 300 === 0 && process.env.VERBOSE) {
      process.stderr.write(`${film.id} frame ${f}/${f1} ${((Date.now() - t0) / Math.max(1, f - f0)).toFixed(1)} ms/f\n`);
    }
  }
  ff.stdin.end();
  await new Promise((r, j) => ff.on('close', (c) => (c === 0 ? r() : j(new Error(`ffmpeg exited ${c}`)))));
}

async function still(film, times, dir) {
  setupFonts();
  fs.mkdirSync(dir, { recursive: true });
  const canvas = createCanvas(W, H);
  const ctx = canvas.getContext('2d');
  const files = [];
  for (const t of times) {
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.globalAlpha = 1;
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, W, H);
    film.draw(ctx, t);
    const f = path.join(dir, `${film.id}_${String(t.toFixed(2)).padStart(7, '0')}.png`);
    fs.writeFileSync(f, await canvas.encode('png'));
    files.push(f);
  }
  return files;
}

async function sheet(film, step, file, from = 0, to = null) {
  setupFonts();
  const end = to ?? film.duration;
  const times = [];
  for (let t = from; t < end - 1e-6; t += step) times.push(+t.toFixed(3));
  const cols = 6;
  const tw = 320, th = 180;
  const rows = Math.ceil(times.length / cols);
  const sh = createCanvas(cols * tw, rows * (th + 18));
  const sctx = sh.getContext('2d');
  sctx.fillStyle = '#111';
  sctx.fillRect(0, 0, sh.width, sh.height);
  const canvas = createCanvas(W, H);
  const ctx = canvas.getContext('2d');
  times.forEach((t, i) => {
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.globalAlpha = 1;
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, W, H);
    film.draw(ctx, t);
    const x = (i % cols) * tw, y = Math.floor(i / cols) * (th + 18);
    sctx.drawImage(canvas, x, y + 18, tw, th);
    sctx.fillStyle = '#888';
    sctx.font = '12px "JetBrains Mono"';
    sctx.fillText(`${t.toFixed(2)}s`, x + 4, y + 13);
  });
  fs.mkdirSync(path.dirname(file), { recursive: true });
  fs.writeFileSync(file, await sh.encode('png'));
  return file;
}

function run(cmd, argv) {
  return new Promise((r, j) => {
    const p = spawn(cmd, argv, { stdio: ['ignore', 'inherit', 'inherit'] });
    p.on('close', (c) => (c === 0 ? r() : j(new Error(`${cmd} exited ${c}`))));
  });
}

async function main() {
  const o = args();
  if (!o.film) {
    console.error('usage: node render.mjs <race|streak|trail> [--still t,t] [--sheet step] [--from s --to s] [--preview] [--cues]');
    process.exit(1);
  }
  const film = await loadFilm(o.film);

  if (o.cues) {
    process.stdout.write(JSON.stringify({ id: film.id, duration: film.duration, bpm: film.bpm, sections: film.sections || [], cues: film.cues() }, null, 1));
    return;
  }
  if (o.still) {
    const times = String(o.still).split(',').map(Number);
    const files = await still(film, times, path.join(OUT, 'stills'));
    console.log(files.join('\n'));
    return;
  }
  if (o.sheet) {
    const f = await sheet(film, Number(o.sheet), path.join(OUT, `${film.id}_sheet.png`), o.from ? Number(o.from) : 0, o.to ? Number(o.to) : null);
    console.log(f);
    return;
  }
  if (o.part !== undefined) {
    // A child: render frames [f0, f1) to a part file.
    const f0 = Number(o.f0), f1 = Number(o.f1);
    await renderRange(film, f0, f1, o.out, { scale: Number(o.scale || 1), fps: Number(o.fps || FPS), crf: Number(o.crf || 18) });
    return;
  }

  const preview = !!o.preview;
  const fps = Number(o.fps || (preview ? 30 : FPS));
  const scale = preview ? 0.5 : Number(o.scale || 1);
  const from = Number(o.from || 0);
  const to = Number(o.to || film.duration);
  const F0 = Math.round(from * fps), F1 = Math.round(to * fps);
  const jobs = Number(o.jobs || Math.max(1, os.cpus().length));
  fs.mkdirSync(OUT, { recursive: true });
  const parts = [];
  const per = Math.ceil((F1 - F0) / jobs);
  const t0 = Date.now();
  const kids = [];
  for (let i = 0; i < jobs; i++) {
    const a = F0 + i * per, b = Math.min(F1, a + per);
    if (a >= b) break;
    const file = path.join(OUT, `${film.id}_part${i}.mp4`);
    parts.push(file);
    kids.push(run(process.execPath, [fileURLToPath(import.meta.url), o.film, '--part', String(i), '--f0', String(a), '--f1', String(b),
      '--out', file, '--scale', String(scale), '--fps', String(fps), '--crf', String(o.crf || 18)]));
  }
  await Promise.all(kids);
  const list = path.join(OUT, `${film.id}_parts.txt`);
  fs.writeFileSync(list, parts.map((p) => `file '${p}'`).join('\n'));
  const silent = path.join(OUT, `${film.id}${preview ? '_preview' : ''}_video.mp4`);
  await run(ffmpegPath(), ['-y', '-loglevel', 'error', '-f', 'concat', '-safe', '0', '-i', list, '-c', 'copy', silent]);
  parts.forEach((p) => fs.unlinkSync(p));
  fs.unlinkSync(list);

  // Mux the soundtrack, trimmed to the rendered range.
  const wav = path.join(HERE, 'audio', 'out', `${film.id}.wav`);
  let final = silent;
  if (fs.existsSync(wav) && !o.noaudio) {
    fs.mkdirSync(VIDEOS, { recursive: true });
    final = preview ? path.join(OUT, `${film.id}_preview.mp4`) : path.join(VIDEOS, `hybridx-${film.id}.mp4`);
    await run(ffmpegPath(), ['-y', '-loglevel', 'error', '-i', silent, '-ss', String(from), '-t', String(to - from), '-i', wav,
      '-map', '0:v', '-map', '1:a', '-c:v', 'copy', '-c:a', 'aac', '-b:a', '256k', '-ar', '48000', '-shortest', '-movflags', '+faststart', final]);
  }
  console.log(`${final}  (${((Date.now() - t0) / 1000).toFixed(1)} s)`);
}

main().catch((e) => {
  console.error(e);
  process.exit(1);
});
