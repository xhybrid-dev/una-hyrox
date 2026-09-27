// HybridX Race: "Sixteen".
//
// The film is itself a race. At 128 BPM it runs 80 bars (150 s): a cold
// open, the sting (the gun), then sixteen chapters, one per segment. Runs are
// short cyan interludes; each station is a lemon chapter that shows one
// feature, and ends the way a station does: with a press of R2.

import { W, H, clamp, lerp, prog, ep, E, hash, spring, tempo, mss, hmss, window4, TAU } from '../lib/core.mjs';
import { text, kinetic, measure, poly, rgba, light, group, roundRect, strokePart } from '../lib/gfx.mjs';
import { C, T, xMark, sting, hud, chevronWipe, endCard } from '../lib/brand.mjs';
import { watch, buttonPos, ripple, buzz, screenToFrame } from '../lib/watch.mjs';
import * as UI from '../lib/ui-race.mjs';
import { headline, wrap, callout, pressAt } from '../lib/film.mjs';

const TM = tempo(128);
const B = (b, beats = 0) => TM.at(b, beats);
const RACE = UI.EXAMPLE;
const SEGS = RACE.segs;

// Chapters: when each segment is on screen. Run 1 starts at the gun (bar 8).
const BOUNDS = [8, 14, 20, 22, 27, 29, 34, 36, 42, 44, 50, 52, 57, 59, 64, 66, 72];
const CH = SEGS.map((s, k) => ({ k, t0: B(BOUNDS[k]), t1: B(BOUNDS[k + 1]), seg: s }));
const GUN = B(8);
const FINISH = B(72);
const END = B(76);
const DURATION = B(80);

const RUN_TAGS = [
  'Every segment, timed.',
  'Readable at arm’s length.',
  'Built for sweaty hands.',
  'Indoors first. No GPS needed.',
  'Halfway. Keep moving.',
  'Stamped at the press.',
  'App closed? Still timing.',
  'Last run. Empty the tank.',
];

// --- Race time --------------------------------------------------------------

function chapterAt(t) {
  for (const c of CH) if (t >= c.t0 && t < c.t1) return c;
  return t < GUN ? null : CH[CH.length - 1];
}

/** Race seconds at film time t: a time-lapse, segment by segment. */
function raceSec(t) {
  if (t < GUN) return 0;
  if (t >= FINISH) return RACE.total;
  const c = chapterAt(t);
  return c.seg.start + prog(t, c.t0, c.t1) * c.seg.sec;
}

/** The race face's state at film time t. */
function faceState(t, over = {}) {
  const c = chapterAt(t) || CH[0];
  const k = c.k;
  const s = SEGS[k];
  const rs = raceSec(t);
  const wob = Math.round(3 * Math.sin(t * 1.3) + 2 * Math.sin(t * 3.1));
  return {
    type: s.type, name: s.name, work: s.work, idx: k, count: 16,
    segSec: Math.max(0, rs - s.start), totalSec: rs,
    next: k < 15 ? SEGS[k + 1].name : null,
    hr: s.hr + wob, ...over,
  };
}

// --- Shared pieces ------------------------------------------------------------

/** Horizontal speed lines, drifting left. */
function speedLines(ctx, t, color, { n = 70, alpha = 1, speed = 1, y0 = 0, y1 = H, seed = 1 } = {}) {
  ctx.save();
  for (let i = 0; i < n; i++) {
    const r1 = hash(i, seed), r2 = hash(i + 101, seed), r3 = hash(i + 203, seed), r4 = hash(i + 307, seed);
    const len = 60 + r2 * 420;
    const v = (900 + r3 * 3200) * speed;
    const y = lerp(y0, y1, r1);
    const x = W + len - (((r4 * (W + len) + t * v) % (W + len * 2)));
    const a = (0.08 + 0.5 * Math.pow(r3, 2)) * alpha;
    const g = ctx.createLinearGradient(x, 0, x + len, 0);
    g.addColorStop(0, rgba(color, a));
    g.addColorStop(1, rgba(color, 0));
    ctx.fillStyle = g;
    ctx.fillRect(x, y, len, r2 > 0.8 ? 2 : 1);
  }
  ctx.restore();
}

/** The sixteen segments as a strip of blocks. */
function strip(ctx, x, y, w, h, opts = {}) {
  const gap = opts.gap ?? 6;
  const n = 16;
  const bw = (w - gap * (n - 1)) / n;
  for (let i = 0; i < n; i++) {
    const s = SEGS[i];
    const p = opts.reveal ? clamp(opts.reveal * 16 - i) : 1;
    if (p <= 0) continue;
    const lit = opts.upto === undefined || i <= opts.upto;
    const col = s.type === 'run' ? C.cyan : C.lemon;
    ctx.save();
    ctx.globalAlpha *= p * (lit ? 1 : 0.18);
    ctx.fillStyle = col;
    const hh = h * (opts.only && opts.only !== s.type ? 0.35 : 1);
    ctx.fillRect(x + i * (bw + gap), y + (h - hh) / 2 + (1 - E.outBack(p)) * 20, bw, hh);
    ctx.restore();
  }
}

/** A station's chapter card: the big italic name and its number. */
function stationCard(ctx, lt, c, { x = 150, y = 360, out = 1.6 } = {}) {
  const s = c.seg;
  const p = prog(lt, -0.22, 0.32);
  const q = prog(lt, out, out + 0.45);
  if (p <= 0 || q >= 1) return;
  kinetic(ctx, `${String(c.k + 1).padStart(2, '0')} / 16`, x + 4, y - 150, T.label(22, C.soft), p, q, 'scramble', { t: lt });
  kinetic(ctx, s.name, x, y, { size: s.name.length > 12 ? 118 : 168, weight: 600, italic: true, tracking: -0.02, color: C.lemon }, p, q, 'rise');
  kinetic(ctx, s.work, x + 6, y + 70, T.mono(30, C.white), prog(lt, 0.25, 0.7), q, 'type', { t: lt });
}

// --- Scenes -------------------------------------------------------------------

/** Cold open, part 1: out of the amber hint to "On your marks". */
function coldOpen(ctx, t) {
  // Pull back from a macro of the R2 hint to the whole screen.
  const pull = ep(t, 0.2, 5.4, E.inOutCubic);
  const d = lerp(5200, 620, pull);
  // Aim at the amber R1 hint and its tick (240-space ~205, 68) early, the centre later.
  const hx = 205, hy = 70;
  const aim = E.inOutQuad(pull);
  const s = d / 240;
  const cx = W / 2 - (hx - 120) * s * (1 - aim);
  const cy = H / 2 - (hy - 120) * s * (1 - aim);
  const on = prog(t, 0.0, 0.9);
  const glowR2 = 0.6 + 0.4 * Math.sin(t * TAU * (128 / 60) / 2);
  const out = prog(t, 6.9, 7.5);
  ctx.save();
  ctx.globalAlpha = 1 - E.inCubic(out);
  watch(ctx, {
    cx, cy, d, rot: lerp(-0.18, 0, pull), on,
    screen: (c) => UI.onYourMarks(c, { glow: { r2: 0, r1: glowR2 } }),
    bloom: 0.55, strap: pull > 0.5,
  });
  ctx.restore();
  // A mono line, lower left.
  kinetic(ctx, 'FULL RACE · 16 SEGMENTS · ROXZONE MERGED', 120, H - 120, T.label(18, C.soft), prog(t, 3.0, 4.2), prog(t, 6.6, 7.2), 'scramble', { t });
  kinetic(ctx, 'R1 TO START', 120, H - 90, T.label(18, C.amber), prog(t, 3.6, 4.6), prog(t, 6.6, 7.2), 'scramble', { t: t + 2 });
}

/** Cold open, part 2: eight runs, eight stations, one button. */
function countIn(ctx, t) {
  const lt = t - B(4);
  const bar = TM.bar;
  const words = [
    { s: '8 runs.', c: C.cyan, at: 0 },
    { s: '8 stations.', c: C.lemon, at: bar },
    { s: '1 button.', c: C.white, at: bar * 2 },
  ];
  const y = H / 2 + 40;
  words.forEach((w, i) => {
    const p = prog(lt, w.at, w.at + 0.35);
    const q = i < 2 ? prog(lt, w.at + bar - 0.18, w.at + bar) : prog(lt, bar * 3 - 0.7, bar * 3 - 0.35);
    kinetic(ctx, w.s, W / 2, y, { ...T.hero(210, w.c), align: 'center' }, p, q, 'rise');
  });
  // The strip builds under the words: runs, then stations between them.
  const sx = W / 2 - 560, sy = y + 90;
  const pr = prog(lt, 0.1, 0.9);
  const ps = prog(lt, bar + 0.1, bar + 0.9);
  const collapse = ep(lt, bar * 2, bar * 2 + 0.5, E.inOutQuart);
  const gone = prog(lt, bar * 3 - 0.8, bar * 3 - 0.4);
  if (gone < 1) {
    ctx.save();
    ctx.globalAlpha *= 1 - gone;
    const bw = 62, gap = 8;
    for (let i = 0; i < 16; i++) {
      const s = SEGS[i];
      const p = s.type === 'run' ? clamp(pr * 8 - i / 2) : clamp(ps * 8 - (i - 1) / 2);
      if (p <= 0) continue;
      const x0 = sx + i * (bw + gap);
      // Collapse every block into the amber R2 button mark in the middle.
      const x = lerp(x0, W / 2 - bw / 2, collapse);
      const col = collapse > 0.5 ? C.amber : s.type === 'run' ? C.cyan : C.lemon;
      ctx.fillStyle = col;
      ctx.globalAlpha = (1 - gone) * E.outCubic(p);
      ctx.fillRect(x, sy + (1 - E.outBack(p)) * 30, bw, 10);
    }
    ctx.restore();
  }
}

/** The title: the sting at the gun, then the start line. */
function title(ctx, t) {
  const st = t - (GUN - 0.95);
  const out = prog(t, B(11, 2), B(12));
  sting(ctx, st, { accent: C.cyan, accent2: C.lemon, name: 'Race', sub: 'A RACE TIMER FOR UNA WATCH', out, h: 190 });
  // The start line: shoots out from the mark at the gun.
  const lt = t - GUN;
  if (lt > 0) {
    const e = E.outExpo(clamp(lt / 0.9));
    const a = clamp(1 - prog(lt, 2.8, 4.0)) * (1 - out);
    ctx.save();
    ctx.globalAlpha *= a;
    ctx.fillStyle = C.cyan;
    ctx.fillRect(W / 2 - (W / 2) * e, H / 2 + 190, (W / 2) * e, 2);
    ctx.fillStyle = C.lemon;
    ctx.fillRect(W / 2, H / 2 + 190, (W / 2) * e, 2);
    ctx.restore();
  }
}

// Run interludes: four layouts in rotation.
function runScene(ctx, t, k) {
  const c = CH[k * 2];
  const t0 = k === 0 ? B(12) : c.t0;
  const lt = t - t0;
  const dur = c.t1 - t0;
  const variant = k % 4;
  const tag = RUN_TAGS[k];
  const name = `RUN ${k + 1}/8`;
  const exitQ = prog(lt, dur - 0.25, dur);
  const face = faceState(Math.max(t, c.t0 + 0.001));

  speedLines(ctx, t, C.cyan, { alpha: 0.9 * (1 - exitQ * 0.5), seed: k + 1, speed: variant === 2 ? 0.6 : 1 });

  // The track: 1 km in 100 m ticks, filling as the run goes.
  const fill = prog(t, c.t0, c.t1);
  const ty = H - 210;
  ctx.save();
  ctx.fillStyle = C.rule;
  ctx.fillRect(120, ty, W - 240, 2);
  ctx.fillStyle = C.cyan;
  ctx.fillRect(120, ty, (W - 240) * fill, 2);
  for (let m = 0; m <= 10; m++) {
    const x = 120 + ((W - 240) * m) / 10;
    ctx.fillStyle = m / 10 <= fill ? C.cyan : C.rule;
    ctx.fillRect(x - 1, ty - (m % 5 === 0 ? 14 : 7), 2, m % 5 === 0 ? 30 : 16);
  }
  ctx.restore();
  text(ctx, `${Math.round(fill * 1000)} m`, 120 + (W - 240) * fill, ty - 26, { ...T.label(16, C.cyan), align: fill > 0.9 ? 'right' : 'left' });

  const p = prog(lt, -0.12, 0.33);
  if (variant === 0 || variant === 2) {
    // Words left (or centred), the watch right.
    const wx = variant === 0 ? 150 : 150;
    kinetic(ctx, name, wx, 470, { size: 176, weight: 600, italic: true, tracking: -0.02, color: C.cyan }, p, exitQ, 'rise');
    kinetic(ctx, tag, wx + 6, 560, T.sub(50, C.white), prog(lt, 0.35, 1.1), exitQ, 'chars');
    const wp = ep(lt, 0, 0.7, E.outExpo);
    watch(ctx, {
      cx: lerp(W + 300, 1480, wp), cy: 470, d: 400, rot: -0.12 + (1 - wp) * 0.3,
      screen: (s) => UI.raceFace(s, face), bloom: 0.45, alpha: 1 - exitQ,
    });
  } else if (variant === 1) {
    // The segment time, huge, as an outline, with the name small.
    const secs = face.segSec;
    const big = mss(secs);
    ctx.save();
    ctx.globalAlpha *= (1 - exitQ) * E.outCubic(p);
    ctx.font = '200 470px Poppins';
    ctx.letterSpacing = '-10px';
    ctx.strokeStyle = rgba(C.cyan, 0.9);
    ctx.lineWidth = 2;
    const bw = ctx.measureText(big).width;
    ctx.strokeText(big, W / 2 - bw / 2 + lerp(80, -80, prog(lt, 0, dur)), H / 2 + 150);
    ctx.restore();
    kinetic(ctx, name, W / 2, 250, { size: 64, weight: 600, italic: true, tracking: 0, color: C.cyan, align: 'center' }, p, exitQ, 'rise');
    kinetic(ctx, tag, W / 2, H - 290, { ...T.sub(46, C.white), align: 'center' }, prog(lt, 0.35, 1.1), exitQ, 'chars');
  } else {
    // The watch centred with the name above and the tag below.
    const wp = ep(lt, 0, 0.6, E.outExpo);
    watch(ctx, {
      cx: W / 2 + 180, cy: 460, d: lerp(240, 430, wp), rot: lerp(0.4, 0.04, wp),
      screen: (s) => UI.raceFace(s, face), bloom: 0.5, alpha: 1 - exitQ,
    });
    kinetic(ctx, name, 150, 330, { size: 120, weight: 600, italic: true, tracking: -0.02, color: C.cyan }, p, exitQ, 'rise');
    kinetic(ctx, tag, 154, 420, T.sub(46, C.white), prog(lt, 0.35, 1.1), exitQ, 'chars');
  }
}

/** The split: a press on R2 and the toast, overlaid at a chapter's end. */
function splitFlash(ctx, t) {
  for (let k = 0; k < 15; k++) {
    const tb = CH[k].t1;
    const dt = t - tb;
    if (dt < -0.05 || dt > 0.45) continue;
    const a = Math.exp(-Math.max(0, dt) * 12);
    ctx.save();
    ctx.globalAlpha = 0.07 * a;
    ctx.fillStyle = SEGS[k + 1].type === 'run' ? C.cyan : C.lemon;
    ctx.fillRect(0, 0, W, H);
    ctx.restore();
    // A hairline sweeping across on the split.
    const x = E.outExpo(clamp(dt / 0.4)) * W;
    ctx.fillStyle = rgba(C.white, 0.8 * (1 - clamp(dt / 0.45)));
    ctx.fillRect(x - 2, 0, 2, H);
  }
}

// --- Station chapters --------------------------------------------------------

/** SKIERG: one button. */
function skierg(ctx, t) {
  const c = CH[1];
  const lt = t - c.t0;
  const dur = c.t1 - c.t0;
  stationCard(ctx, lt, c, { out: 1.7 });
  // The watch comes in from the right and settles centre-right.
  const wIn = ep(lt, 0.2, 1.3, E.outExpo);
  const slide = ep(lt, 1.8, 2.8, E.inOutQuint);
  const pressT = c.t1 - 0.12;
  const bz = buzz(t - pressT, 7);
  const wo = { cx: lerp(W + 400, lerp(1260, 1200, slide), wIn) + bz[0], cy: 530 + bz[1], d: lerp(560, 600, slide), rot: lerp(0.25, -0.05, wIn) };
  const dtp = t - pressT;
  const showToast = dtp > 0 && dtp < 0.9;
  const glowR2 = 0.5 + 0.5 * Math.sin(lt * TAU * 1.07);
  watch(ctx, {
    ...wo,
    screen: (s) => (showToast ? UI.splitToast(s, { name: 'SKIERG', sec: SEGS[1].sec }) : UI.raceFace(s, faceState(t, { glow: { r2: glowR2 } }))),
    bloom: 0.55, press: { r2: pressAt(dtp) },
  });
  // Callout to R2.
  const bp = buttonPos(wo, 'r2', 1.34);
  callout(ctx, bp, [bp[0] + 70, bp[1] + 120], 'R2 · SPLIT', prog(lt, 3.0, 3.9));
  if (dtp > -0.05) ripple(ctx, bp[0], bp[1], dtp, C.amber, { rings: 3, speed: 520, life: 0.8 });
  headline(ctx, lt, 'One button.', 'R2 ends every run and every station. Sixteen presses, sixteen splits.', 150, 470, { p0: 2.3, q0: dur - 0.55, size: 120, width: 700 });
  kinetic(ctx, 'EVERY RUN · EVERY STATION', 154, 760, T.label(20, C.amber), prog(lt, 5.4, 6.4), prog(lt, dur - 0.5, dur - 0.1), 'scramble', { t: lt });
}

/** SLED PUSH: the split lock. */
function sledPush(ctx, t) {
  const c = CH[3];
  const lt = t - c.t0;
  const dur = c.t1 - c.t0;
  stationCard(ctx, lt, c, { out: 1.5 });
  headline(ctx, lt, 'Fumble-proof.', 'A split lock of 1 to 10 seconds after every press, so a double press can’t cost you a segment.', 150, 330, { p0: 1.9, q0: dur - 0.5, size: 112, width: 820 });

  // The lock diagram.
  const ax = 150, ay = 760, aw = 1080;
  const axisP = ep(lt, 2.0, 2.8, E.inOutQuart);
  const out = prog(lt, dur - 0.5, dur - 0.1);
  ctx.save();
  ctx.globalAlpha *= 1 - out;
  ctx.fillStyle = C.rule;
  ctx.fillRect(ax, ay, aw * axisP, 2);
  const sec = aw / 6; // 6 s across
  for (let s = 0; s <= 6; s++) {
    if (s / 6 > axisP) break;
    ctx.fillStyle = C.rule;
    ctx.fillRect(ax + s * sec - 1, ay + 8, 2, 10);
    text(ctx, `${s} s`, ax + s * sec, ay + 44, { ...T.mono(15, C.mute), align: 'center' });
  }
  // Press 1: at 0.5 s on the axis; the lock window grows to 3 s.
  const t1 = 3.0;
  const p1 = prog(lt, t1, t1 + 0.25);
  const px1 = ax + 0.5 * sec;
  if (p1 > 0) {
    const winP = ep(lt, t1 + 0.15, t1 + 1.1, E.inOutCubic);
    ctx.fillStyle = rgba(C.lemon, 0.12);
    ctx.fillRect(px1, ay - 150, 3 * sec * winP, 150);
    ctx.fillStyle = rgba(C.lemon, 0.9);
    ctx.fillRect(px1, ay - 150, 3 * sec * winP, 2);
    text(ctx, 'SPLIT LOCK · 3 S', px1 + 3 * sec * winP - 12, ay - 164, { ...T.label(16, C.lemon), align: 'right', alpha: winP });
    marker(ctx, px1, ay, E.outBack(p1), C.amber, 'SPLIT');
  }
  // Press 2: a fumble inside the window, ignored.
  const t2 = 4.4;
  const p2 = prog(lt, t2, t2 + 0.2);
  if (p2 > 0) {
    const px2 = ax + 1.3 * sec;
    const bounce = prog(lt, t2 + 0.2, t2 + 0.9);
    ctx.save();
    ctx.globalAlpha *= 1 - bounce * 0.6;
    marker(ctx, px2, ay - bounce * 40, E.outBack(p2), C.gray, 'IGNORED', true);
    ctx.restore();
  }
  // Press 3: after the window, counted.
  const t3 = 5.6;
  const p3 = prog(lt, t3, t3 + 0.25);
  if (p3 > 0) marker(ctx, ax + 4.2 * sec, ay, E.outBack(p3), C.amber, 'SPLIT');
  ctx.restore();

  // The watch: the split-lock setting, stepping through its values.
  const wIn = ep(lt, 0.4, 1.4, E.outExpo);
  const vals = [3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3];
  const vi = Math.floor(clamp(prog(lt, 3.0, 6.2)) * (vals.length - 1));
  watch(ctx, {
    cx: lerp(W + 400, 1540, wIn), cy: 560, d: 470, rot: lerp(0.3, 0.06, wIn),
    screen: (s) => UI.settingValue(s, { label: 'Split lock', value: `${vals[vi]} s`, nextLabel: 'Vibrate', nextTip: 'on split', index: 2, count: 5 }),
    bloom: 0.5, alpha: 1 - out,
  });
}

function marker(ctx, x, y, s, color, label, crossed = false) {
  ctx.save();
  ctx.translate(x, y);
  ctx.scale(s, s);
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.moveTo(0, 0);
  ctx.lineTo(-11, -20);
  ctx.lineTo(11, -20);
  ctx.closePath();
  ctx.fill();
  ctx.fillRect(-1, -64, 2, 44);
  if (crossed) {
    ctx.strokeStyle = color;
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(-10, -86);
    ctx.lineTo(10, -66);
    ctx.moveTo(10, -86);
    ctx.lineTo(-10, -66);
    ctx.stroke();
  }
  ctx.restore();
  text(ctx, label, x, y - 96 * s, { ...T.label(15, color), align: 'center', alpha: clamp(s) });
}

/** SLED PULL: undo. */
function sledPull(ctx, t) {
  const c = CH[5];
  const lt = t - c.t0;
  const dur = c.t1 - c.t0;
  stationCard(ctx, lt, c, { out: 1.5 });
  headline(ctx, lt, 'Split too early?', 'Undo it. The last split, or even the finish, can be taken back, and the times merge exactly.', 150, 330, { p0: 1.9, q0: dur - 0.5, size: 112, width: 780 });

  // Two bars: the pull, cut in two by an early press, then merged back.
  const out = prog(lt, dur - 0.5, dur - 0.1);
  const bx = 150, by = 700, bw = 1020, bh = 64;
  const inP = ep(lt, 2.2, 3.0, E.outExpo);
  const cut = 0.42;
  const cutAt = 3.4, undoAt = 5.6;
  const split = ep(lt, cutAt, cutAt + 0.3, E.outBack) * (1 - ep(lt, undoAt + 0.2, undoAt + 0.7, E.inOutCubic));
  ctx.save();
  ctx.globalAlpha *= 1 - out;
  const sep = 14 * split;
  ctx.fillStyle = C.lemon;
  ctx.fillRect(bx, by, bw * cut * inP, bh);
  if (inP > cut) ctx.fillRect(bx + bw * cut + sep, by, bw * (inP - cut), bh);
  // Labels.
  const full = SEGS[5].sec;
  const early = Math.round(full * cut);
  const inSplit = lt > cutAt && lt < undoAt + 0.45;
  ctx.fillStyle = '#000';
  if (inP > 0.5) {
    text(ctx, inSplit ? `SLED PULL  ${mss(early)}` : `SLED PULL  ${mss(full)}`, bx + 22, by + 42, T.label(18, '#000'));
    if (inSplit) text(ctx, `RUN 4/8  ${mss(full - early)}`, bx + bw * cut + sep + 22, by + 42, { ...T.label(18, '#000'), alpha: split });
  }
  // The undo sweep.
  if (lt > undoAt) {
    const u = prog(lt, undoAt, undoAt + 0.6);
    text(ctx, 'UNDO', bx + bw * cut + 7, by - 26, { ...T.label(18, C.amber), align: 'center', alpha: 1 - prog(lt, undoAt + 1.2, undoAt + 1.6) });
    ctx.fillStyle = rgba(C.amber, 1 - u);
    ctx.fillRect(bx + bw * cut + 6 - 1, by - 14 + u * 20, 3, bh + 28 - u * 40);
    text(ctx, 'MERGED · TOTAL UNCHANGED', bx, by + bh + 44, { ...T.label(16, C.soft), alpha: prog(lt, undoAt + 0.6, undoAt + 1.1) });
  } else if (lt > cutAt) {
    text(ctx, 'TOO EARLY', bx + bw * cut + 7, by - 26, { ...T.label(18, C.gray), align: 'center', alpha: prog(lt, cutAt, cutAt + 0.3) });
  }
  ctx.restore();

  // The watch: the action menu, Undo last split selected.
  const wIn = ep(lt, 0.4, 1.4, E.outExpo);
  const pressT = c.t0 + undoAt - 0.1;
  const sel = lt > 4.4 ? 1 : 0;
  const shift = lt > 4.1 && lt < 4.4 ? ep(lt, 4.1, 4.4, E.inOutCubic) : 0;
  watch(ctx, {
    cx: lerp(W + 400, 1540, wIn), cy: 540, d: 470, rot: lerp(0.3, 0.05, wIn),
    screen: (s) => {
      if (lt < 3.7) UI.raceFace(s, faceState(t));
      else if (lt < undoAt + 0.1) UI.actionMenu(s, { items: [{ label: 'Resume' }, { label: 'Undo last', tip: 'split' }, { label: 'Pause' }, { label: 'End race' }], sel, shift });
      else UI.raceFace(s, faceState(t));
    },
    bloom: 0.5, alpha: 1 - out, press: { r1: pressAt(t - pressT) },
  });
}

/** BURPEE BROAD JUMPS: Roxzone, 16 → 31. */
function burpees(ctx, t) {
  const c = CH[7];
  const lt = t - c.t0;
  const dur = c.t1 - c.t0;
  stationCard(ctx, lt, c, { out: 1.4 });
  const out = prog(lt, dur - 0.5, dur - 0.1);
  headline(ctx, lt, 'Roxzone, split.', 'Turn it on and every station gets a Roxzone-in and a Roxzone-out lap.', 150, 300, { p0: 1.8, q0: dur - 0.5, size: 112, width: 900 });

  // The watch, small: the toggle, then ROXZONE IN on the face.
  const wIn = ep(lt, 0.3, 1.3, E.outExpo);
  const tog = ep(lt, 3.6, 4.1, E.inOutCubic);
  watch(ctx, {
    cx: lerp(W + 400, 1640, wIn), cy: 290, d: 280, rot: lerp(0.3, 0.05, wIn),
    screen: (s) => (lt < 6.2 ? UI.settingsToggle(s, { on: tog }) : UI.raceFace(s, { ...faceState(t), type: 'rox', name: 'ROXZONE IN', work: '', idx: 13, count: 31, next: 'BURPEE BROAD JUMPS' })),
    bloom: 0.5, alpha: 1 - out, press: { r1: pressAt(t - (c.t0 + 3.55)) },
  });
  // The strip, reflowing from 16 to 31 blocks.
  const on = ep(lt, 4.2, 5.6, E.inOutQuart);
  const x0 = 150, x1 = W - 150, y = 700, h = 56;
  const blocks = [];
  SEGS.forEach((s, i) => {
    if (s.type === 'run') blocks.push({ col: C.cyan, w: 1 });
    else {
      blocks.push({ col: C.orchid, w: on, rox: true });
      blocks.push({ col: C.lemon, w: 1 });
      if (i < 15) blocks.push({ col: C.orchid, w: on, rox: true });
    }
  });
  const gap = 6;
  const nVis = blocks.reduce((a, b) => a + b.w, 0);
  const unit = (x1 - x0 - gap * (nVis - 1)) / nVis;
  const inP = ep(lt, 2.0, 3.0, E.outExpo);
  let x = x0;
  ctx.save();
  ctx.globalAlpha *= 1 - out;
  blocks.forEach((b, i) => {
    if (b.w <= 0.001) return;
    const bw = unit * b.w;
    const pop = b.rox ? E.outBack(clamp(on * 1.4 - (i / blocks.length) * 0.4)) : 1;
    ctx.fillStyle = b.col;
    const hh = h * (b.rox ? 0.72 : 1) * pop;
    ctx.globalAlpha = (1 - out) * clamp(inP * 1.5 - i / blocks.length);
    ctx.fillRect(x, y + (h - hh) / 2, bw, hh);
    x += bw + gap * b.w;
  });
  ctx.restore();
  // The count.
  const n = Math.round(lerp(16, 31, on));
  kinetic(ctx, String(n), W - 150, 640, { size: 200, weight: 300, tracking: -0.04, color: on > 0.5 ? C.orchid : C.white, align: 'right' }, prog(lt, 2.2, 2.8), out, 'rise');
  kinetic(ctx, 'SEGMENTS', W - 470, 640, { ...T.label(20, C.soft), align: 'right' }, prog(lt, 2.4, 3.0), out, 'scramble', { t: lt });
  // Legend.
  const lg = prog(lt, 5.8, 6.4);
  if (lg > 0) {
    ctx.save();
    ctx.globalAlpha *= lg * (1 - out);
    [['RUN', C.cyan], ['STATION', C.lemon], ['ROXZONE IN / OUT', C.orchid]].forEach(([l, col], i) => {
      ctx.fillStyle = col;
      ctx.fillRect(150 + i * 260, 800, 12, 12);
      text(ctx, l, 150 + i * 260 + 22, 812, T.label(16, C.soft));
    });
    text(ctx, 'NO ROXZONE-OUT AFTER THE LAST STATION', 150, 850, T.label(14, C.mute));
    ctx.restore();
  }
}

/** ROW: heart rate. The music breaks down here. */
function row(ctx, t) {
  const c = CH[9];
  const lt = t - c.t0;
  const dur = c.t1 - c.t0;
  stationCard(ctx, lt, c, { out: 1.4, y: 330 });
  const out = prog(lt, dur - 0.5, dur - 0.1);
  // A giant zone arc across the bottom of the frame.
  const arcIn = ep(lt, 1.0, 2.6, E.inOutQuart);
  const R = 1500;
  const cx = W / 2, cy = H + R - 360;
  const cols = [C.gray, C.chartreuse, C.yellow, C.amber, C.red];
  const bpm = Math.round(lerp(138, 176, ep(lt, 1.5, dur - 1.2, E.inOutSine)) + 2 * Math.sin(lt * 5));
  const zone = UI.zoneFor(bpm);
  ctx.save();
  ctx.globalAlpha *= 1 - out;
  for (let i = 0; i < 5; i++) {
    const a0 = -64 + i * 26, a1 = a0 + 24;
    const aa0 = ((a0 - 90) * Math.PI) / 180, aa1 = ((lerp(a0, a1, clamp(arcIn * 5 - i)) - 90) * Math.PI) / 180;
    if (aa1 <= aa0) continue;
    ctx.strokeStyle = cols[i];
    ctx.lineWidth = i === zone ? 34 : 14;
    ctx.globalAlpha = (1 - out) * (i === zone ? 1 : 0.45);
    ctx.beginPath();
    ctx.arc(cx, cy, R, aa0, aa1);
    ctx.stroke();
  }
  // Marker arrow on the active zone.
  if (zone >= 0 && arcIn > 0.9) {
    const am = ((-64 + zone * 26 + 12 - 90) * Math.PI) / 180;
    const px = cx + Math.cos(am) * (R - 44), py = cy + Math.sin(am) * (R - 44);
    ctx.globalAlpha = 1 - out;
    ctx.fillStyle = cols[zone];
    ctx.beginPath();
    ctx.moveTo(px + Math.cos(am) * 22, py + Math.sin(am) * 22);
    ctx.lineTo(px - Math.sin(am) * 16, py + Math.cos(am) * 16);
    ctx.lineTo(px + Math.sin(am) * 16, py - Math.cos(am) * 16);
    ctx.closePath();
    ctx.fill();
  }
  ctx.restore();

  // Heartbeat trace across the frame, pulsing on the beat.
  const trP = ep(lt, 1.2, 2.2, E.inOutCubic);
  ctx.save();
  ctx.globalAlpha *= (1 - out) * trP;
  ctx.strokeStyle = rgba(C.red, 0.85);
  ctx.lineWidth = 3;
  ctx.lineJoin = 'round';
  ctx.beginPath();
  const base = 610;
  for (let x = 0; x <= W; x += 6) {
    const tt = lt - (W - x) / 900;
    const ph = ((tt / TM.beat) % 1 + 1) % 1;
    let yv = 0;
    if (ph < 0.08) yv = -Math.sin((ph / 0.08) * Math.PI) * 110;
    else if (ph < 0.14) yv = Math.sin(((ph - 0.08) / 0.06) * Math.PI) * 40;
    else if (ph > 0.4 && ph < 0.52) yv = -Math.sin(((ph - 0.4) / 0.12) * Math.PI) * 18;
    const fadeX = clamp(x / 300) * clamp((W - x) / 300);
    const yy = base + yv * fadeX;
    if (x === 0) ctx.moveTo(x, yy);
    else ctx.lineTo(x, yy);
  }
  ctx.stroke();
  ctx.restore();

  // The number.
  kinetic(ctx, String(bpm), W / 2 + 20, 520, { size: 230, weight: 300, tracking: -0.04, color: C.white, align: 'center' }, prog(lt, 1.5, 2.1), out, 'rise');
  kinetic(ctx, 'BPM', W / 2 + 250, 520, T.label(26, zone >= 0 ? cols[zone] : C.soft), prog(lt, 1.7, 2.3), out, 'scramble', { t: lt });
  headline(ctx, lt, 'Heart rate, every segment.', 'Average and maximum for each split, from the watch or a chest strap.', 150, 820, { p0: 3.2, q0: dur - 0.5, size: 64, width: 900 });
  // Per-segment bars, top right.
  const bp = prog(lt, 4.2, 5.4);
  if (bp > 0) {
    ctx.save();
    ctx.globalAlpha *= 1 - out;
    for (let i = 0; i < 16; i++) {
      const s = SEGS[i];
      const pp = clamp(bp * 16 - i);
      if (pp <= 0) continue;
      const avg = s.hr + 2, mx = s.hr + 14;
      const x = 1240 + i * 32;
      const hA = (avg - 120) * 3.2 * E.outCubic(pp), hM = (mx - 120) * 3.2 * E.outCubic(pp);
      const zc = cols[Math.max(0, UI.zoneFor(avg))];
      ctx.fillStyle = zc;
      ctx.fillRect(x, 330 - hA, 18, hA);
      ctx.fillStyle = C.white;
      ctx.fillRect(x, 330 - hM - 6, 18, 3);
    }
    text(ctx, 'AVG / MAX · 16 SEGMENTS', 1240, 380, T.label(14, C.mute));
    ctx.restore();
  }
}

/** FARMERS CARRY: formats and run length. */
function carry(ctx, t) {
  const c = CH[11];
  const lt = t - c.t0;
  const dur = c.t1 - c.t0;
  stationCard(ctx, lt, c, { out: 1.4 });
  const out = prog(lt, dur - 0.5, dur - 0.1);
  headline(ctx, lt, ['Race it.', 'Or sim it.'], 'The full race or either half, with real round numbers. Shorten the runs and it becomes a sim, on the watch and in the file.', 150, 300, { p0: 1.7, q0: dur - 0.5, size: 104, width: 760 });

  // Formats: three strips.
  const fp = prog(lt, 2.6, 3.6);
  const rows = [
    ['FULL', 'ROUNDS 1-8', (i) => true],
    ['HALF A', 'ROUNDS 1-4', (i) => i < 8],
    ['HALF B', 'ROUNDS 5-8', (i) => i >= 8],
  ];
  ctx.save();
  ctx.globalAlpha *= 1 - out;
  rows.forEach(([n, r, on], j) => {
    const y = 330 + j * 70;
    const pj = clamp(fp * 3 - j);
    if (pj <= 0) return;
    text(ctx, n, 1060, y + 18, { ...T.label(18, C.white), alpha: pj });
    text(ctx, r, 1060, y + 42, { ...T.label(13, C.mute), alpha: pj });
    for (let i = 0; i < 16; i++) {
      const s = SEGS[i];
      ctx.fillStyle = s.type === 'run' ? C.cyan : C.lemon;
      ctx.globalAlpha = (1 - out) * pj * (on(i) ? 1 : 0.14);
      ctx.fillRect(1250 + i * 34, y, 28, 36);
    }
    ctx.globalAlpha = 1 - out;
  });
  ctx.restore();

  // The run length ruler: 100 m to 1 km.
  const rp = ep(lt, 4.2, 5.0, E.outExpo);
  const rx = 1060, ry = 690, rw = 400;
  ctx.save();
  ctx.globalAlpha *= (1 - out) * rp;
  ctx.fillStyle = C.rule;
  ctx.fillRect(rx, ry, rw, 2);
  for (let m = 1; m <= 10; m++) {
    const x = rx + ((m - 1) / 9) * rw;
    ctx.fillStyle = C.rule;
    ctx.fillRect(x - 1, ry - 8, 2, 18);
    if (m === 1 || m === 5 || m === 10) text(ctx, m === 10 ? '1 km' : `${m * 100} m`, x, ry + 40, { ...T.label(14, C.mute), align: 'center' });
  }
  const len = Math.round(lerp(10, 5, ep(lt, 5.2, 6.4, E.inOutCubic)));
  const mx = rx + ((len - 1) / 9) * rw;
  ctx.fillStyle = C.cyan;
  ctx.fillRect(rx, ry, mx - rx, 2);
  ctx.beginPath();
  ctx.arc(mx, ry + 1, 9, 0, TAU);
  ctx.fill();
  text(ctx, 'RUN LENGTH', rx, ry - 34, T.label(16, C.soft));
  text(ctx, len === 10 ? '1 km · RACE' : `${len * 100} m · SIM`, mx, ry - 26, { ...T.label(16, len === 10 ? C.cyan : C.amber), align: 'center' });
  ctx.restore();

  // The watch: On your marks, race then sim.
  const wIn = ep(lt, 0.3, 1.3, E.outExpo);
  const sim = lt > 6.0;
  watch(ctx, {
    cx: lerp(W + 400, 1725, wIn), cy: 720, d: 280, rot: lerp(0.3, -0.05, wIn),
    screen: (s) => UI.onYourMarks(s, sim ? { format: 'Full sim', segments: '16 segments · 500 m runs' } : {}),
    bloom: 0.45, alpha: 1 - out,
  });
}

/** SANDBAG LUNGES: sixteen labelled laps. */
function lunges(ctx, t) {
  const c = CH[13];
  const lt = t - c.t0;
  const dur = c.t1 - c.t0;
  stationCard(ctx, lt, c, { out: 1.4 });
  const out = prog(lt, dur - 0.5, dur - 0.1);
  headline(ctx, lt, 'Every split, a lap.', 'Sixteen labelled laps in Strava and Garmin Connect, not one 90-minute blob.', 150, 330, { p0: 1.7, q0: dur - 0.5, size: 88, width: 700 });
  kinetic(ctx, 'SEGMENT_TYPE · ROUND · STATION_ID', 154, 560, T.label(16, C.mute), prog(lt, 2.6, 3.4), out, 'scramble', { t: lt });

  // The file, and the laps pouring out of it.
  const tx = 1090, ty = 150, tw = 680, rh = 44;
  const head = prog(lt, 2.0, 2.6);
  ctx.save();
  ctx.globalAlpha *= 1 - out;
  kinetic(ctx, 'HybridXRace.fit', tx, ty, T.mono(22, C.white), head, 0, 'type', { t: lt });
  text(ctx, 'LAP', tx, ty + 44, { ...T.label(13, C.mute), alpha: head });
  text(ctx, 'NAME', tx + 70, ty + 44, { ...T.label(13, C.mute), alpha: head });
  text(ctx, 'TIME', tx + 520, ty + 44, { ...T.label(13, C.mute), alpha: head, align: 'right' });
  text(ctx, 'AVG HR', tx + tw, ty + 44, { ...T.label(13, C.mute), alpha: head, align: 'right' });
  for (let i = 0; i < 16; i++) {
    const s = SEGS[i];
    const pi = prog(lt, 2.8 + i * 0.12, 3.3 + i * 0.12);
    if (pi <= 0) continue;
    const e = E.outExpo(pi);
    const y = ty + 70 + i * rh;
    const x = lerp(tx - 300, tx, e);
    ctx.globalAlpha = (1 - out) * e;
    ctx.fillStyle = i % 2 ? '#0c0e10' : '#111417';
    ctx.fillRect(x - 12, y - 28, tw + 24, rh - 4);
    ctx.fillStyle = s.type === 'run' ? C.cyan : C.lemon;
    ctx.fillRect(x - 12, y - 28, 4, rh - 4);
    text(ctx, String(i + 1).padStart(2, '0'), x, y, T.mono(18, C.mute));
    text(ctx, s.name, x + 70, y, T.mono(18, C.white));
    text(ctx, mss(s.sec), x + 520, y, { ...T.mono(18, C.white), align: 'right' });
    text(ctx, `${s.hr + 2}`, x + tw, y, { ...T.mono(18, C.soft), align: 'right' });
  }
  ctx.restore();
  // Destinations, set plainly (no logos).
  kinetic(ctx, '→ STRAVA · GARMIN CONNECT', 154, 640, T.label(20, C.amber), prog(lt, 4.8, 5.6), out, 'scramble', { t: lt });
}

/** WALL BALLS: the last segment, a hundred reps round the watch. */
function wallBalls(ctx, t) {
  const c = CH[15];
  const lt = t - c.t0;
  const dur = c.t1 - c.t0;
  stationCard(ctx, lt, c, { out: 1.5, y: 330 });
  // After the card: the watch zooms in at the centre, the rep ring round it.
  const wIn = ep(lt, 1.7, 2.8, E.outExpo);
  const pressT = c.t1 - 0.12;
  const bz = buzz(t - pressT, 9);
  const push = ep(lt, 3, dur, E.inQuad);
  const d = lerp(120, 560, wIn) + push * 50;
  const wo = { cx: W / 2 + bz[0], cy: H / 2 + 10 + bz[1], d, rot: lerp(-0.6, 0, wIn) };
  const reps = Math.floor(100 * E.inQuad(prog(lt, 2.4, dur - 0.15)));
  const rr = 440 + push * 30;
  const ringIn = prog(lt, 2.0, 3.0);
  if (ringIn > 0) {
    ctx.save();
    for (let i = 0; i < 100; i++) {
      const pi = clamp(ringIn * 1.6 - (i / 100) * 0.6);
      if (pi <= 0) continue;
      const a = -Math.PI / 2 + (i / 100) * TAU;
      const on = i < reps;
      const head = on && i === reps - 1;
      ctx.strokeStyle = on ? C.lemon : C.rule;
      ctx.lineWidth = on ? 4 : 2;
      const len = (i % 10 === 0 ? 30 : 16) * (head ? 1.8 : 1) * E.outCubic(pi);
      ctx.beginPath();
      ctx.moveTo(wo.cx + Math.cos(a) * rr, wo.cy + Math.sin(a) * rr);
      ctx.lineTo(wo.cx + Math.cos(a) * (rr + len), wo.cy + Math.sin(a) * (rr + len));
      ctx.stroke();
    }
    ctx.restore();
  }
  light(ctx, wo.cx, wo.cy, d * 1.5, rgba(C.lemon, 0.22), push * 0.9);
  if (wIn > 0) {
    watch(ctx, {
      ...wo,
      screen: (s) => UI.raceFace(s, faceState(t, { glow: { r2: 0.5 + 0.5 * Math.sin(lt * (4 + 12 * push)) } })),
      bloom: 0.6, press: { r2: pressAt(t - pressT) }, alpha: clamp(wIn * 3),
    });
  }
  const tq = prog(lt, dur - 0.25, dur);
  kinetic(ctx, String(reps).padStart(3, '0'), W - 150, 560, { size: 120, weight: 300, tracking: 0, color: C.lemon, align: 'right' }, prog(lt, 2.6, 3.2), tq, 'fade');
  kinetic(ctx, 'OF 100 REPS', W - 154, 610, { ...T.label(18, C.soft), align: 'right' }, prog(lt, 2.8, 3.4), tq, 'fade');
  kinetic(ctx, 'Last', 150, 500, T.head(88, C.white), prog(lt, 3.0, 3.6), tq, 'rise');
  kinetic(ctx, 'segment.', 150, 592, T.head(88, C.white), prog(lt, 3.1, 3.7), tq, 'rise');
  kinetic(ctx, 'One more press.', 154, 670, T.sub(40, C.soft), prog(lt, 4.4, 5.1), tq, 'chars');
}

/** The finish: race time, saved, the summary. */
function finish(ctx, t) {
  const lt = t - FINISH;
  // The flash.
  const fl = Math.exp(-Math.max(0, lt) * 5);
  const d = 520;
  const wo = { cx: W - 480, cy: H / 2 + 10, d, rot: 0 };
  const saveT = 3.0;
  const sumT = 4.6;
  watch(ctx, {
    ...wo,
    screen: (s) => {
      if (lt < saveT) UI.finished(s, { sec: RACE.total, glow: { r1: 0.5 + 0.5 * Math.sin(lt * 6) } });
      else if (lt < sumT) UI.saved(s, { p: prog(lt, saveT, saveT + 0.8) });
      else UI.summary(s, { page: lt > 6.2 ? 1 : 0 });
    },
    bloom: 0.55, press: { r1: pressAt(t - (FINISH + saveT - 0.1)) },
  });
  // The race time, large.
  const out = prog(lt, 7.1, 7.5);
  kinetic(ctx, 'RACE TIME', 154, 330, T.label(22, C.soft), prog(lt, 0.3, 1.0), out, 'scramble', { t: lt });
  const big = hmss(RACE.total);
  kinetic(ctx, big, 140, 520, { size: 230, weight: 300, tracking: -0.03, color: C.white }, prog(lt, 0.15, 0.7), out, 'rise');
  // The breakdown.
  const rows = [['RUNS', hmss(RACE.runs), C.cyan], ['STATIONS', hmss(RACE.stations), C.lemon], ['AVG HR', '161 bpm', C.white], ['MAX HR', '183 bpm', C.white]];
  rows.forEach(([k, v, col], i) => {
    const p = prog(lt, 1.2 + i * 0.12, 1.8 + i * 0.12);
    kinetic(ctx, k, 154 + i * 260, 630, T.label(16, C.mute), p, out, 'fade');
    kinetic(ctx, v, 150 + i * 260, 680, T.mono(34, col), p, out, 'rise');
  });
  strip(ctx, 154, 760, 1000, 16, { reveal: clamp(lt / 1.2) });
  kinetic(ctx, 'Sixteen splits. One button.', 150, 900, T.head(60, C.white), prog(lt, 5.0, 5.6), out, 'rise');
  if (fl > 0.01) {
    ctx.save();
    ctx.globalAlpha = 0.85 * fl;
    ctx.fillStyle = C.white;
    ctx.fillRect(0, 0, W, H);
    ctx.restore();
  }
}

// --- The film -------------------------------------------------------------------

function sceneFor(t) {
  if (t < B(4)) return coldOpen;
  if (t < B(8) - 0.95) return countIn;
  if (t < B(12)) return title;
  if (t >= END) return null;
  if (t >= FINISH) return finish;
  const c = chapterAt(t);
  if (!c) return null;
  if (c.seg.type === 'run') return (x, tt) => runScene(x, tt, c.k / 2);
  return [skierg, sledPush, sledPull, burpees, row, carry, lunges, wallBalls][(c.k - 1) / 2];
}

function drawHud(ctx, t) {
  const a = prog(t, B(12), B(12) + 0.6) * (1 - prog(t, END - 0.8, END - 0.2));
  if (a <= 0) return;
  const c = chapterAt(t);
  const k = c ? c.k : 15;
  const s = SEGS[k];
  hud(ctx, t, {
    app: 'RACE', accent: C.lemon, alpha: a,
    topRight: t >= FINISH ? 'FINISHED' : `${s.name}`,
    topRightColor: t >= FINISH ? C.white : UI.accentFor(s.type),
    topRight2: t >= FINISH ? '16 OF 16' : `${k + 1} OF 16`,
    progress: raceSec(t) / RACE.total,
    ticks: SEGS.map((sg) => sg.end / RACE.total),
    tickColors: SEGS.map((sg) => UI.accentFor(sg.type)),
    bottomRight: hmss(raceSec(t)),
    bottomRightLabel: 'RACE TIME',
  });
}

function draw(ctx, t) {
  const scene = sceneFor(t);
  // A small push on each hard cut into a run: the split lands.
  const cc = chapterAt(t);
  ctx.save();
  if (cc && cc.k > 0 && cc.seg.type === 'run' && t < FINISH) {
    const kt = t - cc.t0;
    if (kt >= 0 && kt < 0.4) {
      const k = 1 + 0.035 * (1 - E.outCubic(kt / 0.4));
      ctx.translate(W / 2, H / 2);
      ctx.scale(k, k);
      ctx.translate(-W / 2, -H / 2);
    }
  }
  // Station chapters enter on a chevron wipe from the run before.
  let wiped = false;
  for (let k = 1; k < 16; k += 2) {
    const tb = CH[k].t0;
    const p = prog(t, tb - 0.32, tb + 0.28);
    if (p > 0 && p < 1) {
      runScene(ctx, Math.min(t, tb - 0.001), (k - 1) / 2);
      const next = sceneFor(tb + 0.001);
      chevronWipe(ctx, p, (c) => {
        c.fillStyle = '#000';
        c.fillRect(0, 0, W, H);
        next(c, t);
      }, { dir: 'right', color: C.lemon, band: 64, band2: 12, color2: C.white });
      wiped = true;
      break;
    }
  }
  if (!wiped && scene) scene(ctx, t);
  ctx.restore();
  if (t >= END - 0.1) {
    const lt = t - END;
    const fade = 1 - prog(t, DURATION - 0.9, DURATION - 0.05);
    ctx.save();
    ctx.globalAlpha = fade;
    endCard(ctx, lt, { app: 'race', accent: C.cyan, accent2: C.lemon, name: 'Race' });
    ctx.restore();
  }
  splitFlash(ctx, t);
  drawHud(ctx, t);
}

// --- Cues for the soundtrack --------------------------------------------------

function cues() {
  const q = [];
  q.push({ t: 0, type: 'section', name: 'intro' });
  q.push({ t: B(4), type: 'hit', v: 0.8 }, { t: B(5), type: 'hit', v: 0.9 }, { t: B(6), type: 'hit', v: 1.0 });
  q.push({ t: B(6) + 0.2, type: 'riser', until: GUN - 0.02 });
  q.push({ t: GUN - 0.95, type: 'draw' });
  q.push({ t: GUN, type: 'lock' });
  q.push({ t: GUN, type: 'drop' });
  for (let k = 0; k < 15; k++) q.push({ t: CH[k].t1 - 0.12, type: 'split', station: SEGS[k].type === 'station' });
  for (let k = 1; k < 16; k += 2) q.push({ t: CH[k].t0 - 0.32, type: 'whoosh' });
  // Feature beats.
  q.push({ t: CH[3].t0 + 3.0, type: 'click' }, { t: CH[3].t0 + 4.4, type: 'thud' }, { t: CH[3].t0 + 5.6, type: 'click' });
  q.push({ t: CH[5].t0 + 3.4, type: 'click' }, { t: CH[5].t0 + 5.5, type: 'undo' });
  q.push({ t: CH[7].t0 + 3.55, type: 'toggle' }, { t: CH[7].t0 + 4.2, type: 'swell', until: CH[7].t0 + 5.6 });
  for (let i = 0; i < 16; i++) q.push({ t: CH[13].t0 + 2.8 + i * 0.12, type: 'tick', v: 0.5 });
  q.push({ t: FINISH - 0.12, type: 'finishPress' });
  q.push({ t: FINISH, type: 'finish' });
  q.push({ t: FINISH + 2.9, type: 'saved' });
  q.push({ t: END + 0.95, type: 'logo' });
  return q;
}

export default {
  id: 'race',
  title: 'HybridX Race',
  duration: DURATION,
  bpm: 128,
  sections: [
    { name: 'intro', bar: 0 }, { name: 'countin', bar: 4 }, { name: 'drop', bar: 8 },
    { name: 'groove', bar: 12 }, { name: 'breakdown', bar: 44 }, { name: 'rebuild', bar: 50 },
    { name: 'final', bar: 66 }, { name: 'finish', bar: 72 }, { name: 'end', bar: 76 },
  ],
  draw,
  cues,
};
