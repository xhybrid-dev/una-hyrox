// HybridX, the unboxing reel: "Box to start line", portrait, from real footage.
//
// 120 BPM, 16 bars (32 s), 1080 x 1920 for Instagram Reels.
// Jon's own unboxing of a UNA Watch (two phone clips in ../unbooxing/), cut
// as a race. It opens on the finished watch and asks how fast, rewinds to the
// empty table, and the clock starts as the box slides in. Eight stations, one
// per step of the unboxing, each carrying one of UNA's own claims (from
// unawatch.com and the box itself); a split on every station. The race clock
// is real elapsed time, read from the clips' own timestamps, so it runs fast
// through the sped-up shots and jumps the gap between the clips. The finish
// is the Run screen. Then the open platform, and the HybridX end card.
//
// The lid is annotated like a coach's telestrator: the box prints three
// promises inside its lid, and the reel draws on the freeze-frame.

import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { clamp, lerp, prog, ep, E, hash, tempo, TAU } from '../lib/core.mjs';
import { text, kinetic, rgba, strokePart, vignette } from '../lib/gfx.mjs';
import { C, T, chevronWipe } from '../lib/brand.mjs';
import { RW as W, RH as H, SAFE, fit, reelHud, reelHead, reelEnd } from '../lib/reel.mjs';
import { Footage, placement } from '../lib/footage.mjs';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const CLIPS = path.join(HERE, '..', 'unbooxing');
const SOURCES = {
  A: path.join(CLIPS, 'PXL_20260928_104714350.TS.mp4'), // the unboxing, 114.6 s
  B: path.join(CLIPS, 'PXL_20260928_105038991.TS.mp4'), // power on, 89.0 s
};
const SW = 1080, SH = 1920; // the clips, upright
const FOOT = new Footage(SOURCES, SW, SH);

const TM = tempo(120);
const B = (b, beats = 0) => TM.at(b, beats);
const DURATION = B(16);

// --- The race clock ------------------------------------------------------------
// Each clip's first frame on the wall clock, from the Pixel's file names
// (10:47:14.350 and 10:50:38.991), in seconds after the first. The clock
// starts as the box slides into shot.
const WALL = { A: 0, B: 204.641 };
const GO = { src: 'A', s: 2.05 };
const real = (src, s) => WALL[src] + s - (WALL[GO.src] + GO.s);

/** m:ss.t */
function clk(sec) {
  sec = Math.max(0, sec);
  const m = Math.floor(sec / 60);
  const s = sec - m * 60;
  return `${m}:${String(Math.floor(s)).padStart(2, '0')}.${Math.floor((s % 1) * 10)}`;
}

// --- The stations -----------------------------------------------------------------

const ST = [
  { name: 'THE BOX', t0: B(1), t1: B(1, 3) },
  { name: 'THE SEAL', t0: B(1, 3), t1: B(2, 2) },
  { name: 'THE LID', t0: B(2, 2), t1: B(4, 2) },
  { name: 'THE WATCH', t0: B(4, 2), t1: B(6) },
  { name: 'USB-C', t0: B(6), t1: B(7) },
  { name: 'THE STRAPS', t0: B(7), t1: B(8) },
  { name: 'POWER ON', t0: B(8), t1: B(10) },
  { name: 'START LINE', t0: B(10), t1: B(12) },
];
const REWIND = 1.3; // the hook rewinds from here to GO at B(1)
const FINISH = B(12);
const BRIDGE = B(13);
const END = B(14);
const FREEZE = 7.2; // the lid, held for the telestrator

// --- The shots ----------------------------------------------------------------------
// Each shot plays source [a, b] of a clip over reel time [t0, t1] (so its
// speed is their ratio), or holds the frame at a. `ease` blends in a speed
// ramp (slow in, fast through the middle, slow out). The frame: source point
// (px, py) sits at output (ox, oy), scaled from z0 to z1 across the shot;
// `from` snaps in from another framing over the shot's first `snap` seconds.
// `nat` is how much of the clip's own sound the score keeps.

const SHOTS = [
  // The hook: the finished watch, from later on (flash-forward).
  { id: 'hero', src: 'B', a: 81.55, b: 82.75, t0: 0, t1: REWIND, frame: { z0: 1.26, z1: 1.36, px: 470, py: 800, ox: 540, oy: 1090 } },
  { id: 'rewind', t0: REWIND, t1: B(1), rewind: true },
  // 01 The box slides in. 02 Over, and the knife through the seal.
  { id: 'box', src: 'A', a: GO.s, b: 3.55, t0: B(1), t1: B(1, 3), nat: 1, frame: { z0: 1.12, z1: 1.2, px: 460, py: 920, ox: 540, oy: 1020 } },
  { id: 'flip', src: 'A', a: 6.6, b: 7.9, t0: B(1, 3), t1: 3.8, nat: 0.35, frame: { z0: 1.12, z1: 1.14, px: 520, py: 960, ox: 540, oy: 1020 } },
  { id: 'knife', src: 'A', a: 11.05, b: 12.25, t0: 3.8, t1: B(2, 2), nat: 1.25, frame: { z0: 1.16, z1: 1.24, px: 470, py: 860, ox: 540, oy: 1010 } },
  // 03 The lid opens, the card comes out, and the lid is held.
  { id: 'lid', src: 'A', a: 15.2, b: 16.9, t0: B(2, 2), t1: 6.2, nat: 0.9, frame: { z0: 1.04, z1: 1.08, px: 540, py: 900, ox: 540, oy: 980 } },
  { id: 'card', src: 'A', a: 22.3, b: 25.0, t0: 6.2, t1: FREEZE, ease: 0.6, nat: 0.3, frame: { z0: 1.02, z1: 1.02, px: 430, py: 520 } },
  { id: 'lidHold', src: 'A', a: 25.0, hold: true, t0: FREEZE, t1: B(4, 2), frame: { z0: 1.45, z1: 1.53, px: 430, py: 300, ox: 520, oy: 420, from: { z: 1.02, px: 430, py: 520 }, snap: 0.35 } },
  // 04 The watch, lifted out.
  { id: 'reach', src: 'A', a: 41.8, b: 43.5, t0: B(4, 2), t1: 9.85, nat: 0.4, frame: { z0: 1.12, z1: 1.16, px: 470, py: 860, ox: 540, oy: 1080 } },
  { id: 'held', src: 'A', a: 43.5, b: 45.65, t0: 9.85, t1: B(6), nat: 1, frame: { z0: 1.2, z1: 1.3, px: 430, py: 860, ox: 540, oy: 1080 } },
  // 05 USB-C: the cable uncoiled, and its plug.
  { id: 'cable', src: 'A', a: 69.8, b: 71.8, t0: B(6), t1: 12.9, nat: 0.5, frame: { z0: 1.12, z1: 1.16, px: 500, py: 900, ox: 540, oy: 1020 } },
  { id: 'plug', src: 'A', a: 75.35, b: 76.45, t0: 12.9, t1: B(7), nat: 0.9, frame: { z0: 1.16, z1: 1.24, px: 560, py: 760, ox: 540, oy: 1010 } },
  // 06 The white strap.
  { id: 'strapBox', src: 'A', a: 89.7, b: 91.3, t0: B(7), t1: B(7, 2), nat: 0.8, frame: { z0: 1.08, z1: 1.12, px: 540, py: 700, ox: 540, oy: 760 } },
  { id: 'strap', src: 'A', a: 95.6, b: 97.1, t0: B(7, 2), t1: B(8), nat: 0.5, frame: { z0: 1.06, z1: 1.1, px: 540, py: 700, ox: 540, oy: 760 } },
  // 07 Power on: the cable into the watch, and the screen lights (6.5 s in
  // the clip) on the downbeat.
  { id: 'plugIn', src: 'B', a: 3.2, b: 6.0, t0: B(8), t1: B(8, 3), ease: 0.5, nat: 0.9, frame: { z0: 1.14, z1: 1.2, px: 500, py: 1000, ox: 540, oy: 1080 } },
  { id: 'boot', src: 'B', a: 6.0, b: 8.5, t0: B(8, 3), t1: B(10), nat: 1.2, frame: { z0: 1.2, z1: 1.32, px: 450, py: 1070, ox: 540, oy: 1090 } },
  // 08 The watch face, into the sport menu, down to Run.
  { id: 'menu', src: 'B', a: 56.6, b: 61.45, t0: B(10), t1: FINISH, ease: 0.35, nat: 0.6, frame: { z0: 1.42, z1: 1.52, px: 420, py: 760, ox: 540, oy: 1080 } },
  // The finish, held on Run.
  { id: 'finish', src: 'B', a: 61.45, hold: true, t0: FINISH, t1: BRIDGE, frame: { z0: 1.52, z1: 1.58, px: 420, py: 760, ox: 540, oy: 1080 } },
  // The flat lay, a touch slower than life, under the open platform and the wipe.
  { id: 'flatlay', src: 'A', a: 112.75, b: 114.5, t0: BRIDGE, t1: END + 0.3, frame: { z0: 1.08, z1: 1.16, px: 540, py: 1180, ox: 540, oy: 1180 } },
];

function shotAt(t) {
  for (const s of SHOTS) if (t >= s.t0 && t < s.t1) return s;
  return null;
}

/** Source time of shot `sh` at reel time t. */
function srcTime(sh, t) {
  if (sh.hold) return sh.a;
  const p = prog(t, sh.t0, sh.t1);
  const q = lerp(p, E.inOutSine(p), sh.ease || 0);
  return lerp(sh.a, sh.b, q);
}

// The rewind: the reel's own shots, backwards, a still every three frames at
// 60 fps, ending on the empty table before the box arrives.
const REW_STILLS = (() => {
  const n = Math.round(((B(1) - REWIND) * 60) / 3);
  const out = [];
  for (let k = 0; k < n - 1; k++) {
    const t = lerp(FINISH - 0.1, B(1) + 0.15, k / (n - 2));
    const sh = shotAt(t);
    out.push({ src: sh.src, s: +srcTime(sh, t).toFixed(3) });
  }
  out.push({ src: 'A', s: 1.2 });
  return out;
})();

function rewindAt(t) {
  const k = clamp(Math.floor(((t - REWIND) * 60) / 3), 0, REW_STILLS.length - 1);
  return REW_STILLS[k];
}

/** The race clock at reel time t. */
function clockAt(t) {
  if (t < REWIND) return null;
  if (t < B(1)) return real(rewindAt(t).src, rewindAt(t).s);
  if (t >= FINISH) return real('B', 61.45);
  const sh = shotAt(t);
  // A hold keeps the clock running at real speed.
  if (sh.hold) return real(sh.src, sh.a) + (t - sh.t0);
  return real(sh.src, srcTime(sh, t));
}

function stationAt(t) {
  for (let i = 0; i < ST.length; i++) if (t >= ST[i].t0 && t < ST[i].t1) return i;
  return -1;
}

// --- Drawing helpers --------------------------------------------------------------------

/** The plate's placement for shot `sh` at reel time t: [z, dx, dy]. */
function placeOf(sh, t) {
  const f = sh.frame || {};
  const p = prog(t, sh.t0, sh.t1);
  let z = lerp(f.z0 ?? 1.06, f.z1 ?? f.z0 ?? 1.06, E.inOutSine(p));
  let px = f.px, py = f.py, ox = f.ox ?? f.px, oy = f.oy ?? f.py;
  if (f.from) {
    const g = f.from;
    const k = E.outExpo(prog(t, sh.t0, sh.t0 + (f.snap ?? 0.3)));
    z = lerp(g.z, z, k);
    px = lerp(g.px, px, k);
    py = lerp(g.py, py, k);
    ox = lerp(g.ox ?? g.px, ox, k);
    oy = lerp(g.oy ?? g.py, oy, k);
  }
  return placement(W, H, SW, SH, { z, px, py, ox, oy });
}

// Cuts inside the reel, for the small push on each.
const CUTS = SHOTS.slice(1).map((s) => s.t0).filter((c) => c < END);

/** Apply the push on a cut: a quick 4% zoom settling back, about the centre. */
function punch(ctx, t) {
  for (const c of CUTS) {
    const dt = t - c;
    if (dt >= 0 && dt < 0.3) {
      const k = 1 + 0.04 * (1 - E.outCubic(dt / 0.3));
      ctx.translate(W / 2, H / 2);
      ctx.scale(k, k);
      ctx.translate(-W / 2, -H / 2);
    }
  }
}

function drawPlate(ctx, [z, dx, dy]) {
  ctx.imageSmoothingEnabled = true;
  ctx.imageSmoothingQuality = 'high';
  ctx.drawImage(FOOT.plate, dx, dy, SW * z, SH * z);
}

/** Dark gradients where the type and Instagram's own interface sit. */
function scrims(ctx, top = 0.78, bottom = 0.55) {
  let g = ctx.createLinearGradient(0, 0, 0, 760);
  g.addColorStop(0, `rgba(0,0,0,${top})`);
  g.addColorStop(0.55, `rgba(0,0,0,${top * 0.45})`);
  g.addColorStop(1, 'rgba(0,0,0,0)');
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, W, 760);
  g = ctx.createLinearGradient(0, 1240, 0, H);
  g.addColorStop(0, 'rgba(0,0,0,0)');
  g.addColorStop(1, `rgba(0,0,0,${bottom})`);
  ctx.fillStyle = g;
  ctx.fillRect(0, 1240, W, H - 1240);
}

/** A pen ring, drawn on like a coach's telestrator. Coordinates in source px. */
function penRing(ctx, cx, cy, rx, ry, p, seed, color = C.lemon, width = 8) {
  if (p <= 0) return;
  const pts = [];
  const a0 = -2.4 + hash(seed, 1) * 0.5;
  for (let i = 0; i <= 72; i++) {
    const u = i / 72;
    const a = a0 + u * 1.12 * TAU;
    const wob = 1 + 0.03 * Math.sin(u * 8 + seed) + 0.07 * u;
    pts.push([cx + Math.cos(a) * rx * wob, cy + Math.sin(a) * ry * wob]);
  }
  pen(ctx, pts, E.outCubic(clamp(p)), color, width);
}

/** A pen underline from (x0, y0) to (x1, y1), with a slight bow. */
function penLine(ctx, x0, y0, x1, y1, p, color = C.lemon, width = 8) {
  if (p <= 0) return;
  const pts = [];
  for (let i = 0; i <= 24; i++) {
    const u = i / 24;
    pts.push([lerp(x0, x1, u), lerp(y0, y1, u) + Math.sin(u * Math.PI) * 7]);
  }
  pen(ctx, pts, E.outCubic(clamp(p)), color, width);
}

function pen(ctx, pts, e, color, width) {
  ctx.save();
  ctx.lineCap = 'round';
  ctx.lineJoin = 'round';
  ctx.shadowColor = 'rgba(0,0,0,0.55)';
  ctx.shadowBlur = 14;
  ctx.strokeStyle = color;
  ctx.lineWidth = width;
  strokePart(ctx, pts, 0, e);
  ctx.restore();
}

/** The station label under the top bar: "03/08  THE LID". */
function stationLabel(ctx, t) {
  const i = stationAt(t);
  if (i < 0) return;
  const st = ST[i];
  const lt = t - st.t0;
  const p = prog(lt, 0, 0.35);
  const n = `${String(i + 1).padStart(2, '0')}/08`;
  const y = 330;
  kinetic(ctx, n, SAFE.left, y, T.label(26, C.lemon), p, 0, 'scramble', { t: lt, seed: i + 3 });
  kinetic(ctx, st.name, SAFE.left + 150, y, T.label(26, C.white), prog(lt, 0.08, 0.45), 0, 'scramble', { t: lt, seed: i + 11 });
}

/** A left-aligned headline at the top of the frame, for [t0, t1). */
function head(ctx, t, t0, t1, a, b, { y = 470, size = 124, bottom = false } = {}) {
  if (t < t0 || t >= t1) return;
  const lt = t - t0;
  reelHead(ctx, lt, a, b, SAFE.left, bottom ? 1380 : y, { p0: 0.05, q0: t1 - t0 - 0.4, size, subSize: 46, subColor: '#E8EAED', subWeight: 400 });
}

// --- Scenes -----------------------------------------------------------------------------

/** 0 to 2 s: the finished watch, "how fast?", and the rewind. */
function hook(ctx, t) {
  const q = prog(t, B(1) - 0.2, B(1) - 0.02);
  const lines = ['Box to', 'start line.'];
  lines.forEach((ln, i) => {
    // Already rising on the first frame, which is also the reel's still.
    kinetic(ctx, ln, SAFE.left, 470 + i * 134, T.hero(138, C.white), prog(t, -0.3 + i * 0.1, 0.25 + i * 0.1), q, 'rise');
  });
  kinetic(ctx, 'HOW FAST?', SAFE.left + 2, 710, T.label(38, C.lemon), prog(t, 0.35, 0.75), q, 'scramble', { t, seed: 5 });
}

/** The rewind's texture: the picture tears, bands slip, the scanlines show. */
function rewindFx(ctx, t, place) {
  const lt = t - REWIND;
  const f = Math.floor(lt * 60);
  ctx.save();
  // Wash some of the colour out.
  ctx.globalCompositeOperation = 'saturation';
  ctx.fillStyle = 'rgba(128,128,128,0.55)';
  ctx.fillRect(0, 0, W, H);
  ctx.restore();
  // Bands of the picture slipping sideways.
  const [z, dx, dy] = place;
  for (let i = 0; i < 5; i++) {
    const y = hash(f, i) * H;
    const h = 20 + hash(f + 7, i) * 140;
    const off = (hash(f + 3, i) - 0.5) * 160;
    ctx.save();
    ctx.beginPath();
    ctx.rect(0, y, W, h);
    ctx.clip();
    ctx.drawImage(FOOT.plate, dx + off, dy, SW * z, SH * z);
    ctx.fillStyle = rgba(i % 2 ? C.cyan : C.lemon, 0.08);
    ctx.fillRect(0, y, W, h);
    ctx.restore();
  }
  // A tracking band rolling up the frame.
  const ty = H - ((lt * 2600) % (H + 200));
  const g = ctx.createLinearGradient(0, ty - 60, 0, ty + 60);
  g.addColorStop(0, 'rgba(255,255,255,0)');
  g.addColorStop(0.5, 'rgba(255,255,255,0.22)');
  g.addColorStop(1, 'rgba(255,255,255,0)');
  ctx.fillStyle = g;
  ctx.fillRect(0, ty - 60, W, 120);
  // Scanlines.
  ctx.fillStyle = 'rgba(0,0,0,0.16)';
  for (let y = 0; y < H; y += 6) ctx.fillRect(0, y, W, 2);
}

/**
 * The race clock, large, bottom left: real elapsed time since the box came
 * into shot. It runs back through the rewind and stops at the finish, where
 * the finish's own number takes over.
 */
function raceClock(ctx, t) {
  if (t < REWIND || t >= FINISH) return;
  const rew = t < B(1);
  const x = SAFE.left, y = 1500;
  ctx.save();
  ctx.globalAlpha *= prog(t, REWIND, REWIND + 0.12);
  if (rew) {
    ctx.fillStyle = C.lemon;
    for (const ox of [0, 26]) {
      ctx.beginPath();
      ctx.moveTo(x + ox + 26, y - 104);
      ctx.lineTo(x + ox, y - 91);
      ctx.lineTo(x + ox + 26, y - 78);
      ctx.closePath();
      ctx.fill();
    }
    text(ctx, 'REWIND', x + 70, y - 82, T.label(24, C.lemon));
  } else {
    text(ctx, 'RACE CLOCK', x, y - 82, T.label(24, C.soft));
  }
  text(ctx, clk(clockAt(t)), x - 4, y, { ...T.mono(84, rew ? C.white : C.lemon), weight: 500 });
  ctx.restore();
}

/** The lid, held: the telestrator draws on the box's three promises. */
function lidMarks(ctx, t) {
  const lt = t - FREEZE;
  penLine(ctx, 165, 362, 695, 344, prog(lt, 0.15, 0.4), C.lemon, 8);
  penRing(ctx, 212, 520, 88, 90, prog(t, B(3, 3), B(3, 3) + 0.3), 1);
  penRing(ctx, 436, 510, 100, 94, prog(t, B(4), B(4) + 0.3), 2);
  penRing(ctx, 656, 505, 104, 94, prog(t, B(4, 1), B(4, 1) + 0.3), 3);
  penLine(ctx, 296, 712, 606, 700, prog(t, B(4, 1) + 0.3, B(4, 1) + 0.55), C.lemon, 8);
}

/** The finish: held on Run, the clock stopped, the flash. */
function finishText(ctx, t) {
  const lt = t - FINISH;
  const q = prog(t, BRIDGE - 0.25, BRIDGE - 0.02);
  kinetic(ctx, 'BOX TO START LINE', SAFE.left, 340, T.label(28, C.lemon), prog(lt, 0.1, 0.5), q, 'scramble', { t: lt, seed: 21 });
  const fin = clk(real('B', 61.45)).replace(/\.\d$/, '');
  kinetic(ctx, fin, SAFE.left - 12, 555, fit(ctx, fin, T.num(250, C.white), W - 2 * SAFE.left), prog(lt, 0.02, 0.4), q, 'rise');
  kinetic(ctx, 'REAL ELAPSED TIME', SAFE.left, 615, T.label(22, C.soft), prog(lt, 0.4, 0.8), q, 'fade');
}

function flash(ctx, t, at, amount = 0.85, rate = 5) {
  const dt = t - at;
  if (dt < 0 || dt > 1) return;
  ctx.save();
  ctx.globalAlpha = amount * Math.exp(-dt * rate);
  ctx.fillStyle = C.white;
  ctx.fillRect(0, 0, W, H);
  ctx.restore();
}

/** A split: a tint of lemon and a hairline sweeping across. */
function splitFlash(ctx, t) {
  for (const tb of [B(1), ...ST.slice(1).map((s) => s.t0)]) {
    const dt = t - tb;
    if (dt < 0 || dt > 0.45) continue;
    ctx.save();
    ctx.globalAlpha = 0.1 * Math.exp(-dt * 12);
    ctx.fillStyle = C.lemon;
    ctx.fillRect(0, 0, W, H);
    ctx.restore();
    ctx.fillStyle = rgba(C.white, 0.85 * (1 - clamp(dt / 0.45)));
    ctx.fillRect(E.outExpo(clamp(dt / 0.4)) * W - 2, 0, 3, H);
  }
}

// --- The frame ----------------------------------------------------------------------------

function footageScene(ctx, t) {
  const sh = shotAt(t);
  if (!sh) return;
  const place = placeOf(sh, t);
  ctx.save();
  punch(ctx, t);
  drawPlate(ctx, place);
  if (sh.rewind) rewindFx(ctx, t, place);
  // The telestrator draws in the plate's own pixels.
  if (sh.id === 'lidHold' || sh.id === 'finish') {
    ctx.save();
    ctx.translate(place[1], place[2]);
    ctx.scale(place[0], place[0]);
    if (sh.id === 'lidHold') lidMarks(ctx, t);
    else penRing(ctx, 362, 752, 158, 66, prog(t, FINISH + 0.12, FINISH + 0.45), 9, C.lemon, 9);
    ctx.restore();
  }
  ctx.restore();
  vignette(ctx, 0.4);
  scrims(ctx, sh.id === 'lidHold' || sh.id === 'card' ? 0.5 : 0.78, sh.id === 'strap' || sh.id === 'strapBox' ? 0.75 : 0.55);
}

function draw(ctx, t) {
  if (t < END + 0.3) footageScene(ctx, t);

  // The words, with a soft shadow to lift them off the footage.
  ctx.save();
  ctx.shadowColor = 'rgba(0,0,0,0.55)';
  ctx.shadowBlur = 18;
  if (t < B(1)) hook(ctx, t);
  stationLabel(ctx, t);
  head(ctx, t, B(1) + 0.1, B(2, 2), ['One watch', 'for life.'], null, { size: 132 });
  head(ctx, t, B(4, 2) + 0.05, B(6), 'Built to be opened.', 'Swap the battery. Swap the screen.', { size: 112 });
  head(ctx, t, B(6) + 0.05, B(7), 'USB-C.', 'The cable you already carry.', { size: 132 });
  head(ctx, t, B(7) + 0.05, B(8), 'Swap the strap too.', null, { size: 112 });
  head(ctx, t, B(8) + 0.05, B(10), '10-day battery.', 'Up to 20 hours of GPS.', { size: 124 });
  head(ctx, t, B(10) + 0.05, FINISH - 0.1, 'Dual-frequency GPS.', 'Run · Bike · Hike · Walk · Workout', { size: 104 });
  if (t >= FINISH && t < BRIDGE) finishText(ctx, t);
  head(ctx, t, BRIDGE + 0.1, END - 0.3, ['Open to', 'developers.'], 'So we build apps for it.', { size: 124 });
  raceClock(ctx, t);
  ctx.restore();

  flash(ctx, t, B(1), 0.55, 9);
  flash(ctx, t, FINISH, 0.9, 5);
  splitFlash(ctx, t);

  // The end card, wiped in with the chevron.
  const wp = prog(t, END - 0.3, END + 0.3);
  if (wp > 0) {
    ctx.save();
    ctx.globalAlpha = 1 - prog(t, DURATION - 0.7, DURATION - 0.05);
    chevronWipe(ctx, wp, (c) => {
      c.fillStyle = '#000';
      c.fillRect(0, 0, W, H);
      if (t > END) reelEnd(c, t - END, { app: 'all', accent: C.lemon, accent2: C.cyan, name: '', tagline: ['Unboxed.', 'Now we race.'], tagColor: C.lemon });
    }, { dir: 'right', color: C.lemon, band: 64, band2: 12, color2: C.white });
    ctx.restore();
  }

  // The top bar: the eight stations, and the race clock.
  const i = stationAt(t);
  const clock = clockAt(t);
  const right = t >= FINISH ? `FINISHED · ${clk(clock)}` : 'UNA WATCH';
  const rightColor = t >= FINISH ? C.white : C.soft;
  const progress = t >= FINISH ? 1 : i >= 0 ? (i + prog(t, ST[i].t0, ST[i].t1)) / 8 : 0;
  reelHud(ctx, t, {
    app: 'UNBOXED', accent: C.lemon, alpha: 1 - prog(t, END - 0.3, END),
    right, rightColor, progress, ticks: [1, 2, 3, 4, 5, 6, 7].map((k) => k / 8),
  });
}

// --- Footage hooks (see render.mjs) ----------------------------------------------------------

async function setup() {
  if (!FOOT.available()) {
    throw new Error(`The unboxing reel needs its two clips in ${CLIPS} (they are not in git).`);
  }
  // Grab the held frames and the rewind's stills once, a few at a time.
  const jobs = [...REW_STILLS, ...SHOTS.filter((s) => s.hold).map((s) => ({ src: s.src, s: s.a }))];
  for (let i = 0; i < jobs.length; i += 6) {
    await Promise.all(jobs.slice(i, i + 6).map((j) => FOOT.grabAsync(j.src, j.s)));
  }
}

async function prepare(t) {
  const sh = shotAt(t);
  if (!sh) return;
  if (sh.rewind) {
    const r = rewindAt(t);
    await FOOT.still(r.src, r.s);
  } else if (sh.hold) {
    await FOOT.still(sh.src, sh.a);
  } else {
    await FOOT.moving(sh.id, sh.src, srcTime(sh, t), sh.b);
    FOOT.retire(sh.id);
  }
}

// --- Cues for the soundtrack ----------------------------------------------------------------

function cues() {
  const q = [];
  q.push({ t: 0, type: 'hit', v: 0.9 });
  q.push({ t: REWIND, type: 'rewind', until: B(1) });
  q.push({ t: B(1), type: 'go' });
  for (const st of ST.slice(1)) q.push({ t: st.t0, type: 'split' });
  for (const s of SHOTS) {
    if (!s.rewind && !s.hold && s.t0 > B(1) && (s.b - s.a) / (s.t1 - s.t0) > 2) q.push({ t: s.t0, type: 'swish' });
  }
  q.push({ t: FREEZE, type: 'freeze' });
  [[FREEZE + 0.15, 0], [B(3, 3), 1], [B(4), 2], [B(4, 1), 3], [B(4, 1) + 0.3, 4]].forEach(([tt, k]) => q.push({ t: tt, type: 'pen', k }));
  q.push({ t: B(9), type: 'boot' });
  q.push({ t: FINISH, type: 'finish' }, { t: FINISH + 0.12, type: 'pen', k: 5 });
  q.push({ t: END - 0.3, type: 'whoosh' });
  q.push({ t: END + 0.35 + 0.9, type: 'logo' });
  return q.sort((a, b) => a.t - b.t);
}

/** The shots, for the score to lay the clips' own sound under the music. */
function edl() {
  return {
    sources: SOURCES,
    shots: SHOTS.filter((s) => !s.rewind).map(({ id, src, a, b, t0, t1, ease, hold, nat }) => ({ id, src, a, b: hold ? a : b, t0, t1, ease: ease || 0, hold: !!hold, nat: nat || 0 })),
    // Where the clips' own sound leads and the music steps back.
    duck: [[3.8, 5.0], [B(9) - 0.1, B(9) + 0.6]],
  };
}

export default {
  id: 'unboxing-reel',
  title: 'HybridX, unboxing the UNA Watch (reel)',
  frame: [W, H],
  duration: DURATION,
  bpm: 120,
  tune: 'film',
  crf: 19,
  sections: [{ name: 'hook', bar: 0 }, { name: 'race', bar: 1 }, { name: 'finish', bar: 12 }, { name: 'open', bar: 13 }, { name: 'end', bar: 14 }],
  setup,
  prepare,
  close: () => FOOT.close(),
  draw,
  cues,
  edl,
};
