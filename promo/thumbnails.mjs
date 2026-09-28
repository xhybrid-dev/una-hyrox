#!/usr/bin/env node
// YouTube thumbnails for the three films: 1280 x 720 JPEGs in
// videos/thumbnails/, drawn with the films' own kit (the lockup, UNA's watch,
// the apps' screens). Each is laid out at 1920 x 1080 and scaled down, so
// every edge is supersampled.
//
//   node thumbnails.mjs              all six, and a review sheet in out/
//   node thumbnails.mjs race         one film's pair, for a quicker look
//
// Each film has two: A, the main one, and B, a different hook to try against
// it with YouTube's "Test & compare" once the videos are public. All six share
// a layout so they read as a series on a channel page: the lockup top left,
// the headline on the left, the watch on the right. Nothing that matters sits
// bottom right, where YouTube shows the duration.

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { createCanvas, loadImage } from '@napi-rs/canvas';
import { W, H, TAU, hash, clamp } from './lib/core.mjs';
import { setupFonts, text, measure, light, rgba, roundRect } from './lib/gfx.mjs';
import { C, T, XM, XJOINTS, xMark } from './lib/brand.mjs';
import { watch, setWatchModel, buttonPos, ripple } from './lib/watch.mjs';
import * as UI from './lib/ui-race.mjs';
import * as S from './lib/ui-streak.mjs';
import * as TR from './lib/ui-trail.mjs';
import { contours, routeLine, toScreen } from './films/trail.mjs';
import { routeAt } from './lib/terrain.mjs';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const OUT = path.join(HERE, 'videos', 'thumbnails');
const TW = 1280, TH = 720;

const M = 100; // the left margin
const WATCH = { cx: 1470, cy: 548, d: 650, rot: -0.07 };
const HEAD = { y: 405, maxW: 800, size: 176 };

// --- Shared pieces -----------------------------------------------------------

/** The lockup as on the end card: the mark, its gaps lit, "HybridX" and the app. */
function lockup(ctx, name, accent, accent2, x = M, cy = 150, h = 66) {
  const markW = XM.w * h;
  const mx = x + markW / 2;
  xMark(ctx, mx, cy, h, C.white);
  for (const [key, col] of [['up', accent], ['down', accent2]]) {
    const [u, v] = XJOINTS[key];
    light(ctx, mx + (u - XM.w / 2) * h, cy + (v - 0.5) * h, h * 0.6, rgba(col, 0.8));
  }
  const s = { size: h * 0.62, weight: 600, tracking: -0.02, color: C.white };
  const wx = x + markW + h * 0.42;
  const base = cy + s.size * 0.36;
  const w = text(ctx, 'HybridX', wx, base, s);
  text(ctx, name, wx + w + h * 0.16, base, { ...s, weight: 300 });
}

/**
 * The headline: lines at one size, fitted to maxW, the first baseline at y.
 * Returns the size and the baseline of the last line.
 */
function headline(ctx, lines, y = HEAD.y, maxW = HEAD.maxW) {
  let size = HEAD.size;
  for (const l of lines) size = Math.min(size, (size * maxW) / measure(ctx, l.s, T.hero(size)));
  lines.forEach((l, i) => {
    ctx.save();
    ctx.shadowColor = 'rgba(0,0,0,0.9)';
    ctx.shadowBlur = 40;
    text(ctx, l.s, M - 6, y + i * size * 1.02, T.hero(size, l.color));
    ctx.restore();
  });
  return { size, last: y + (lines.length - 1) * size * 1.02 };
}

/** The small line under everything: what it is, and what it's for. */
function tagline(ctx, str, y) {
  text(ctx, str, M, y, T.label(26, C.soft));
}

/** Darken the left of the frame so the words always read. */
function leftShade(ctx, a = 0.9, to = 1150) {
  const g = ctx.createLinearGradient(0, 0, to, 0);
  g.addColorStop(0, `rgba(0,0,0,${a})`);
  g.addColorStop(0.55, `rgba(0,0,0,${a * 0.6})`);
  g.addColorStop(1, 'rgba(0,0,0,0)');
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, to, H);
}

function vignette(ctx, a = 0.6) {
  const g = ctx.createRadialGradient(W * 0.62, H / 2, H * 0.32, W * 0.62, H / 2, W * 0.78);
  g.addColorStop(0, 'rgba(0,0,0,0)');
  g.addColorStop(1, `rgba(0,0,0,${a})`);
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, W, H);
}

/** The accent's light behind the watch: a wide wash and a tighter core. */
function halo(ctx, col, a = 1) {
  light(ctx, WATCH.cx, WATCH.cy, 1050, rgba(col, 0.2 * a));
  light(ctx, WATCH.cx, WATCH.cy, 540, rgba(col, 0.32 * a));
}

// --- Race ----------------------------------------------------------------------

function race(ctx, v) {
  setWatchModel('graphite');
  halo(ctx, C.cyan, 0.9);
  // Light streaks running left from behind the watch: the race going by.
  for (let i = 0; i < 150; i++) {
    const r1 = hash(i, 5), r2 = hash(i, 6), r3 = hash(i, 7), r4 = hash(i, 8);
    const y = 30 + r1 * (H - 60);
    const len = 160 + r2 * 900;
    const x1 = W - r4 * 620;
    const x0 = x1 - len;
    const a = (0.08 + 0.62 * r3 * r3) * clamp(1.15 - Math.abs(y - WATCH.cy) / 600);
    const col = hash(i, 9) > 0.85 ? C.lemon : C.cyan;
    const g = ctx.createLinearGradient(x0, 0, x1, 0);
    g.addColorStop(0, rgba(col, 0));
    g.addColorStop(0.8, rgba(col, a));
    g.addColorStop(1, rgba(col, 0));
    ctx.fillStyle = g;
    ctx.fillRect(x0, y, len, r2 > 0.85 ? 3 : r2 > 0.5 ? 2 : 1);
  }
  leftShade(ctx);

  // A: the last segment, one more press ends the race. B: the first run.
  const segs = UI.EXAMPLE.segs;
  const face = v === 'a'
    ? { type: 'station', name: 'WALL BALLS', work: '100 reps', idx: 15, count: 16, segSec: 347, totalSec: segs[15].start + 347, next: null, hr: 176, glow: { r2: 1 } }
    : { type: 'run', name: 'RUN 1/8', work: '1 km', idx: 0, count: 16, segSec: 192, totalSec: 192, next: 'SKIERG', hr: 152, glow: { r2: 1 } };
  watch(ctx, { ...WATCH, screen: (s) => UI.raceFace(s, face), bloom: 0.6, press: { r2: 0.7 } });
  const [bx, by] = buttonPos(WATCH, 'r2', 1.34);
  light(ctx, bx, by, 200, rgba(C.amber, 0.55));
  ripple(ctx, bx, by, 0.38, C.amber, { rings: 3, spread: 0.11, speed: 430, life: 0.85, width: 4 });
  vignette(ctx, 0.55);

  lockup(ctx, 'Race', C.cyan, C.lemon);
  if (v === 'b') {
    const { last } = headline(ctx, [{ s: '8 runs.', color: C.cyan }, { s: '8 stations.', color: C.lemon }, { s: '1 button.', color: C.white }], 372);
    tagline(ctx, 'RACE TIMER  ·  FOR UNA WATCH', last + 96);
    return;
  }
  const { size, last } = headline(ctx, [{ s: '16 splits.', color: C.white }, { s: 'One button.', color: C.lemon }]);
  // The race as a strip: eight runs, eight stations.
  const sw = measure(ctx, 'One button.', T.hero(size)), gap = 10;
  const bw = (sw - gap * 15) / 16;
  const sy = last + 68;
  for (let i = 0; i < 16; i++) {
    ctx.fillStyle = i % 2 ? C.lemon : C.cyan;
    ctx.fillRect(M + i * (bw + gap), sy, bw, 16);
  }
  tagline(ctx, 'RACE TIMER  ·  FOR UNA WATCH', sy + 84);
}

// --- Streak --------------------------------------------------------------------

/** A mountain from the app's own scene, k times its size on the watch, foot on baseY. */
function peak(ctx, o, cx, baseY, k) {
  ctx.save();
  ctx.translate(cx - 120 * k, baseY - 118 * k);
  ctx.scale(k, k);
  const r = S.scene(ctx, { stars: false, far: false, ...o });
  ctx.restore();
  return [cx - 120 * k + r.here[0] * k, baseY - 118 * k + r.here[1] * k];
}

function streak(ctx, v) {
  setWatchModel('teal');
  // The night sky.
  for (let i = 0; i < 440; i++) {
    const x = hash(i, 21) * W, y = hash(i, 22) * H * 0.85;
    const r = 0.8 + Math.pow(hash(i, 23), 7) * 3.2;
    ctx.globalAlpha = 0.2 + 0.8 * hash(i, 24);
    ctx.fillStyle = r > 2 ? C.white : '#9aa3ad';
    ctx.fillRect(x, y, r, r);
  }
  ctx.globalAlpha = 1;
  const g = ctx.createRadialGradient(W * 0.35, H * 1.25, 0, W * 0.35, H * 1.25, H * 1.15);
  g.addColorStop(0, rgba(C.teal, 0.34));
  g.addColorStop(1, 'rgba(0,0,0,0)');
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, W, H);
  halo(ctx, v === 'a' ? C.lime : C.shield, v === 'a' ? 0.5 : 0.4);

  // A far range for depth, then the peaks; the lime trail is up the big one,
  // the climber eight weeks from the top.
  ctx.fillStyle = '#0b1a1c';
  ctx.beginPath();
  ctx.moveTo(0, H);
  let x = -40;
  for (let i = 0; x < W + 100; i++) {
    const w = 160 + hash(i, 41) * 240, h = 70 + hash(i, 42) * 150;
    ctx.lineTo(x + w / 2, H - 150 - h);
    ctx.lineTo(x + w, H - 150);
    x += w * 0.72;
  }
  ctx.lineTo(W, H);
  ctx.closePath();
  ctx.fill();
  peak(ctx, { shape: 2, climbed: 0, steps: 14, flag: false }, 120, H + 30, 2.9);
  peak(ctx, { shape: 1, climbed: 0, steps: 14, flag: false }, 1010, H + 40, 2.5);
  const here = peak(ctx, { shape: 4, climbed: 44, steps: 52, flag: true, wave: 1 }, 610, H + 34, 4.1);
  ctx.save();
  ctx.translate(here[0], here[1]);
  ctx.scale(3, 3);
  S.climber(ctx, 0, 0, 2);
  ctx.restore();
  light(ctx, here[0], here[1], 160, rgba(C.lime, 0.5));
  leftShade(ctx, 0.7, 1000);

  // A: confetti spilling from the summit on the watch. B: the shield's offer.
  for (let i = 0; v === 'a' && i < 80; i++) {
    const a = hash(i, 31) * TAU, rr = 390 + hash(i, 32) * 520;
    const cx = WATCH.cx + Math.cos(a) * rr * 1.1, cy = WATCH.cy - 60 + Math.sin(a) * rr * 0.75;
    if (cx < 1060) continue;
    ctx.save();
    ctx.translate(cx, cy);
    ctx.rotate(hash(i, 33) * TAU);
    ctx.globalAlpha = 0.35 + 0.65 * hash(i, 34);
    ctx.fillStyle = S.CONF[i % 4];
    const s = 0.7 + hash(i, 35) * 0.8;
    ctx.fillRect(-7 * s, -13 * s, 14 * s, 26 * s);
    ctx.restore();
  }
  watch(ctx, {
    ...WATCH,
    screen: (s) => (v === 'a' ? S.summitScreen(s, { climb: 4, t: 0.9, next: 'Next: Everest again' }) : S.shieldScreen(s, { saved: false, glow: { r1: 1 } })),
    bloom: 0.65,
  });
  vignette(ctx, 0.5);

  lockup(ctx, 'Streak', C.lime, C.teal);
  const lines = v === 'a'
    ? [{ s: 'Every week', color: C.white }, { s: 'counts.', color: C.lime }]
    : [{ s: 'Life happens.', color: C.white }, { s: 'Keep climbing.', color: C.lime }];
  const { last } = headline(ctx, lines);
  tagline(ctx, 'WEEKLY STREAK  ·  FOR UNA WATCH', last + 84);
}

// --- Trail ---------------------------------------------------------------------

/** A map camera that puts route distance s0 at screen p0 and s1 at p1. */
function fitCam(s0, p0, s1, p1) {
  const a = routeAt(s0), b = routeAt(s1);
  const vx = b.x - a.x, vy = -(b.y - a.y);
  const ux = p1[0] - p0[0], uy = p1[1] - p0[1];
  return {
    x: a.x, y: a.y, sx: p0[0], sy: p0[1],
    k: Math.hypot(ux, uy) / Math.hypot(vx, vy), rot: Math.atan2(uy, ux) - Math.atan2(vy, vx),
  };
}

function trail(ctx, v) {
  setWatchModel('white');
  // The climb's switchbacks run under the words and lead up into the watch.
  const cam = fitCam(1300, [120, 1120], 3800, [1000, 820]);
  contours(ctx, cam, 1);
  halo(ctx, C.pink, 0.75);
  routeLine(ctx, cam, { width: 10, glow: 34 });
  leftShade(ctx, 0.85, 1050);
  // You, 64 m off the line, as the watch says.
  const on = routeAt(3100);
  const [lx, ly] = toScreen(cam, on.x, on.y);
  const off = [lx - 12, ly + 64 * cam.k + 4];
  ctx.save();
  ctx.strokeStyle = rgba(C.amber, 0.9);
  ctx.lineWidth = 3;
  ctx.setLineDash([8, 8]);
  ctx.beginPath();
  ctx.moveTo(off[0], off[1]);
  ctx.lineTo(lx, ly);
  ctx.stroke();
  ctx.restore();
  ripple(ctx, off[0], off[1], 0.42, C.amber, { rings: 3, spread: 0.12, speed: 220, life: 0.9, width: 3 });
  light(ctx, off[0], off[1], 90, rgba(C.amber, 0.6));
  ctx.fillStyle = C.amber;
  ctx.beginPath();
  ctx.arc(off[0], off[1], 12, 0, TAU);
  ctx.fill();

  const map = (s) => TR.mapScreen(s, { at: 4200, radius: 200, scale: '200 m', banner: 'off', bannerP: 1, offBy: 64, toGo: 9.9 });
  if (v === 'b') {
    // The buzz: haptic marks beside the watch.
    ctx.save();
    ctx.strokeStyle = C.amber;
    ctx.lineCap = 'round';
    for (let i = 0; i < 3; i++) {
      const r = 505 + i * 36;
      ctx.lineWidth = 7 - i * 1.5;
      ctx.globalAlpha = 0.9 - i * 0.25;
      ctx.beginPath();
      ctx.arc(WATCH.cx, WATCH.cy, r, Math.PI - 0.17, Math.PI + 0.17);
      ctx.stroke();
    }
    ctx.restore();
  }
  watch(ctx, { ...WATCH, screen: map, bloom: 0.4 });
  vignette(ctx, 0.5);

  lockup(ctx, 'Trail', C.pink, C.pink);
  const lines = v === 'a'
    ? [{ s: 'Follow', color: C.white }, { s: 'the line.', color: C.pink }]
    : [{ s: 'Wander off?', color: C.white }, { s: 'You’ll feel it.', color: C.amber }];
  // B's marks sit beside the words, so its headline is a little narrower.
  const { last } = headline(ctx, lines, HEAD.y, v === 'a' ? HEAD.maxW : 720);
  // "Concept preview", as the title says.
  const py = last + 56;
  const pl = 'CONCEPT PREVIEW';
  const ps = T.label(24, C.pink);
  const pw = measure(ctx, pl, ps) + 56;
  ctx.fillStyle = 'rgba(0,0,0,0.6)';
  roundRect(ctx, M, py, pw, 54, 27);
  ctx.fill();
  ctx.strokeStyle = rgba(C.pink, 0.9);
  ctx.lineWidth = 2.5;
  ctx.stroke();
  text(ctx, pl, M + 28, py + 36, ps);
  tagline(ctx, 'GPX NAVIGATION  ·  FOR UNA WATCH', py + 118);
}

// --- Output --------------------------------------------------------------------

const DESIGNS = {
  race: { draw: race, title: 'HybridX Race — A 16-Segment Run & Station Race Timer for UNA Watch' },
  streak: { draw: streak, title: 'HybridX Streak — A Weekly Workout Streak Tracker for UNA Watch' },
  trail: { draw: trail, title: 'HybridX Trail — Breadcrumb GPX Navigation for UNA Watch (Concept Preview)' },
};
const VARIANTS = ['a', 'b'];

/** Scale the 1920 x 1080 layout down to 1280 x 720 and add a fine grain. */
function finish(big) {
  const c = createCanvas(TW, TH);
  const o = c.getContext('2d');
  o.imageSmoothingEnabled = true;
  o.imageSmoothingQuality = 'high';
  o.drawImage(big, 0, 0, TW, TH);
  // A little grain keeps the dark gradients from banding once YouTube
  // recompresses them.
  const img = o.getImageData(0, 0, TW, TH);
  const d = img.data;
  for (let i = 0, p = 0; i < d.length; i += 4, p++) {
    const n = (hash(p, 97) - 0.5) * 7;
    d[i] += n;
    d[i + 1] += n;
    d[i + 2] += n;
  }
  o.putImageData(img, 0, 0);
  return c;
}

/** A thumbnail as YouTube shows one: scaled, with the duration bottom right. */
function thumb(x, img, x0, y, w, h) {
  x.imageSmoothingQuality = 'high';
  x.drawImage(img, x0, y, w, h);
  const f = Math.max(11, Math.round(h * 0.07));
  const bw = f * 3.2, bh = f * 1.55;
  x.fillStyle = 'rgba(0,0,0,0.8)';
  roundRect(x, x0 + w - bw - 6, y + h - bh - 6, bw, bh, 4);
  x.fill();
  text(x, '2:30', x0 + w - bw / 2 - 6, y + h - 6 - bh * 0.28, { size: f, weight: 500, color: C.white, align: 'center' });
}

/**
 * A review sheet: each film's A and B large, then both at the size of
 * YouTube's sidebar, where a thumbnail has to work hardest.
 */
async function review(done) {
  const ids = [...new Set(done.map((d) => d.id))];
  const pad = 36, colW = 560 + pad;
  const sheet = createCanvas(pad + ids.length * colW, 1000);
  const x = sheet.getContext('2d');
  x.fillStyle = '#0f0f0f';
  x.fillRect(0, 0, sheet.width, sheet.height);
  for (const [i, id] of ids.entries()) {
    const x0 = pad + i * colW;
    const [name, rest] = done.find((d) => d.id === id).title.split(' — ');
    text(x, name, x0, pad + 20, { size: 22, weight: 600, color: C.white });
    text(x, rest, x0, pad + 48, { size: 16, weight: 400, color: '#aaaaaa' });
    let y = pad + 76;
    for (const v of VARIANTS) {
      const d = done.find((e) => e.id === id && e.v === v);
      thumb(x, await loadImage(d.jpg), x0, y, 560, 315);
      text(x, v === 'a' ? 'A  ·  MAIN' : 'B  ·  TO TEST AGAINST A', x0, y + 315 + 26, T.label(13, '#8a8a8a'));
      y += 315 + 48;
    }
    for (const [j, v] of VARIANTS.entries()) {
      const d = done.find((e) => e.id === id && e.v === v);
      thumb(x, await loadImage(d.jpg), x0 + j * (168 + 20), y + 6, 168, 94);
    }
    text(x, 'SIDEBAR SIZE', x0 + 2 * (168 + 20), y + 58, T.label(13, '#8a8a8a'));
  }
  const f = path.join(HERE, 'out', 'thumbnails_review.png');
  fs.mkdirSync(path.dirname(f), { recursive: true });
  fs.writeFileSync(f, await sheet.encode('png'));
  console.log(f);
}

setupFonts();
fs.mkdirSync(OUT, { recursive: true });
const only = process.argv[2];
const done = [];
for (const [id, { draw, title }] of Object.entries(DESIGNS)) {
  if (only && only !== id) continue;
  for (const v of VARIANTS) {
    const big = createCanvas(W, H);
    const ctx = big.getContext('2d');
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, W, H);
    draw(ctx, v);
    const jpg = await finish(big).encode('jpeg', 93);
    const f = path.join(OUT, `hybridx-${id}-thumbnail-${v}.jpg`);
    fs.writeFileSync(f, jpg);
    done.push({ id, v, title, jpg });
    console.log(`${f}  ${(jpg.length / 1024).toFixed(0)} KB`);
  }
}
if (!only) await review(done);
