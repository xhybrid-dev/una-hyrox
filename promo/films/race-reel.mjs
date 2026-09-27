// HybridX Race, the reel: "Sixteen" in 34 seconds, portrait.
//
// 128 BPM, 18 bars (33.75 s), 1080 x 1920 for Instagram Reels and adverts.
// The same grammar as the film, cut to its hooks: the count-in, the sting on
// the gun, five features a two-bar chapter each, the finish, the end card.
// The reel is still a race: the top bar is the sixteen segments, filling as
// the race clock runs from the gun to the finish.

import { clamp, lerp, prog, ep, E, hash, tempo, mss, hmss, TAU } from '../lib/core.mjs';
import { text, kinetic, measure, rgba, light } from '../lib/gfx.mjs';
import { C, T, sting, chevronWipe } from '../lib/brand.mjs';
import { watch, setWatchModel, buttonPos, ripple, buzz } from '../lib/watch.mjs';
import * as UI from '../lib/ui-race.mjs';
import { callout, pressAt } from '../lib/film.mjs';
import { RW as W, RH as H, SAFE, fit, reelHud, reelHead, reelEnd } from '../lib/reel.mjs';

const TM = tempo(128);
const B = (b, beats = 0) => TM.at(b, beats);
const RACE = UI.EXAMPLE;
const SEGS = RACE.segs;
const LOCK = B(2);
const CH = { one: B(4), rox: B(6), hr: B(8), undo: B(10), laps: B(12), finish: B(14), end: B(16) };
const END = CH.end + 0.35; // the end card's own clock starts here
const DURATION = B(18);
const WIPES = [CH.rox, CH.hr, CH.undo, CH.laps];
const PRESSES = [B(5) - 0.12, B(6) - 0.12];

// --- The race clock -----------------------------------------------------------
// Segment k runs from SPLIT[k] to SPLIT[k + 1] in film time: the first two are
// shown live, a bar each; the other fourteen share the next eight bars.
const SPLIT = (() => {
  const s = [CH.one, PRESSES[0], PRESSES[1]];
  for (let k = 1; k <= 14; k++) s.push(lerp(PRESSES[1], CH.finish - 0.12, k / 14));
  return s;
})();

function segAt(t) {
  for (let k = 0; k < 16; k++) if (t < SPLIT[k + 1]) return k;
  return 15;
}

function raceSec(t) {
  if (t < CH.one) return 0;
  if (t >= SPLIT[16]) return RACE.total;
  const k = segAt(t);
  return SEGS[k].start + prog(t, SPLIT[k], SPLIT[k + 1]) * SEGS[k].sec;
}

function faceState(t, over = {}) {
  const k = segAt(t);
  const s = SEGS[k];
  const rs = raceSec(t);
  return {
    type: s.type, name: s.name, work: s.work, idx: k, count: 16, segSec: Math.max(0, rs - s.start), totalSec: rs,
    next: k < 15 ? SEGS[k + 1].name : null, hr: s.hr + Math.round(3 * Math.sin(t * 1.3)), ...over,
  };
}

// --- Scenes ---------------------------------------------------------------------

/** Speed lines drifting left, behind everything. */
function speedLines(ctx, t, color, alpha = 1, seed = 1) {
  ctx.save();
  for (let i = 0; i < 60; i++) {
    const r1 = hash(i, seed), r2 = hash(i + 101, seed), r3 = hash(i + 203, seed), r4 = hash(i + 307, seed);
    const len = 60 + r2 * 360;
    const v = 700 + r3 * 2600;
    const y = r1 * H;
    const x = W + len - ((r4 * (W + len) + t * v) % (W + len * 2));
    const g = ctx.createLinearGradient(x, 0, x + len, 0);
    g.addColorStop(0, rgba(color, (0.06 + 0.4 * r3 * r3) * alpha));
    g.addColorStop(1, rgba(color, 0));
    ctx.fillStyle = g;
    ctx.fillRect(x, y, len, r2 > 0.8 ? 2 : 1);
  }
  ctx.restore();
}

/** 0 to 2.8 s: eight runs, eight stations, one button. */
function countIn(ctx, t) {
  const beat = TM.beat;
  const words = [
    { s: '8 runs.', c: C.cyan, at: 0 },
    { s: '8 stations.', c: C.lemon, at: beat * 2 },
    { s: '1 button.', c: C.white, at: beat * 4 },
  ];
  const y = 930;
  words.forEach((w, i) => {
    const p = prog(t, w.at - 0.25, w.at + 0.12);
    const q = i < 2 ? prog(t, words[i + 1].at - 0.14, words[i + 1].at) : prog(t, LOCK - 1.25, LOCK - 0.95);
    kinetic(ctx, w.s, W / 2, y, fit(ctx, w.s, { ...T.hero(190, w.c), align: 'center' }, W - 2 * SAFE.left), p, q, 'rise');
  });
  // The strip under the words: runs, then stations, then all sixteen fold
  // into one amber block, the R2 hint.
  const bw = 48, gap = 8, sx = W / 2 - (16 * (bw + gap) - gap) / 2, sy = y + 110;
  const pr = prog(t, -0.2, 0.7), ps = prog(t, beat * 2, beat * 2 + 0.7);
  const fold = ep(t, beat * 4 + 0.1, beat * 4 + 0.55, E.inOutQuart);
  const gone = prog(t, LOCK - 1.25, LOCK - 0.95);
  const press = beat * 6;
  ctx.save();
  ctx.globalAlpha *= 1 - gone;
  for (let i = 0; i < 16; i++) {
    const s = SEGS[i];
    const p = s.type === 'run' ? clamp(pr * 8 - i / 2) : clamp(ps * 8 - (i - 1) / 2);
    if (p <= 0) continue;
    const x = lerp(sx + i * (bw + gap), W / 2 - bw / 2, fold);
    ctx.globalAlpha = (1 - gone) * E.outCubic(p);
    ctx.fillStyle = fold > 0.5 ? C.amber : s.type === 'run' ? C.cyan : C.lemon;
    const pop = t > press ? 1 + 0.5 * Math.exp(-(t - press) * 8) : 1;
    ctx.fillRect(x - (bw * (pop - 1)) / 2, sy + (1 - E.outBack(p)) * 30, bw * pop, 12);
  }
  ctx.restore();
  if (t > press) ripple(ctx, W / 2, sy + 6, t - press, C.amber, { rings: 3, speed: 600, life: 0.6 });
  kinetic(ctx, 'R2 · SPLIT', W / 2, sy + 80, { ...T.label(24, C.amber), align: 'center' }, prog(t, beat * 4 + 0.4, beat * 4 + 0.8), gone, 'scramble', { t });
}

/** The sting on the gun, and the start line. */
function title(ctx, t) {
  const out = prog(t, B(3, 2), B(4) - 0.1);
  sting(ctx, t - (LOCK - 0.95), { accent: C.cyan, accent2: C.lemon, name: 'Race', sub: 'A RACE TIMER FOR UNA WATCH', out, h: 164, cy: 900 });
  const lt = t - LOCK;
  if (lt > 0) {
    const e = E.outExpo(clamp(lt / 0.9));
    ctx.save();
    ctx.globalAlpha *= 1 - out;
    ctx.fillStyle = C.cyan;
    ctx.fillRect(W / 2 - (W / 2) * e, 1060, (W / 2) * e, 3);
    ctx.fillStyle = C.lemon;
    ctx.fillRect(W / 2, 1060, (W / 2) * e, 3);
    ctx.restore();
    speedLines(ctx, t, C.cyan, clamp(lt / 0.6) * (1 - out) * 0.8, 3);
  }
}

/** One button: the race face, two live splits. */
function oneButton(ctx, t) {
  const lt = t - CH.one;
  speedLines(ctx, t, C.cyan, 0.8, 1);
  reelHead(ctx, lt, 'One button.', 'R2 ends every run and every station. Sixteen presses, sixteen splits.', SAFE.left, 400, { p0: 0.15, size: 132 });
  const wIn = ep(lt, 0.05, 0.8, E.outExpo);
  let bz = [0, 0];
  for (const pt of PRESSES) {
    const b = buzz(t - pt, 7);
    bz = [bz[0] + b[0], bz[1] + b[1]];
  }
  const wo = { cx: W / 2 + bz[0], cy: lerp(H + 500, 1100, wIn) + bz[1], d: 540, rot: lerp(0.35, 0, wIn) };
  const last = PRESSES.filter((p) => t >= p).pop();
  const since = last !== undefined ? t - last : 99;
  const k = segAt(t - since - 0.001);
  watch(ctx, {
    ...wo,
    screen: (s) => (since < 0.9 ? UI.splitToast(s, { name: SEGS[k].name, sec: SEGS[k].sec }) : UI.raceFace(s, faceState(t, { glow: { r2: 0.5 + 0.5 * Math.sin(lt * TAU * 1.07) } }))),
    bloom: 0.55, press: { r2: PRESSES.reduce((a, p) => a + pressAt(t - p), 0) },
  });
  const bp = buttonPos(wo, 'r2', 1.34);
  callout(ctx, bp, [SAFE.left + 460, 1470], 'R2 · SPLIT', prog(lt, 0.9, 1.5));
  for (const pt of PRESSES) if (t > pt) ripple(ctx, bp[0], bp[1], t - pt, C.amber, { rings: 3, speed: 520, life: 0.8 });
}

/** Roxzone: the toggle, and sixteen segments becoming thirty-one. */
function roxzone(ctx, t) {
  const lt = t - CH.rox;
  reelHead(ctx, lt, 'Roxzone, split.', 'Turn it on and every station gets a lap in and a lap out.', SAFE.left, 400, { p0: 0.2, size: 124 });
  const wIn = ep(lt, 0.1, 0.9, E.outExpo);
  const tog = ep(lt, 1.0, 1.4, E.inOutCubic);
  watch(ctx, {
    cx: W / 2, cy: lerp(H + 400, 960, wIn), d: 400, rot: lerp(0.3, 0, wIn),
    screen: (s) => (lt < 2.5 ? UI.settingsToggle(s, { on: tog }) : UI.raceFace(s, { ...faceState(t), type: 'rox', name: 'ROXZONE IN', work: '', idx: 13, count: 31, next: 'BURPEE BROAD JUMPS' })),
    bloom: 0.5, press: { r1: pressAt(t - (CH.rox + 0.95)) },
  });
  // The strip, reflowing from 16 to 31 blocks.
  const on = ep(lt, 1.4, 2.7, E.inOutQuart);
  const x0 = SAFE.left, x1 = W - SAFE.left, y = 1290, h = 60;
  const blocks = [];
  SEGS.forEach((s, i) => {
    if (s.type === 'run') blocks.push({ col: C.cyan, w: 1 });
    else {
      blocks.push({ col: C.orchid, w: on, rox: true });
      blocks.push({ col: C.lemon, w: 1 });
      if (i < 15) blocks.push({ col: C.orchid, w: on, rox: true });
    }
  });
  const gap = 5;
  const nVis = blocks.reduce((a, b) => a + b.w, 0);
  const unit = (x1 - x0 - gap * (nVis - 1)) / nVis;
  const inP = ep(lt, 0.5, 1.3, E.outExpo);
  let x = x0;
  ctx.save();
  blocks.forEach((b, i) => {
    if (b.w <= 0.001) return;
    const bw = unit * b.w;
    const pop = b.rox ? E.outBack(clamp(on * 1.4 - (i / blocks.length) * 0.4)) : 1;
    const hh = h * (b.rox ? 0.72 : 1) * pop;
    ctx.globalAlpha = clamp(inP * 1.5 - i / blocks.length);
    ctx.fillStyle = b.col;
    ctx.fillRect(x, y + (h - hh) / 2, bw, hh);
    x += bw + gap * b.w;
  });
  ctx.restore();
  const n = Math.round(lerp(16, 31, on));
  const numS = T.num(170, on > 0.5 ? C.orchid : C.white);
  kinetic(ctx, String(n), SAFE.left - 8, 1510, numS, prog(lt, 0.6, 1.0), 0, 'rise');
  kinetic(ctx, 'SEGMENTS', SAFE.left + 236, 1506, T.label(24, C.soft), prog(lt, 0.7, 1.1), 0, 'scramble', { t: lt });
}

/** Heart rate: the number, the beat, the zones, every segment's average. */
function heart(ctx, t) {
  const lt = t - CH.hr;
  const cols = [C.gray, C.chartreuse, C.yellow, C.amber, C.red];
  reelHead(ctx, lt, ['Heart rate,', 'every segment.'], null, SAFE.left, 400, { p0: 0.2, size: 124 });
  // The trace, pulsing on the beat.
  const trP = ep(lt, 0.2, 1.0, E.inOutCubic);
  ctx.save();
  ctx.globalAlpha *= trP;
  ctx.strokeStyle = rgba(C.red, 0.9);
  ctx.lineWidth = 4;
  ctx.lineJoin = 'round';
  ctx.beginPath();
  const base = 800;
  for (let x = 0; x <= W; x += 5) {
    const tt = lt - (W - x) / 700;
    const ph = (((tt / TM.beat) % 1) + 1) % 1;
    let yv = 0;
    if (ph < 0.08) yv = -Math.sin((ph / 0.08) * Math.PI) * 120;
    else if (ph < 0.14) yv = Math.sin(((ph - 0.08) / 0.06) * Math.PI) * 44;
    else if (ph > 0.4 && ph < 0.52) yv = -Math.sin(((ph - 0.4) / 0.12) * Math.PI) * 20;
    const yy = base + yv * clamp(x / 200) * clamp((W - x) / 200);
    if (x === 0) ctx.moveTo(x, yy);
    else ctx.lineTo(x, yy);
  }
  ctx.stroke();
  ctx.restore();
  // The zone arc, as on the race face, drawn large.
  const bpm = Math.round(lerp(142, 176, ep(lt, 0.6, 3.4, E.inOutSine)) + 2 * Math.sin(lt * 5));
  const zone = UI.zoneFor(bpm);
  const arcIn = ep(lt, 0.4, 1.4, E.inOutQuart);
  const R = 470, cx = W / 2, cy = 1540;
  for (let i = 0; i < 5; i++) {
    const a0 = -64 + i * 26, a1 = a0 + 24;
    const aa0 = ((a0 - 90) * Math.PI) / 180, aa1 = ((lerp(a0, a1, clamp(arcIn * 5 - i)) - 90) * Math.PI) / 180;
    if (aa1 <= aa0) continue;
    ctx.save();
    ctx.strokeStyle = cols[i];
    ctx.lineWidth = i === zone ? 30 : 12;
    ctx.globalAlpha *= i === zone ? 1 : 0.45;
    ctx.beginPath();
    ctx.arc(cx, cy, R, aa0, aa1);
    ctx.stroke();
    ctx.restore();
  }
  const numS = { ...T.num(230, C.white), align: 'center' };
  kinetic(ctx, String(bpm), cx - 30, 1330, numS, prog(lt, 0.5, 0.9), 0, 'rise');
  kinetic(ctx, 'BPM', cx + 140, 1330, T.label(28, zone >= 0 ? cols[zone] : C.soft), prog(lt, 0.7, 1.1), 0, 'scramble', { t: lt });
  // Every segment's average and maximum.
  const bp = prog(lt, 1.6, 2.6);
  for (let i = 0; i < 16; i++) {
    const s = SEGS[i];
    const pp = clamp(bp * 16 - i);
    if (pp <= 0) continue;
    const avg = s.hr + 2, mx = s.hr + 14;
    const bw = 34, x = W / 2 - (16 * 46 - 12) / 2 + i * 46;
    const hA = (avg - 130) * 2.4 * E.outCubic(pp), hM = (mx - 130) * 2.4 * E.outCubic(pp);
    ctx.fillStyle = cols[Math.max(0, UI.zoneFor(avg))];
    ctx.fillRect(x, 1500 - hA, bw, hA);
    ctx.fillStyle = C.white;
    ctx.fillRect(x, 1500 - hM - 6, bw, 3);
  }
  kinetic(ctx, 'AVG / MAX · 16 SEGMENTS', W / 2, 1400, { ...T.label(18, C.mute), align: 'center' }, prog(lt, 2.4, 2.9), 0, 'fade');
}

/** Undo: an early press cuts the pull in two; undo merges it back. */
function undo(ctx, t) {
  const lt = t - CH.undo;
  reelHead(ctx, lt, 'Split too early?', 'Undo it. The times merge back, exactly.', SAFE.left, 400, { p0: 0.2, size: 124 });
  const cutAt = 0.9, undoAt = 2.65;
  const wIn = ep(lt, 0.1, 0.9, E.outExpo);
  const sel = lt > 1.95 ? 1 : 0;
  const shift = lt > 1.7 && lt < 1.95 ? ep(lt, 1.7, 1.95, E.inOutCubic) : 0;
  const k5 = { ...faceState(t), type: 'station', name: 'SLED PULL', work: '50 m', idx: 5, next: 'RUN 4/8', segSec: 76 + lt * 9, totalSec: SEGS[5].start + 76 + lt * 9 };
  watch(ctx, {
    cx: W / 2, cy: lerp(H + 400, 940, wIn), d: 420, rot: lerp(0.3, 0, wIn),
    screen: (s) => {
      if (lt < 1.3 || lt >= undoAt + 0.1) UI.raceFace(s, k5);
      else UI.actionMenu(s, { items: [{ label: 'Resume' }, { label: 'Undo last', tip: 'split' }, { label: 'Pause' }, { label: 'End race' }], sel, shift });
    },
    bloom: 0.5, press: { r2: pressAt(t - (CH.undo + cutAt)), r1: pressAt(t - (CH.undo + undoAt - 0.1)) },
  });
  // The bar: cut in two, then merged.
  const bx = SAFE.left, by = 1320, bw = W - 2 * SAFE.left, bh = 76;
  const inP = ep(lt, 0.3, 1.0, E.outExpo);
  const cut = 0.44;
  const split = ep(lt, cutAt, cutAt + 0.3, E.outBack) * (1 - ep(lt, undoAt + 0.15, undoAt + 0.6, E.inOutCubic));
  const sep = 16 * split;
  ctx.fillStyle = C.lemon;
  ctx.fillRect(bx, by, bw * Math.min(cut, inP), bh);
  if (inP > cut) ctx.fillRect(bx + bw * cut + sep, by, bw * (inP - cut), bh);
  const full = SEGS[5].sec, early = Math.round(full * cut);
  const inSplit = lt > cutAt && lt < undoAt + 0.4;
  if (inP > 0.5) {
    text(ctx, inSplit ? `SLED PULL  ${mss(early)}` : `SLED PULL  ${mss(full)}`, bx + 22, by + 48, T.label(22, '#000'));
    if (inSplit) text(ctx, `RUN 4/8  ${mss(full - early)}`, bx + bw * cut + sep + 22, by + 48, { ...T.label(22, '#000'), alpha: split });
  }
  if (lt > undoAt) {
    const u = prog(lt, undoAt, undoAt + 0.6);
    text(ctx, 'UNDO', bx + bw * cut + 7, by - 30, { ...T.label(22, C.amber), align: 'center', alpha: 1 - prog(lt, undoAt + 0.8, undoAt + 1.1) });
    ctx.fillStyle = rgba(C.amber, 1 - u);
    ctx.fillRect(bx + bw * cut + 6, by - 14 + u * 20, 3, bh + 28 - u * 40);
    text(ctx, 'MERGED · TOTAL UNCHANGED', bx, by + bh + 60, { ...T.label(22, C.soft), alpha: prog(lt, undoAt + 0.5, undoAt + 0.9) });
  } else if (lt > cutAt) {
    text(ctx, 'TOO EARLY', bx + bw * cut + 7, by - 30, { ...T.label(22, C.gray), align: 'center', alpha: prog(lt, cutAt, cutAt + 0.3) });
  }
}

/** Laps: the FIT file's sixteen labelled laps. */
function laps(ctx, t) {
  const lt = t - CH.laps;
  reelHead(ctx, lt, 'Every split, a lap.', null, SAFE.left, 400, { p0: 0.2, size: 118 });
  const tx = SAFE.left + 12, ty = 560, tw = W - 2 * SAFE.left - 24, rh = 50;
  const head = prog(lt, 0.3, 0.8);
  kinetic(ctx, 'HybridXRace.fit', tx, ty, T.mono(28, C.white), head, 0, 'type', { t: lt });
  const hs = T.label(16, C.mute);
  text(ctx, 'LAP', tx, ty + 50, { ...hs, alpha: head });
  text(ctx, 'NAME', tx + 80, ty + 50, { ...hs, alpha: head });
  text(ctx, 'TIME', tx + tw - 140, ty + 50, { ...hs, alpha: head, align: 'right' });
  text(ctx, 'HR', tx + tw, ty + 50, { ...hs, alpha: head, align: 'right' });
  for (let i = 0; i < 16; i++) {
    const s = SEGS[i];
    const pi = prog(lt, 0.5 + i * 0.07, 0.95 + i * 0.07);
    if (pi <= 0) continue;
    const e = E.outExpo(pi);
    const y = ty + 94 + i * rh;
    const x = lerp(tx - 260, tx, e);
    ctx.save();
    ctx.globalAlpha *= e;
    ctx.fillStyle = i % 2 ? '#0c0e10' : '#111417';
    ctx.fillRect(x - 12, y - 33, tw + 24, rh - 4);
    ctx.fillStyle = s.type === 'run' ? C.cyan : C.lemon;
    ctx.fillRect(x - 12, y - 33, 5, rh - 4);
    const ms = T.mono(24, C.white);
    text(ctx, String(i + 1).padStart(2, '0'), x, y, { ...ms, color: C.mute });
    text(ctx, s.name, x + 80, y, fit(ctx, s.name, ms, tw - 330));
    text(ctx, mss(s.sec), x + tw - 140, y, { ...ms, align: 'right' });
    text(ctx, `${s.hr + 2}`, x + tw, y, { ...ms, color: C.soft, align: 'right' });
    ctx.restore();
  }
  kinetic(ctx, '→ STRAVA · GARMIN CONNECT', SAFE.left, 1500, T.label(26, C.amber), prog(lt, 1.9, 2.5), 0, 'scramble', { t: lt });
}

/** The finish: the last press, the flash, the race time. */
function finish(ctx, t) {
  const lt = t - CH.finish;
  const saveT = 1.75, sumT = 2.9;
  kinetic(ctx, 'RACE TIME', SAFE.left, 380, T.label(26, C.soft), prog(lt, 0.2, 0.7), 0, 'scramble', { t: lt });
  kinetic(ctx, hmss(RACE.total), SAFE.left - 10, 580, fit(ctx, hmss(RACE.total), T.num(250, C.white), W - 2 * SAFE.left), prog(lt, 0.05, 0.5), 0, 'rise');
  [['RUNS', hmss(RACE.runs), C.cyan], ['STATIONS', hmss(RACE.stations), C.lemon]].forEach(([k, v, col], i) => {
    const p = prog(lt, 0.6 + i * 0.12, 1.1 + i * 0.12);
    kinetic(ctx, k, SAFE.left + i * 420, 660, T.label(20, C.mute), p, 0, 'fade');
    kinetic(ctx, v, SAFE.left - 2 + i * 420, 720, T.mono(44, col), p, 0, 'rise');
  });
  const wIn = ep(lt, 0.0, 0.7, E.outExpo);
  watch(ctx, {
    cx: W / 2, cy: lerp(1300, 1140, wIn), d: 460, rot: 0,
    screen: (s) => {
      if (lt < saveT) UI.finished(s, { sec: RACE.total, glow: { r1: 0.5 + 0.5 * Math.sin(lt * 6) } });
      else if (lt < sumT) UI.saved(s, { p: prog(lt, saveT, saveT + 0.8) });
      else UI.summary(s, { page: 0 });
    },
    bloom: 0.55, press: { r1: pressAt(t - (CH.finish + saveT - 0.1)) },
  });
  const fl = Math.exp(-Math.max(0, lt) * 5);
  if (fl > 0.01) {
    ctx.save();
    ctx.globalAlpha = 0.85 * fl;
    ctx.fillStyle = C.white;
    ctx.fillRect(0, 0, W, H);
    ctx.restore();
  }
}

// --- The reel ----------------------------------------------------------------------

const SCENES = [
  [0, LOCK - 0.95, countIn], [LOCK - 0.95, CH.one, title], [CH.one, CH.rox, oneButton], [CH.rox, CH.hr, roxzone],
  [CH.hr, CH.undo, heart], [CH.undo, CH.laps, undo], [CH.laps, CH.finish, laps], [CH.finish, CH.end, finish],
];

function sceneAt(t) {
  for (const [a, b, f] of SCENES) if (t >= a && t < b) return f;
  return null;
}

/** A split: a tint of the next segment's colour and a hairline across. */
function splitFlash(ctx, t) {
  for (const tb of [...PRESSES, CH.finish - 0.12]) {
    const dt = t - tb;
    if (dt < -0.05 || dt > 0.45) continue;
    ctx.save();
    ctx.globalAlpha = 0.08 * Math.exp(-Math.max(0, dt) * 12);
    ctx.fillStyle = C.lemon;
    ctx.fillRect(0, 0, W, H);
    ctx.restore();
    ctx.fillStyle = rgba(C.white, 0.8 * (1 - clamp(dt / 0.45)));
    ctx.fillRect(E.outExpo(clamp(dt / 0.4)) * W - 2, 0, 2, H);
  }
}

function draw(ctx, t) {
  setWatchModel('graphite');
  ctx.save();
  // A small push on the hard cuts: the gun's chapter and the finish.
  for (const tc of [CH.one, CH.finish]) {
    const kt = t - tc;
    if (kt >= 0 && kt < 0.4) {
      const k = 1 + 0.035 * (1 - E.outCubic(kt / 0.4));
      ctx.translate(W / 2, H / 2);
      ctx.scale(k, k);
      ctx.translate(-W / 2, -H / 2);
    }
  }
  let wiped = false;
  for (const tb of WIPES) {
    const p = prog(t, tb - 0.3, tb + 0.3);
    if (p > 0 && p < 1) {
      sceneAt(tb - 0.001)(ctx, Math.min(t, tb - 0.001));
      const next = sceneAt(tb + 0.001);
      chevronWipe(ctx, p, (c) => {
        c.fillStyle = '#000';
        c.fillRect(0, 0, W, H);
        next(c, t);
      }, { dir: 'right', color: C.lemon, band: 64, band2: 12, color2: C.white });
      wiped = true;
      break;
    }
  }
  if (!wiped) {
    const f = sceneAt(t);
    if (f) f(ctx, t);
  }
  ctx.restore();
  if (t >= CH.end) {
    ctx.save();
    ctx.globalAlpha = 1 - prog(t, DURATION - 0.7, DURATION - 0.05);
    reelEnd(ctx, t - CH.end, { app: 'race', accent: C.cyan, accent2: C.lemon, name: 'Race', tagline: ['Sixteen splits.', 'One button.'], tagColor: C.lemon });
    ctx.restore();
  }
  splitFlash(ctx, t);
  // The top bar: from the first frame, out for the sting, back at the gun's chapter.
  const a = (1 - prog(t, LOCK - 1.15, LOCK - 0.9)) + prog(t, CH.one, CH.one + 0.5) - prog(t, CH.end - 0.5, CH.end);
  const k = segAt(t);
  reelHud(ctx, t, {
    app: 'RACE', accent: C.lemon, alpha: clamp(a),
    right: t < CH.one ? '' : t >= SPLIT[16] ? `FINISHED · ${hmss(RACE.total)}` : `${SEGS[k].short} · ${hmss(raceSec(t))}`,
    rightColor: t >= SPLIT[16] ? C.white : UI.accentFor(SEGS[k].type),
    progress: raceSec(t) / RACE.total,
    ticks: SEGS.slice(0, 15).map((s) => s.end / RACE.total),
    tickColors: SEGS.map((s) => UI.accentFor(s.type)),
  });
}

// --- Cues for the soundtrack ----------------------------------------------------------

function cues() {
  const q = [];
  q.push({ t: 0, type: 'hit', v: 0.9 }, { t: B(0, 2), type: 'hit', v: 0.95 }, { t: B(1), type: 'hit', v: 1.0 });
  q.push({ t: B(1, 2), type: 'press' });
  q.push({ t: B(1, 2) + 0.2, type: 'riser', until: LOCK - 0.02 });
  q.push({ t: LOCK - 0.95, type: 'draw' }, { t: LOCK, type: 'lock' }, { t: LOCK, type: 'drop' });
  for (const p of PRESSES) q.push({ t: p, type: 'split', station: true });
  for (const tb of WIPES) q.push({ t: tb - 0.3, type: 'whoosh' });
  q.push({ t: CH.rox + 0.95, type: 'toggle' }, { t: CH.rox + 1.4, type: 'swell', until: CH.rox + 2.7 });
  q.push({ t: CH.undo + 0.9, type: 'click' }, { t: CH.undo + 2.55, type: 'undo' });
  for (let i = 0; i < 16; i++) q.push({ t: CH.laps + 0.5 + i * 0.07, type: 'tick', v: 0.5 });
  q.push({ t: CH.finish - 0.12, type: 'finishPress' }, { t: CH.finish, type: 'finish' }, { t: CH.finish + 1.65, type: 'saved' });
  q.push({ t: END + 0.9, type: 'logo' });
  return q.sort((a, b) => a.t - b.t);
}

export default {
  id: 'race-reel',
  title: 'HybridX Race (reel)',
  frame: [W, H],
  duration: DURATION,
  bpm: 128,
  sections: [{ name: 'countin', bar: 0 }, { name: 'drop', bar: 2 }, { name: 'groove', bar: 4 }, { name: 'finish', bar: 14 }, { name: 'end', bar: 16 }],
  draw,
  cues,
};
