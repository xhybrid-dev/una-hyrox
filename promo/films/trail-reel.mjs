// HybridX Trail, the reel: "Follow the line" in 35 seconds, portrait.
//
// 96 BPM, 14 bars (35 s), 1080 x 1920 for Instagram Reels and adverts. The
// map turned on its side to fill the frame, one orchid line, and the film's
// one sharp moment, going off course, at the centre of the reel.
//
// Trail is at its T0 probe: the screens are the film's concept designs, and
// the end card says "Coming soon".

import { clamp, lerp, prog, ep, E, tempo, TAU } from '../lib/core.mjs';
import { text, kinetic, rgba, strokePart, pointAt } from '../lib/gfx.mjs';
import { C, T, sting, lineWipe } from '../lib/brand.mjs';
import { watch, setWatchModel, ripple, buzz } from '../lib/watch.mjs';
import { pressAt } from '../lib/film.mjs';
import { ROUTE, routeAt } from '../lib/terrain.mjs';
import * as TR from '../lib/ui-trail.mjs';
import { contours, routeLine, dot, shade, toScreen, OFF, offRunner, distToRoute, START } from './trail.mjs';
import { RW as W, RH as H, SAFE, reelHud, reelHead, reelEnd } from '../lib/reel.mjs';

const TM = tempo(96);
const B = (b, beats = 0) => TM.at(b, beats);
const LOCK = B(2);
const CH = { load: B(3.5), follow: B(5.5), off: B(7.5), finale: B(11), end: B(12.5) };
const END = CH.end + 0.35;
const DURATION = B(14);
const PINK = C.pink;
const WIPES = [CH.load, CH.off, CH.finale];
const SOFT = [CH.follow];

// The whole loop, turned a quarter so it runs up the portrait frame.
const LOOP = { x: 900, y: -300, k: 0.17, rot: -Math.PI / 2, sx: W / 2, sy: 930 };

// --- Scenes ----------------------------------------------------------------------

/** 0 to 4 s: the line draws itself across the fells, from the first frame. */
function hook(ctx, t) {
  const pull = ep(t, 0.4, 4.2, E.inOutCubic);
  const cam = {
    x: lerp(START.x + 250, LOOP.x, pull), y: lerp(START.y + 150, LOOP.y, pull), k: lerp(0.55, LOOP.k, pull),
    rot: lerp(-0.25, LOOP.rot, pull), sx: W / 2, sy: lerp(1000, LOOP.sy, pull),
  };
  const fade = prog(t, LOCK - 1.25, LOCK - 0.7);
  contours(ctx, cam, (0.55 + 0.45 * prog(t, 0, 0.8)) * (1 - fade * 0.6));
  routeLine(ctx, cam, { from: 0, to: ep(t, 0.0, 3.9, E.inOutSine), width: lerp(7, 5, pull), glow: 22, alpha: 1 - fade * 0.7 });
  const [dx, dy] = toScreen(cam, START.x, START.y);
  dot(ctx, dx, dy, t, C.white, 10, true);
  shade(ctx, W / 2, 430, 620, 260, 0.85 * (1 - fade));
  const q = prog(t, LOCK - 1.2, LOCK - 0.95);
  kinetic(ctx, 'Just', SAFE.left, 420, T.hero(150), prog(t, -0.3, 0.2), q, 'rise');
  kinetic(ctx, 'the line.', SAFE.left, 570, T.hero(150, PINK), prog(t, 0.1, 0.55), q, 'rise');
  ['NO MAP TILES.', 'NO TURN-BY-TURN.', 'NO REROUTING.'].forEach((s, i) => {
    kinetic(ctx, s, SAFE.left + 4, 660 + i * 40, T.label(24, C.soft), prog(t, 0.8 + i * 0.55, 1.4 + i * 0.55), q, 'type', { t });
  });
}

function title(ctx, t) {
  const out = prog(t, CH.load - 0.9, CH.load - 0.1);
  const bg = 0.35 * (1 - out);
  contours(ctx, { ...LOOP, rot: LOOP.rot + (t - LOCK) * 0.01 }, bg);
  routeLine(ctx, { ...LOOP, rot: LOOP.rot + (t - LOCK) * 0.01 }, { alpha: bg, width: 4, glow: 0 });
  sting(ctx, t - (LOCK - 0.95), { accent: PINK, accent2: PINK, name: 'Trail', sub: 'ROUTE NAVIGATION FOR UNA WATCH', out, h: 160, cy: 900 });
  const lt = t - LOCK;
  if (lt > 0) ripple(ctx, W / 2 - 300, 900, lt, PINK, { rings: 4, spread: 0.22, speed: 520, life: 1.6, width: 2 });
}

/** Load a route: a GPX over USB, picked on the watch. */
function load(ctx, t) {
  const lt = t - CH.load;
  contours(ctx, LOOP, 0.16);
  reelHead(ctx, lt, 'Load a route.', 'Any GPX file, from OS Maps, Komoot or Strava, copied over USB.', SAFE.left, 400, { p0: 0.3, size: 128 });
  const wIn = ep(lt, 0.3, 1.2, E.outExpo);
  const steps = [[1.5, 1], [2.1, 2], [2.7, 1], [3.2, 0]];
  let sel = 0, shift = 0;
  for (const [ts, v] of steps) {
    if (lt >= ts + 0.3) sel = v;
    else if (lt >= ts) {
      shift = (v - sel) * E.inOutCubic((lt - ts) / 0.3);
      break;
    }
  }
  const pressT = 3.75;
  watch(ctx, {
    cx: W / 2, cy: lerp(H + 400, 1050, wIn), d: 480, rot: lerp(0.3, 0, wIn),
    screen: (s) => (lt < pressT + 0.1 ? TR.routeList(s, { sel: Math.floor(sel + Math.min(0, shift)), shift: shift < 0 ? 1 + shift : shift }) : TR.routeSummary(s, { p: prog(lt, pressT + 0.1, pressT + 1.1) })),
    bloom: 0.5, press: { r1: pressAt(lt - pressT), l2: pressAt(lt - 1.5) + pressAt(lt - 2.1), l1: pressAt(lt - 2.7) + pressAt(lt - 3.2) },
  });
  kinetic(ctx, 'Routes/ridge-loop.gpx', SAFE.left, 1500, T.mono(30, PINK), prog(lt, 0.9, 1.9), 0, 'type', { t: lt });
}

/** Follow it: the map turns with you, the compass round the watch. */
function follow(ctx, t) {
  const lt = t - CH.follow;
  const s = 2520 + lt * 30;
  const here = routeAt(s);
  const hu = ep(lt, 2.2, 3.0, E.inOutCubic);
  reelHead(ctx, lt, 'Follow the line.', 'Heading up or north up: turn, and the map turns with you.', SAFE.left, 400, { p0: 0.2, size: 124 });
  const wo = { cx: W / 2, cy: 1080, d: 460, rot: 0 };
  const mapRot = lerp(0, here.heading, hu);
  const wIn = ep(lt, 0.0, 0.8, E.outExpo);
  ctx.save();
  ctx.globalAlpha *= wIn;
  ctx.translate(wo.cx, wo.cy);
  ctx.rotate(-mapRot);
  const R0 = 360;
  for (let d = 0; d < 360; d += 5) {
    const a = (d * Math.PI) / 180;
    const big = d % 30 === 0;
    ctx.strokeStyle = d === 0 ? C.red : big ? rgba(C.white, 0.9) : rgba(C.white, 0.45);
    ctx.lineWidth = d === 0 ? 5 : big ? 3 : 1.5;
    ctx.beginPath();
    ctx.moveTo(Math.sin(a) * R0, -Math.cos(a) * R0);
    ctx.lineTo(Math.sin(a) * (R0 + (big ? 22 : 12)), -Math.cos(a) * (R0 + (big ? 22 : 12)));
    ctx.stroke();
  }
  [['N', 0], ['E', 90], ['S', 180], ['W', 270]].forEach(([l, d]) => {
    const a = (d * Math.PI) / 180;
    ctx.save();
    ctx.translate(Math.sin(a) * (R0 + 54), -Math.cos(a) * (R0 + 54));
    ctx.rotate(mapRot);
    text(ctx, l, 0, 11, { ...T.label(30, l === 'N' ? C.red : C.white), align: 'center' });
    ctx.restore();
  });
  ctx.restore();
  const headingUp = lt > 2.4;
  watch(ctx, {
    ...wo, alpha: wIn,
    screen: (sc) => TR.mapScreen(sc, { at: s, radius: 300, scale: '200 m', headingUp, heading: headingUp ? here.heading * hu : here.heading }),
    bloom: 0.45, press: { r2: pressAt(lt - 2.2) },
  });
  kinetic(ctx, headingUp ? 'HEADING UP' : 'NORTH UP', W / 2, 1530, { ...T.label(26, PINK), align: 'center' }, prog(lt, 0.6, 1.1), 0, 'fade');
}

// Off course: the runner's clock runs 1.45 times the film's, so the watch
// calls it about 5 s into the chapter and the line is found again at about 7.
const RUN = (lt) => 2.0 + lt * 1.45;
const OFF_AT = (() => {
  let offAt = 99, backAt = 99;
  for (let x = 0; x < 12; x += 0.01) {
    const rr = offRunner(RUN(x));
    const d = distToRoute(rr.x, rr.y).d;
    if (offAt === 99 && d > 50) offAt = x;
    if (offAt !== 99 && backAt === 99 && d < 30) backAt = x;
  }
  return { offAt, backAt };
})();

function offCourse(ctx, t) {
  const lt = t - CH.off;
  const run = offRunner(RUN(lt));
  const dr = distToRoute(run.x, run.y);
  const { offAt, backAt } = OFF_AT;
  const isOff = lt >= offAt && lt < backAt;
  const cam = { x: OFF.j[0] + 20, y: OFF.j[1] - 40 + lt * 6, k: lerp(2.2, 2.45, lt / 8), rot: 0.04, sx: W / 2, sy: 960 };
  contours(ctx, cam, 1);
  const drawIn = ep(lt, 0.1, 1.4, E.inOutCubic);
  const S = (p) => toScreen(cam, p[0], p[1]);
  ctx.save();
  ctx.globalAlpha *= drawIn;
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';
  const corridor = (w, col) => {
    ctx.strokeStyle = col;
    ctx.lineWidth = w;
    ctx.beginPath();
    OFF.route.forEach((p, i) => (i ? ctx.lineTo(...S(p)) : ctx.moveTo(...S(p))));
    ctx.stroke();
  };
  const edgeCol = isOff ? C.amber : PINK;
  corridor(100 * cam.k + 4, rgba(edgeCol, isOff ? 0.5 : 0.28));
  corridor(100 * cam.k, 'rgba(0,0,0,0.9)');
  corridor(100 * cam.k, rgba(edgeCol, 0.07));
  ctx.setLineDash([10, 12]);
  ctx.strokeStyle = rgba(C.gray, 0.6);
  ctx.lineWidth = 3;
  ctx.beginPath();
  OFF.fork.forEach((p, i) => (i ? ctx.lineTo(...S(p)) : ctx.moveTo(...S(p))));
  ctx.stroke();
  ctx.setLineDash([]);
  ctx.restore();
  ctx.save();
  ctx.strokeStyle = PINK;
  ctx.lineWidth = 7;
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';
  ctx.shadowColor = rgba(PINK, 0.8);
  ctx.shadowBlur = 16;
  strokePart(ctx, OFF.route.map(S), 0, drawIn);
  ctx.restore();
  // Breadcrumbs, the gap to the line, the runner.
  const crumbs = Math.floor(run.s / 12);
  for (let i = 0; i < crumbs; i++) {
    const p = pointAt(run.path, (i * 12) / run.L);
    const [X, Y] = S([p.x, p.y]);
    ctx.fillStyle = rgba(C.white, 0.5);
    ctx.fillRect(X - 1.5, Y - 1.5, 3, 3);
  }
  const [rx, ry] = S([run.x, run.y]);
  if (dr.d > 12) {
    const [qx, qy] = S([dr.px, dr.py]);
    ctx.save();
    ctx.strokeStyle = isOff ? C.amber : C.soft;
    ctx.lineWidth = 2;
    ctx.setLineDash([6, 6]);
    ctx.beginPath();
    ctx.moveTo(rx, ry);
    ctx.lineTo(qx, qy);
    ctx.stroke();
    ctx.restore();
    text(ctx, `${Math.round(dr.d)} M`, rx + 26, ry + 10, T.label(28, isOff ? C.amber : C.white));
  }
  dot(ctx, rx, ry, t, isOff ? C.amber : C.white, 11, !isOff);
  if (lt > offAt) ripple(ctx, rx, ry, lt - offAt, C.amber, { rings: 4, spread: 0.12, speed: 420, life: 0.9, width: 3 });
  if (lt > backAt) ripple(ctx, rx, ry, lt - backAt, PINK, { rings: 2, spread: 0.12, speed: 380, life: 0.8, width: 2 });
  // Words at the top, over a shade.
  shade(ctx, W / 2, 440, 700, 280, 0.9);
  const pre = lt < offAt + 0.2;
  const q1 = prog(lt, offAt - 0.1, offAt + 0.2);
  kinetic(ctx, 'Wander off?', SAFE.left, 420, T.hero(132), prog(lt, 0.2, 0.7), q1, 'rise');
  kinetic(ctx, 'OFF COURSE', SAFE.left + 4, 420, T.hero(132, C.amber), prog(lt, offAt, offAt + 0.2), prog(lt, backAt - 0.15, backAt + 0.1), 'rise');
  kinetic(ctx, 'Back on course.', SAFE.left, 420, T.hero(120, PINK), prog(lt, backAt, backAt + 0.35), 0, 'rise');
  kinetic(ctx, 'A buzz past 50 m from the line, and another when you’re back.', SAFE.left + 4, 500, { ...T.sub(36, C.soft) }, prog(lt, 0.5, 1.1), pre ? 0 : prog(lt, offAt, offAt + 0.2), 'fade');
  // The watch, low, with its banner.
  const bz1 = buzz(lt - offAt, 10, 0.5), bz2 = buzz(lt - backAt, 6, 0.3);
  const bannerP = isOff ? ep(lt, offAt, offAt + 0.25, E.outBack) : lt >= backAt && lt < backAt + 2.2 ? ep(lt, backAt, backAt + 0.25, E.outBack) : 0;
  watch(ctx, {
    cx: W / 2 + bz1[0] + bz2[0], cy: lerp(H + 400, 1400, ep(lt, 0.3, 1.2, E.outExpo)) + bz1[1] + bz2[1], d: 360, rot: 0,
    screen: (s) => {
      const at = 4200 + lt * 8;
      TR.mapScreen(s, { at, radius: 200, scale: '200 m', headingUp: false, banner: isOff ? 'off' : lt >= backAt ? 'back' : null, bannerP, offBy: dr.d, toGo: (ROUTE.length - at) / 1000 });
    },
    bloom: 0.5,
  });
}

/** The loop, finished. */
function finale(ctx, t) {
  const lt = t - CH.finale;
  const cam = { ...LOOP, rot: LOOP.rot + lt * 0.015, k: LOOP.k * (1 + lt * 0.02) };
  contours(ctx, cam, 0.9);
  const f = lerp(0.8, 1, ep(lt, 0.0, 2.0, E.inOutSine));
  routeLine(ctx, cam, { alpha: 0.35, width: 4, glow: 0 });
  routeLine(ctx, cam, { from: 0, to: f, width: 6, glow: 24 });
  const here = routeAt(f * ROUTE.length);
  if (f < 1) dot(ctx, ...toScreen(cam, here.x, here.y), t, C.white, 10);
  else {
    const [sx, sy] = toScreen(cam, START.x, START.y);
    dot(ctx, sx, sy, t, PINK, 12, false);
    ripple(ctx, sx, sy, lt - 2.0, PINK, { rings: 4, spread: 0.2, speed: 460, life: 1.6, width: 2 });
  }
  shade(ctx, W / 2, 1440, 620, 160, 0.85);
  kinetic(ctx, 'RIDGE LOOP · 14.2 KM', SAFE.left, 1420, T.label(28, PINK), prog(lt, 2.0, 2.6), 0, 'scramble', { t: lt });
  kinetic(ctx, 'SAVED AS A RUN · STRAVA · GARMIN CONNECT', SAFE.left, 1470, T.label(20, C.soft), prog(lt, 2.3, 2.9), 0, 'scramble', { t: lt + 1 });
}

// --- The reel --------------------------------------------------------------------------

const SCENES = [
  [0, LOCK - 0.95, hook], [LOCK - 0.95, CH.load, title], [CH.load, CH.follow, load], [CH.follow, CH.off, follow],
  [CH.off, CH.finale, offCourse], [CH.finale, CH.end, finale],
];

function sceneAt(t) {
  for (const [a, b, f] of SCENES) if (t >= a && t < b) return f;
  return null;
}

function draw(ctx, t) {
  setWatchModel('white');
  let done = false;
  for (const tb of WIPES) {
    const p = prog(t, tb - 0.45, tb + 0.55);
    if (p > 0 && p < 1) {
      sceneAt(tb - 0.001)(ctx, Math.min(t, tb - 0.001));
      const next = sceneAt(tb + 0.001);
      lineWipe(ctx, p, (c) => {
        c.fillStyle = '#000';
        c.fillRect(0, 0, W, H);
        next(c, t);
      }, { color: PINK, seed: tb });
      done = true;
      break;
    }
  }
  if (!done) {
    for (const tb of SOFT) {
      const p = prog(t, tb - 0.35, tb + 0.35);
      if (p > 0 && p < 1) {
        sceneAt(tb - 0.001)(ctx, Math.min(t, tb - 0.001));
        ctx.save();
        ctx.globalAlpha = E.inOutSine(p);
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, W, H);
        sceneAt(tb + 0.001)(ctx, t);
        ctx.restore();
        done = true;
      }
    }
  }
  if (!done) {
    const f = sceneAt(t);
    if (f) f(ctx, t);
  }
  if (t >= CH.end - 0.3) {
    const et = t - CH.end;
    ctx.save();
    ctx.globalAlpha = prog(et, -0.3, 0.2) * (1 - prog(t, DURATION - 0.8, DURATION - 0.05));
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, W, H);
    reelEnd(ctx, et, { app: 'trail', accent: PINK, accent2: PINK, name: 'Trail', note: 'COMING SOON', tagline: ['Follow', 'the line.'] });
    ctx.restore();
  }
  const a = (1 - prog(t, LOCK - 1.15, LOCK - 0.9)) + prog(t, CH.load, CH.load + 0.5) - prog(t, CH.end - 0.5, CH.end);
  reelHud(ctx, t, {
    app: 'TRAIL', accent: PINK, alpha: clamp(a),
    right: 'RIDGE LOOP · 14.2 KM', rightColor: PINK,
    progress: t / CH.end, ticks: [LOCK, CH.load, CH.follow, CH.off, CH.finale].map((x) => x / CH.end),
  });
}

function cues() {
  const q = [];
  q.push({ t: 0, type: 'line' });
  ['NO MAP TILES.', 'NO TURN-BY-TURN.', 'NO REROUTING.'].forEach((_, i) => q.push({ t: 0.8 + i * 0.55, type: 'type', n: 10 }));
  q.push({ t: LOCK - 0.95, type: 'draw' }, { t: LOCK, type: 'lock' });
  for (const tb of WIPES) q.push({ t: tb - 0.3, type: 'whoosh' });
  [1.5, 2.1, 2.7, 3.2].forEach((x) => q.push({ t: CH.load + x, type: 'click' }));
  q.push({ t: CH.load + 3.75, type: 'select' });
  q.push({ t: CH.follow + 2.2, type: 'toggle' });
  const { offAt, backAt } = OFF_AT;
  q.push({ t: CH.off + 1.2, type: 'tension', until: CH.off + offAt });
  q.push({ t: CH.off + offAt, type: 'offCourse' }, { t: CH.off + backAt, type: 'backOnCourse' });
  q.push({ t: CH.finale + 2.0, type: 'complete' });
  q.push({ t: END + 0.9, type: 'logo' });
  return q.sort((a, b) => a.t - b.t);
}

export default {
  id: 'trail-reel',
  title: 'HybridX Trail (reel)',
  frame: [W, H],
  duration: DURATION,
  bpm: 96,
  sections: [{ name: 'intro', bar: 0 }, { name: 'title', bar: 2 }, { name: 'load', bar: 3.5 }, { name: 'follow', bar: 5.5 }, { name: 'off', bar: 7.5 }, { name: 'finale', bar: 11 }, { name: 'end', bar: 12.5 }],
  draw,
  cues,
  offTimes: OFF_AT,
};
