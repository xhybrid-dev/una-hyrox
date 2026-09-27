// HybridX Trail: "Follow the line".
//
// 96 BPM, 60 bars (150 s). A topographic map at night, one orchid line, and
// a white dot that follows it. Calm and continuous: the camera drifts, lines
// draw themselves, and the one sharp moment is going off course.
//
// Trail is at its T0 probe: the watch screens here are concept designs for
// the brief's v1 features (see lib/ui-trail.mjs), and the end card says
// "Coming soon".

import { Path2D } from '@napi-rs/canvas';
import { W, H, clamp, lerp, prog, ep, E, hash, tempo, TAU, noise2 } from '../lib/core.mjs';
import { text, kinetic, measure, rgba, light, roundRect, strokePart, pointAt, polyLength } from '../lib/gfx.mjs';
import { C, T, sting, hud, lineWipe, endCard } from '../lib/brand.mjs';
import { watch, setWatchModel, buttonPos, ripple, buzz } from '../lib/watch.mjs';
import { headline, pressAt } from '../lib/film.mjs';
import { CONTOURS, ROUTE, routeAt, routeSlice, routePoint, height } from '../lib/terrain.mjs';
import * as TR from '../lib/ui-trail.mjs';

const TM = tempo(96);
const B = (b, beats = 0) => TM.at(b, beats);
const DURATION = B(60);
const LOCK = B(6);
const CH = {
  gpx: B(9), read: B(14), usb: B(19), list: B(22), zoom: B(25), heading: B(31),
  off: B(35), along: B(43), run: B(47), finale: B(51), end: B(57),
};
const PINK = C.pink;

// --- The map ----------------------------------------------------------------------

function buildPath(arr) {
  const p = new Path2D();
  for (let i = 0; i < arr.length; i += 4) {
    p.moveTo(arr[i], arr[i + 1]);
    p.lineTo(arr[i + 2], arr[i + 3]);
  }
  return p;
}
const MINOR = buildPath(CONTOURS.minor);
const INDEX = buildPath(CONTOURS.index);
const ROUTE_PATH = (() => {
  const p = new Path2D();
  ROUTE.pts.forEach(([x, y], i) => (i ? p.lineTo(x, y) : p.moveTo(x, y)));
  return p;
})();

/** cam: { x, y (world metres at the screen point sx, sy), k (px per m), rot, sx, sy } */
function applyCam(ctx, cam) {
  ctx.translate(cam.sx ?? W / 2, cam.sy ?? H / 2);
  ctx.rotate(cam.rot || 0);
  ctx.scale(cam.k, -cam.k);
  ctx.translate(-cam.x, -cam.y);
}

function toScreen(cam, x, y) {
  const dx = (x - cam.x) * cam.k, dy = -(y - cam.y) * cam.k;
  const c = Math.cos(cam.rot || 0), s = Math.sin(cam.rot || 0);
  return [(cam.sx ?? W / 2) + dx * c - dy * s, (cam.sy ?? H / 2) + dx * s + dy * c];
}

function contours(ctx, cam, alpha = 1) {
  if (alpha <= 0) return;
  ctx.save();
  applyCam(ctx, cam);
  ctx.globalAlpha *= alpha;
  ctx.lineWidth = 1.1 / cam.k;
  ctx.strokeStyle = 'rgba(0,170,85,0.30)';
  ctx.stroke(MINOR);
  ctx.lineWidth = 1.5 / cam.k;
  ctx.strokeStyle = 'rgba(0,170,85,0.58)';
  ctx.stroke(INDEX);
  ctx.restore();
}

/** The route (or part of it), glowing. */
function routeLine(ctx, cam, { from = 0, to = 1, alpha = 1, width = 5, glow = 18, color = PINK, dim = false } = {}) {
  if (alpha <= 0 || to <= from) return;
  ctx.save();
  ctx.globalAlpha *= alpha;
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';
  if (from <= 0 && to >= 1) {
    applyCam(ctx, cam);
    ctx.lineWidth = width / cam.k;
    ctx.strokeStyle = color;
    if (glow) {
      ctx.shadowColor = rgba(color, dim ? 0.3 : 0.8);
      ctx.shadowBlur = glow;
    }
    ctx.stroke(ROUTE_PATH);
  } else {
    const pts = routeSlice(from * ROUTE.length, to * ROUTE.length).map(([x, y]) => toScreen(cam, x, y));
    ctx.lineWidth = width;
    ctx.strokeStyle = color;
    if (glow) {
      ctx.shadowColor = rgba(color, 0.8);
      ctx.shadowBlur = glow;
    }
    ctx.beginPath();
    pts.forEach(([x, y], i) => (i ? ctx.lineTo(x, y) : ctx.moveTo(x, y)));
    ctx.stroke();
  }
  ctx.restore();
}

function dot(ctx, x, y, t, color = C.white, r = 9, pulse = true) {
  if (pulse) {
    const ph = (t * 0.9) % 1;
    ctx.save();
    ctx.strokeStyle = rgba(color, 0.7 * (1 - ph));
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.arc(x, y, r + ph * 34, 0, TAU);
    ctx.stroke();
    ctx.restore();
  }
  light(ctx, x, y, r * 6, rgba(color, 0.35), 1);
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.arc(x, y, r, 0, TAU);
  ctx.fill();
}

/** A soft shade behind a headline that sits over the map. */
function shade(ctx, x, y, w, h, a = 0.85) {
  const g = ctx.createRadialGradient(x, y, 0, x, y, Math.max(w, h));
  g.addColorStop(0, `rgba(0,0,0,${a})`);
  g.addColorStop(0.55, `rgba(0,0,0,${a * 0.75})`);
  g.addColorStop(1, 'rgba(0,0,0,0)');
  ctx.save();
  ctx.translate(x, y);
  ctx.scale(1, h / w);
  ctx.translate(-x, -y);
  ctx.fillStyle = g;
  ctx.fillRect(x - w, y - w, w * 2, w * 2);
  ctx.restore();
}

// --- Scenes ---------------------------------------------------------------------------

const START = routeAt(0);

function coldOpen(ctx, t) {
  // A slow drift over the start, then a pull back to the whole loop.
  const pull = ep(t, 11.0, 14.6, E.inOutCubic);
  const cam = {
    x: lerp(START.x + 300 + t * 18, 900, pull), y: lerp(START.y + 200 + t * 10, -300, pull),
    k: lerp(0.62, 0.105, pull), rot: lerp(0.22 - t * 0.008, 0.0, pull),
  };
  const mapA = prog(t, 0.2, 3.5);
  contours(ctx, cam, mapA * (1 - prog(t, LOCK - 1.2, LOCK - 0.5) * 0.6));
  // The line draws itself from the dot, then all the way round.
  const draw = ep(t, 9.4, 14.2, E.inOutSine);
  routeLine(ctx, cam, { from: 0, to: draw, width: lerp(6, 4, pull), glow: 20 });
  const [dx, dy] = toScreen(cam, START.x, START.y);
  if (t > 1.8) dot(ctx, dx, dy, t, C.white, lerp(9, 7, pull), true);
  // "No map tiles. No turn-by-turn. No rerouting."
  const q = prog(t, 9.2, 9.8);
  ['NO MAP TILES.', 'NO TURN-BY-TURN.', 'NO REROUTING.'].forEach((s, i) => {
    kinetic(ctx, s, 150, 440 + i * 46, T.label(24, C.soft), prog(t, B(1) + i * 2.5 - 1.5, B(1) + i * 2.5 - 0.5), q, 'type', { t });
  });
  kinetic(ctx, 'Just the line.', 150, 560, T.hero(140), prog(t, B(4), B(4) + 0.7), prog(t, LOCK - 1.6, LOCK - 1.0), 'rise');
}

function title(ctx, t) {
  const cam = { x: 900, y: -300, k: 0.105, rot: (t - LOCK) * 0.01 };
  contours(ctx, cam, 0.4 * (1 - prog(t, B(8, 2), B(9))));
  routeLine(ctx, cam, { alpha: 0.35 * (1 - prog(t, B(8, 2), B(9))), width: 4, glow: 0 });
  const st = t - (LOCK - 0.95);
  const out = prog(t, B(8, 2), B(9));
  sting(ctx, st, { accent: PINK, accent2: PINK, name: 'Trail', sub: 'BREADCRUMB NAVIGATION FOR UNA WATCH', out, h: 190 });
  // A GPS ping on the lock.
  const lt = t - LOCK;
  if (lt > 0) ripple(ctx, W / 2 - 330, H / 2, lt, PINK, { rings: 4, spread: 0.22, speed: 520, life: 1.6, width: 2 });
}

// GPX text for the code panel: the route's own points, in fell country.
const LAT0 = 54.45, LON0 = -3.05;
const GPX_LINES = (() => {
  const out = [
    '<?xml version="1.0" encoding="UTF-8"?>',
    '<gpx version="1.1" creator="planner">',
    '  <trk>',
    '    <name>Ridge loop</name>',
    '    <trkseg>',
  ];
  for (let i = 0; i < 44; i++) {
    const [x, y] = ROUTE.pts[i * 3];
    const lat = LAT0 + y / 111320;
    const lon = LON0 + x / (111320 * Math.cos((LAT0 * Math.PI) / 180));
    out.push(`      <trkpt lat="${lat.toFixed(5)}" lon="${lon.toFixed(5)}">`);
    out.push(`        <ele>${height(x, y).toFixed(1)}</ele>`);
    out.push('      </trkpt>');
  }
  return out;
})();

function gpx(ctx, t) {
  const lt = t - CH.gpx;
  const dur = CH.read - CH.gpx;
  const out = prog(lt, dur - 0.5, dur);
  const cam = { x: 900, y: -300, k: 0.105, rot: 0 };
  contours(ctx, cam, 0.18);
  headline(ctx, lt, 'Plan it anywhere.', 'OS Maps, Komoot, Strava: anything that exports a GPX file.', 150, 290, { p0: 0.2, q0: dur - 0.5, size: 88, width: 700 });
  // The code panel: the file scrolls past a read head.
  const px = 1010, py = 150, pw = 780, ph = 780;
  const inP = ep(lt, 0.4, 1.3, E.outExpo);
  ctx.save();
  ctx.globalAlpha *= inP * (1 - out);
  ctx.fillStyle = '#0a0d10';
  roundRect(ctx, px, py + (1 - inP) * 60, pw, ph, 18);
  ctx.fill();
  ctx.strokeStyle = '#1c2228';
  ctx.lineWidth = 1.5;
  ctx.stroke();
  text(ctx, 'ridge-loop.gpx', px + 28, py + 44, T.mono(20, C.white));
  text(ctx, '1,420 POINTS', px + pw - 28, py + 44, { ...T.label(14, C.mute), align: 'right' });
  ctx.fillStyle = '#1c2228';
  ctx.fillRect(px, py + 66, pw, 1);
  ctx.beginPath();
  ctx.rect(px, py + 70, pw, ph - 80);
  ctx.clip();
  const lineH = 32;
  const scroll = Math.max(0, lt - 1.5) * 88;
  const headY = py + 420;
  GPX_LINES.forEach((ln, i) => {
    const y = py + 110 + i * lineH - scroll;
    if (y < py + 60 || y > py + ph + 20) return;
    const isPt = ln.includes('<trkpt');
    const near = Math.abs(y - headY) < lineH / 2;
    const col = isPt ? (y < headY ? PINK : '#8a6f8f') : ln.includes('ele') ? '#6b7680' : '#9aa3ad';
    text(ctx, ln, px + 28, y, { ...T.mono(19, col), alpha: near ? 1 : 0.85 });
  });
  // The read head.
  ctx.fillStyle = rgba(PINK, 0.12);
  ctx.fillRect(px, headY - lineH / 2 - 6, pw, lineH);
  ctx.fillStyle = PINK;
  ctx.fillRect(px, headY - lineH / 2 - 6, 3, lineH);
  ctx.restore();
  // Points leave the file and build the route, bottom left.
  const ox = 440, oy = 760, os = 330;
  const n = Math.floor(clamp((lt - 1.6) / (dur - 2.6)) * ROUTE.pts.length);
  if (n > 1) {
    ctx.save();
    ctx.globalAlpha *= 1 - out;
    TR.overview(ctx, ox, oy, os, 4, n / ROUTE.pts.length, PINK);
    ctx.restore();
  }
  kinetic(ctx, 'TRKPT · LAT · LON · ELE', 154, 600, T.label(16, C.mute), prog(lt, 1.4, 2.2), out, 'scramble', { t: lt });
}

function read(ctx, t) {
  const lt = t - CH.read;
  const dur = CH.usb - CH.read;
  const out = prog(lt, dur - 0.5, dur);
  headline(ctx, lt, 'Read as exported.', 'No converter. The watch reads the file a chunk at a time, in fixed memory, and thins any route to fit.', 150, 250, { p0: 0.2, q0: dur - 0.5, size: 104, width: 820 });
  // The test loop from the probe: 5,001 points round a 1.6 km circle.
  const cx = 1420, cy = 620, R = 300;
  const N = 5001;
  const appear = ep(lt, 1.0, 3.6, E.inOutCubic);
  const thin = ep(lt, 5.0, 7.0, E.inOutCubic);
  ctx.save();
  ctx.globalAlpha *= 1 - out;
  for (let i = 0; i < N; i++) {
    const f = i / (N - 1);
    if (f > appear) break;
    const keep = i % 5 === 0;
    const a = keep ? 1 : 1 - thin;
    if (a <= 0.01) continue;
    const ang = f * TAU;
    const rr = R + (keep ? 0 : 0);
    const x = cx + Math.sin(ang) * rr, y = cy - Math.cos(ang) * rr;
    ctx.fillStyle = keep && thin > 0.5 ? PINK : rgba('#FFFFFF', 0.75 * a);
    const s = keep ? lerp(1.6, 3.4, thin) : 1.4;
    ctx.fillRect(x - s / 2, y - s / 2, s, s);
  }
  ctx.restore();
  // The numbers, as the probe's simulator run reported them (NOTES.md, T0).
  const nA = prog(lt, 1.2, 1.9) * (1 - out);
  text(ctx, 'TEST LOOP · 1.2 MB', cx, cy - R - 60, { ...T.label(16, C.soft), align: 'center', alpha: nA });
  const pts = thin > 0.5 ? '1,001' : '5,001';
  text(ctx, pts, cx, cy + 22, { ...T.num(110, thin > 0.5 ? PINK : C.white), align: 'center', alpha: nA });
  text(ctx, thin > 0.5 ? 'POINTS KEPT · 10 M APART' : 'POINTS IN THE FILE', cx, cy + 64, { ...T.label(15, C.soft), align: 'center', alpha: nA });
  kinetic(ctx, '10.05 KM · 80 M ASCENT, FROM EVERY POINT', cx, cy + R + 80, { ...T.label(16, C.white), align: 'center' }, prog(lt, 7.4, 8.2), out, 'scramble', { t: lt });
  // The chunk: a byte window sliding along a stream, bottom left.
  const sy = 760;
  const sA = ep(lt, 0.8, 1.6, E.outCubic) * (1 - out);
  if (sA > 0) {
    ctx.save();
    ctx.globalAlpha *= sA;
    const stream = GPX_LINES.slice(5, 30).join(' ').replace(/\s+/g, ' ');
    const offset = (lt * 160) % 1200;
    ctx.save();
    ctx.beginPath();
    ctx.rect(150, sy - 30, 720, 44);
    ctx.clip();
    text(ctx, stream, 150 - offset, sy, T.mono(18, '#5c6770'));
    ctx.restore();
    // The window.
    ctx.strokeStyle = PINK;
    ctx.lineWidth = 2;
    roundRect(ctx, 420, sy - 34, 180, 52, 8);
    ctx.stroke();
    text(ctx, 'CHUNK', 510, sy + 50, { ...T.label(14, PINK), align: 'center' });
    ctx.restore();
  }
}

function usb(ctx, t) {
  const lt = t - CH.usb;
  const dur = CH.list - CH.usb;
  const out = prog(lt, dur - 0.45, dur);
  headline(ctx, lt, 'Copy it over USB.', 'Into the app’s Routes folder, the night before.', 150, 300, { p0: 0.2, q0: dur - 0.45, size: 104, width: 760 });
  const tree = [
    ['Apps/', 0, C.soft], ['HybridXTrail/', 1, C.soft], ['Routes/', 2, C.white],
    ['ridge-loop.gpx', 3, PINK], ['coast-path.gpx', 3, C.mute], ['sunday-long.gpx', 3, C.mute],
  ];
  tree.forEach(([s, lvl, col], i) => {
    const t0 = 0.8 + i * 0.25 + (i === 3 ? 1.2 : 0);
    const x = 1060 + lvl * 44, y = 400 + i * 60;
    kinetic(ctx, s, x, y, T.mono(30, col), prog(lt, t0, t0 + 0.6), out, 'type', { t: lt });
    if (i === 3 && lt > t0 + 0.6) {
      kinetic(ctx, '← NEW', x + 330, y, T.label(16, PINK), prog(lt, t0 + 0.6, t0 + 1.1), out, 'fade');
    }
  });
  // A copy bar.
  const bar = ep(lt, 2.2, 3.4, E.inOutCubic);
  ctx.save();
  ctx.globalAlpha *= 1 - out;
  ctx.fillStyle = C.rule;
  ctx.fillRect(1060, 790, 560, 3);
  ctx.fillStyle = PINK;
  ctx.fillRect(1060, 790, 560 * bar, 3);
  ctx.restore();
  kinetic(ctx, bar < 1 ? 'COPYING…' : 'EJECT SAFELY · UNPLUG · OPEN THE APP', 1060, 830, T.label(15, C.soft), prog(lt, 2.2, 2.6), out, 'fade');
}

function list(ctx, t) {
  const lt = t - CH.list;
  const dur = CH.zoom - CH.list;
  const out = prog(lt, dur - 0.45, dur);
  headline(ctx, lt, 'Pick a route.', 'Its name, distance and ascent come straight from the file.', 150, 320, { p0: 0.2, q0: dur - 0.45, size: 104, width: 680 });
  const wIn = ep(lt, 0.2, 1.3, E.outExpo);
  // Scroll down twice, back up twice, then R1.
  const steps = [[1.8, 1], [2.6, 2], [3.6, 1], [4.4, 0]];
  let sel = 0, shift = 0;
  for (const [ts, v] of steps) {
    if (lt >= ts + 0.35) sel = v;
    else if (lt >= ts) {
      const prev = sel;
      shift = (v - prev) * E.inOutCubic((lt - ts) / 0.35);
      break;
    }
  }
  const pressT = CH.list + 5.2;
  watch(ctx, {
    cx: lerp(W + 400, 1400, wIn), cy: 540, d: 560, rot: lerp(0.25, 0.02, wIn),
    screen: (s) => (lt < 5.3 ? TR.routeList(s, { sel: Math.floor(sel + Math.min(0, shift)), shift: shift < 0 ? 1 + shift : shift }) : TR.routeSummary(s, { p: prog(lt, 5.3, 6.6) })),
    bloom: 0.5, alpha: 1 - out, press: { r1: pressAt(t - pressT), l2: pressAt(t - (CH.list + 1.8)) + pressAt(t - (CH.list + 2.6)), l1: pressAt(t - (CH.list + 3.6)) + pressAt(t - (CH.list + 4.4)) },
  });
}

const ZOOM_STEPS = [
  { r: 200, label: '200 m' }, { r: 500, label: '500 m' }, { r: 1000, label: '1 km' }, { r: 2000, label: '2 km' }, { r: 3400, label: 'Route' },
];
const S_ZOOM = 3150; // on the switchbacks

function zoom(ctx, t) {
  const lt = t - CH.zoom;
  const dur = CH.heading - CH.zoom;
  const out = prog(lt, dur - 0.5, dur);
  // Zoom level (fractional) over time: a step every 2.2 s.
  const zt = clamp((lt - 2.0) / 2.2, 0, 4);
  const zi = Math.floor(zt);
  const zf = E.inOutCubic(clamp((zt - zi) / 0.35));
  const r = zi >= 4 ? ZOOM_STEPS[4].r : Math.exp(lerp(Math.log(ZOOM_STEPS[zi].r), Math.log(ZOOM_STEPS[Math.min(4, zi + 1)].r), zf));
  const labelNow = ZOOM_STEPS[Math.min(4, zi + (zf > 0.5 ? 1 : 0))].label;
  const here = routeAt(S_ZOOM + lt * 12);
  // The big map zooms with the watch: its view ring stays 330 px across.
  const ringR = 330;
  const cam = { x: here.x, y: here.y, k: ringR / r, rot: 0, sx: 600, sy: 630 };
  contours(ctx, cam, 0.9 * (1 - out));
  routeLine(ctx, cam, { alpha: 1 - out, width: 5, glow: 14 });
  const [ux, uy] = toScreen(cam, here.x, here.y);
  dot(ctx, ux, uy, t, C.white, 8);
  ctx.save();
  ctx.globalAlpha *= (1 - out) * ep(lt, 1.0, 1.8, E.outCubic);
  ctx.strokeStyle = rgba(C.white, 0.8);
  ctx.lineWidth = 2;
  ctx.setLineDash([10, 10]);
  ctx.beginPath();
  ctx.arc(ux, uy, ringR, 0, TAU);
  ctx.stroke();
  ctx.setLineDash([]);
  ctx.restore();
  kinetic(ctx, labelNow.toUpperCase(), ux + ringR + 24, uy + 8, T.label(24, PINK), prog(lt, 1.2, 1.8), out, 'fade');
  // Soft edges on the map so it doesn't fight the watch.
  const g = ctx.createLinearGradient(900, 0, 1180, 0);
  g.addColorStop(0, 'rgba(0,0,0,0)');
  g.addColorStop(1, 'rgba(0,0,0,1)');
  ctx.fillStyle = g;
  ctx.fillRect(900, 0, W - 900, H);
  shade(ctx, 420, 130, 620, 200, 0.8 * (1 - out));
  headline(ctx, lt, 'Zoom to suit.', 'Fixed scales from 200 m to the whole route, on one button.', 150, 170, { p0: 0.2, q0: dur - 0.5, size: 80, width: 900 });
  // The watch.
  const wIn = ep(lt, 0.3, 1.4, E.outExpo);
  const pressTimes = [0, 1, 2, 3].map((i) => CH.zoom + 2.0 + i * 2.2 - 0.05);
  const press = pressTimes.reduce((a, pt) => a + pressAt(t - pt), 0);
  watch(ctx, {
    cx: lerp(W + 400, 1470, wIn), cy: 540, d: 580, rot: lerp(0.25, 0, wIn),
    screen: (s) => TR.mapScreen(s, { at: S_ZOOM + lt * 12, radius: r, scale: labelNow === 'Route' ? '2 km' : labelNow, toGo: (ROUTE.length - S_ZOOM - lt * 12) / 1000 }),
    bloom: 0.45, alpha: 1 - out, press: { l1: press },
  });
}

const S_HEAD = 2520; // a hairpin on the climb

function heading(ctx, t) {
  const lt = t - CH.heading;
  const dur = CH.off - CH.heading;
  const out = prog(lt, dur - 0.5, dur);
  const s = S_HEAD + lt * 26;
  const here = routeAt(s);
  const headingUp = lt > 4.6;
  const hu = ep(lt, 4.4, 5.2, E.inOutCubic);
  headline(ctx, lt, ['Heading up,', 'or north up.'], 'Turn and the map turns with you: the compass when you stop, GPS when you run.', 150, 300, { p0: 0.2, q0: dur - 0.5, size: 96, width: 640 });
  // The watch with a compass ring that turns with the map.
  const wIn = ep(lt, 0.2, 1.2, E.outExpo);
  const wo = { cx: lerp(W + 400, 1320, wIn), cy: 540, d: 520, rot: 0 };
  const mapRot = lerp(0, here.heading, hu);
  ctx.save();
  ctx.globalAlpha *= (1 - out) * wIn;
  ctx.translate(wo.cx, wo.cy);
  ctx.rotate(-mapRot);
  const R0 = 400;
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
    ctx.translate(Math.sin(a) * (R0 + 52), -Math.cos(a) * (R0 + 52));
    ctx.rotate(mapRot);
    text(ctx, l, 0, 10, { ...T.label(28, l === 'N' ? C.red : C.white), align: 'center' });
    ctx.restore();
  });
  ctx.restore();
  watch(ctx, {
    ...wo,
    screen: (sc) => TR.mapScreen(sc, { at: s, radius: 300, scale: '200 m', headingUp, heading: headingUp ? here.heading * hu + (1 - hu) * 0 : here.heading }),
    bloom: 0.45, alpha: 1 - out,
  });
  const mode = headingUp ? 'HEADING UP' : 'NORTH UP';
  kinetic(ctx, mode, wo.cx, wo.cy + 470, { ...T.label(20, PINK), align: 'center' }, prog(lt, 1.0, 1.6), out, 'fade');
}

// The junction for going off course, in local metres around a centre point.
const OFF = (() => {
  const c = [1200, 2200];
  // The route comes up from the south and turns east at the junction; the
  // wrong fork carries straight on north.
  const route = [];
  for (let i = 0; i <= 60; i++) route.push([c[0] + Math.sin(i * 0.08) * 12, c[1] - 600 + i * 10]);
  for (let i = 1; i <= 80; i++) {
    const a = Math.min(1, i / 14) * (Math.PI / 2);
    const last = route[route.length - 1];
    route.push([last[0] + Math.sin(a) * 10, last[1] + Math.cos(a) * 10]);
  }
  const fork = [];
  const j = route[60];
  for (let i = 0; i <= 50; i++) fork.push([j[0] - i * 2.2 + Math.sin(i * 0.2) * 6, j[1] + i * 10]);
  return { c, route, fork, j };
})();

/** The runner's path: up the route, straight on up the fork, then back. */
const RUNPATH = (() => {
  const r = OFF.route, f = OFF.fork;
  const forward = [...r.slice(0, 61), ...f.slice(1, 10)];
  const back = [...f.slice(0, 10)].reverse();
  const path = [...forward, ...back.slice(1), ...r.slice(61)];
  const d1 = polyLength(forward);
  return { path, L: polyLength(path), d1, d2: d1 + polyLength(back) };
})();

function offRunner(lt) {
  const { path, L, d1, d2 } = RUNPATH;
  // Walk: up and straight on, a pause, back, and onward.
  let s;
  if (lt < 2.0) s = 0;
  else if (lt < 9.5) s = lerp(0, d1, E.inOutSine(prog(lt, 2.0, 9.5)));
  else if (lt < 10.4) s = d1;
  else if (lt < 14.6) s = lerp(d1, d2, E.inOutSine(prog(lt, 10.4, 14.6)));
  else s = lerp(d2, L, E.outSine(prog(lt, 14.6, 20.5)));
  const p = pointAt(path, s / L);
  return { x: p.x, y: p.y, s, path, L };
}

function distToRoute(x, y) {
  let best = Infinity, bx = 0, by = 0;
  const r = OFF.route;
  for (let i = 1; i < r.length; i++) {
    const [ax, ay] = r[i - 1], [cx, cy] = r[i];
    const vx = cx - ax, vy = cy - ay;
    const tt = clamp(((x - ax) * vx + (y - ay) * vy) / (vx * vx + vy * vy));
    const px = ax + vx * tt, py = ay + vy * tt;
    const d = Math.hypot(x - px, y - py);
    if (d < best) {
      best = d; bx = px; by = py;
    }
  }
  return { d: best, px: bx, py: by };
}

/** When the watch calls it: off past 50 m; back once well inside again. */
const OFF_TIMES = (() => {
  let offAt = 99, backAt = 99;
  for (let x = 0; x < 20; x += 0.02) {
    const rr = offRunner(x);
    const d = distToRoute(rr.x, rr.y).d;
    if (offAt === 99 && d > 50) offAt = x;
    if (offAt !== 99 && backAt === 99 && d < 30) backAt = x;
  }
  return { offAt, backAt };
})();

function offCourse(ctx, t) {
  const lt = t - CH.off;
  const dur = CH.along - CH.off;
  const out = prog(lt, dur - 0.5, dur);
  const run = offRunner(lt);
  const dr = distToRoute(run.x, run.y);
  const { offAt, backAt } = OFF_TIMES;
  const isOff = lt >= offAt && lt < backAt;
  const cam = { x: OFF.j[0] + 40 + lt * 1.2, y: OFF.j[1] + 30, k: lerp(2.3, 2.6, lt / dur), rot: 0.05, sx: 700, sy: 600 };
  contours(ctx, cam, (1 - out) * 1.0);
  const drawIn = ep(lt, 0.2, 1.8, E.inOutCubic);
  const S = (p) => toScreen(cam, p[0], p[1]);
  // The corridor: 50 m either side of the line.
  ctx.save();
  ctx.globalAlpha *= (1 - out) * drawIn;
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';
  const corridor = (w, col) => {
    ctx.strokeStyle = col;
    ctx.lineWidth = w;
    ctx.beginPath();
    OFF.route.forEach((p, i) => {
      const [X, Y] = S(p);
      if (i) ctx.lineTo(X, Y);
      else ctx.moveTo(X, Y);
    });
    ctx.stroke();
  };
  const edgeCol = isOff ? C.amber : PINK;
  corridor(100 * cam.k + 4, rgba(edgeCol, isOff ? 0.5 : 0.28));
  corridor(100 * cam.k, 'rgba(0,0,0,0.9)');
  corridor(100 * cam.k, rgba(edgeCol, 0.07));
  // The wrong fork: a grey dashed path.
  ctx.setLineDash([10, 12]);
  ctx.strokeStyle = rgba(C.gray, 0.6);
  ctx.lineWidth = 3;
  ctx.beginPath();
  OFF.fork.forEach((p, i) => {
    const [X, Y] = S(p);
    if (i) ctx.lineTo(X, Y);
    else ctx.moveTo(X, Y);
  });
  ctx.stroke();
  ctx.setLineDash([]);
  ctx.restore();
  // The line itself.
  const rpts = OFF.route.map(S);
  ctx.save();
  ctx.globalAlpha *= 1 - out;
  ctx.strokeStyle = PINK;
  ctx.lineWidth = 7;
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';
  ctx.shadowColor = rgba(PINK, 0.8);
  ctx.shadowBlur = 16;
  strokePart(ctx, rpts, 0, drawIn);
  ctx.restore();
  // The corridor's label.
  const lab = S([OFF.route[40][0] + 58, OFF.route[40][1]]);
  kinetic(ctx, '50 M', lab[0] + 10, lab[1], T.label(16, edgeCol), prog(lt, 1.6, 2.2), out, 'fade');
  // Breadcrumbs of where the runner has really been.
  const crumbs = Math.floor(run.s / 12);
  ctx.save();
  ctx.globalAlpha *= 1 - out;
  for (let i = 0; i < crumbs; i++) {
    const p = pointAt(run.path, (i * 12) / run.L);
    const [X, Y] = S([p.x, p.y]);
    ctx.fillStyle = rgba(C.white, 0.5);
    ctx.fillRect(X - 1.5, Y - 1.5, 3, 3);
  }
  ctx.restore();
  // Distance to the line, when it matters.
  const [rx, ry] = S([run.x, run.y]);
  if (dr.d > 12 && lt > 3) {
    const [qx, qy] = S([dr.px, dr.py]);
    ctx.save();
    ctx.globalAlpha *= 1 - out;
    ctx.strokeStyle = isOff ? C.amber : C.soft;
    ctx.lineWidth = 2;
    ctx.setLineDash([6, 6]);
    ctx.beginPath();
    ctx.moveTo(rx, ry);
    ctx.lineTo(qx, qy);
    ctx.stroke();
    ctx.setLineDash([]);
    ctx.restore();
    text(ctx, `${Math.round(dr.d)} M`, (rx + qx) / 2 - 18, (ry + qy) / 2 - 12, { ...T.label(22, isOff ? C.amber : C.white), align: 'right', alpha: 1 - out });
  }
  // The runner, and the buzz.
  const col = isOff ? C.amber : C.white;
  dot(ctx, rx, ry, t, col, 10, !isOff);
  if (lt > offAt) ripple(ctx, rx, ry, lt - offAt, C.amber, { rings: 4, spread: 0.12, speed: 420, life: 0.9, width: 3 });
  if (lt > backAt) ripple(ctx, rx, ry, lt - backAt, PINK, { rings: 2, spread: 0.12, speed: 380, life: 0.8, width: 2 });
  shade(ctx, 600, 120, 760, 190, 0.8 * (1 - out));
  headline(ctx, lt, 'Wander off? You’ll feel it.', 'A buzz and a banner when you’re more than 50 m from the line, and another when you’re back.', 150, 170, { p0: 0.2, q0: dur - 0.5, size: 80, width: 900 });
  // The watch, right, with its banner.
  const wIn = ep(lt, 0.4, 1.4, E.outExpo);
  const bz1 = buzz(lt - offAt, 9, 0.5), bz2 = buzz(lt - backAt, 5, 0.3);
  const bannerP = isOff ? ep(lt, offAt, offAt + 0.25, E.outBack) : lt >= backAt && lt < backAt + 2.6 ? ep(lt, backAt, backAt + 0.25, E.outBack) * (1 - prog(lt, backAt + 2.2, backAt + 2.6)) : 0;
  watch(ctx, {
    cx: lerp(W + 400, 1540, wIn) + bz1[0] + bz2[0], cy: 600 + bz1[1] + bz2[1], d: 470, rot: 0,
    screen: (s) => {
      const at = 4200 + lt * 8;
      TR.mapScreen(s, { at, radius: 200, scale: '200 m', headingUp: false, banner: isOff ? 'off' : lt >= backAt && lt < backAt + 2.6 ? 'back' : null, bannerP, offBy: dr.d, toGo: (ROUTE.length - at) / 1000 });
    },
    bloom: 0.5, alpha: 1 - out,
  });
}

// A figure of eight, for "measured along the line".
const EIGHT = (() => {
  const pts = [];
  for (let i = 0; i <= 720; i++) {
    const a = (i / 720) * TAU;
    pts.push([Math.sin(a) * 2400, Math.sin(2 * a) * 1100]);
  }
  return pts;
})();

function along(ctx, t) {
  const lt = t - CH.along;
  const dur = CH.run - CH.along;
  const out = prog(lt, dur - 0.5, dur);
  const cam = { x: 0, y: 0, k: 0.215, rot: -0.2, sx: 1330, sy: 580 };
  contours(ctx, { ...cam, x: 2500, y: -1800 }, 0.5 * (1 - out));
  const pts = EIGHT.map(([x, y]) => toScreen(cam, x, y));
  const L = polyLength(pts);
  // The runner on the second pass through the crossing.
  const f = lerp(0.36, 0.53, ep(lt, 0.8, dur - 1.0, E.inOutSine));
  const drawIn = ep(lt, 0.2, 1.6, E.inOutCubic);
  ctx.save();
  ctx.globalAlpha *= 1 - out;
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';
  ctx.strokeStyle = '#AA00AA';
  ctx.lineWidth = 6;
  strokePart(ctx, pts, 0, Math.min(f, drawIn));
  ctx.strokeStyle = PINK;
  ctx.shadowColor = rgba(PINK, 0.7);
  ctx.shadowBlur = 14;
  if (drawIn > f) strokePart(ctx, pts, f, drawIn);
  ctx.restore();
  const p = pointAt(pts, f);
  // The crossing: the same spot on two legs. The tracker keeps to yours.
  if (lt > 2.4) {
    const ghost = pointAt(pts, 0.0);
    const a = prog(lt, 2.4, 3.0) * (1 - out);
    ctx.save();
    ctx.globalAlpha *= a;
    ctx.strokeStyle = C.mute;
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.arc(ghost.x, ghost.y, 22, 0, TAU);
    ctx.stroke();
    ctx.restore();
  }
  dot(ctx, p.x, p.y, t, C.white, 10);
  // At the crossing: two legs meet. Which one are you on?
  const cx0 = pointAt(pts, 0.5);
  const near = clamp(1 - Math.abs(f - 0.5) / 0.06) * (1 - out);
  if (near > 0) {
    text(ctx, '7.1 KM · YOU', cx0.x + 40, cx0.y - 60, { ...T.label(26, C.white), alpha: near });
    text(ctx, '0.0 KM', cx0.x - 40, cx0.y + 74, { ...T.label(26, C.mute), align: 'right', alpha: near });
    ctx.save();
    ctx.globalAlpha *= near;
    ctx.strokeStyle = C.mute;
    ctx.lineWidth = 2;
    const w0 = measure(ctx, '0.0 KM', T.label(26, C.mute));
    ctx.beginPath();
    ctx.moveTo(cx0.x - 40 - w0 - 6, cx0.y + 65);
    ctx.lineTo(cx0.x - 34, cx0.y + 65);
    ctx.stroke();
    ctx.restore();
  }
  const done = f * 14.2, togo = 14.2 - done;
  headline(ctx, lt, ['Measured along', 'the line.'], 'Distance done and to go follow the route, so a loop that crosses itself never jumps to the wrong leg.', 150, 260, { p0: 0.2, q0: dur - 0.5, size: 88, width: 600 });
  kinetic(ctx, `${done.toFixed(1)} KM DONE`, 154, 760, T.label(24, '#D070D0'), prog(lt, 1.6, 2.2), out, 'fade');
  kinetic(ctx, `${togo.toFixed(1)} KM TO GO`, 154, 800, T.label(24, PINK), prog(lt, 1.8, 2.4), out, 'fade');
}

function runScene(ctx, t) {
  const lt = t - CH.run;
  const dur = CH.finale - CH.run;
  const out = prog(lt, dur - 0.5, dur);
  headline(ctx, lt, 'It’s a run, too.', 'Time, distance, pace, heart rate and laps, saved as a normal activity for Strava and Garmin Connect.', 150, 280, { p0: 0.2, q0: dur - 0.5, size: 100, width: 700 });
  const stats = [['DISTANCE', '12.84 km'], ['PACE', '5:36 /km'], ['TIME', '1:11:52'], ['AVG HR', '152 bpm']];
  stats.forEach(([k, v], i) => {
    const x = 154 + (i % 2) * 360, y = 640 + Math.floor(i / 2) * 130;
    kinetic(ctx, k, x, y, T.label(16, C.mute), prog(lt, 1.4 + i * 0.12, 2.0 + i * 0.12), out, 'fade');
    kinetic(ctx, v, x - 4, y + 60, T.num(64, i === 0 ? PINK : C.white), prog(lt, 1.5 + i * 0.12, 2.1 + i * 0.12), out, 'rise');
  });
  const wIn = ep(lt, 0.3, 1.3, E.outExpo);
  const faceMap = lt < 3.2 || lt > 7.6;
  const lapT = CH.run + 5.2;
  const secs = 4312 + Math.floor(lt * 3);
  watch(ctx, {
    cx: lerp(W + 400, 1440, wIn), cy: 540, d: 560, rot: lerp(0.25, 0.02, wIn),
    screen: (s) => (faceMap ? TR.mapScreen(s, { at: 12840 + lt * 3, radius: 500, scale: '500 m', toGo: (ROUTE.length - 12840) / 1000 })
      : TR.dataFace(s, { dist: 12.84, pace: '5:36', time: `1:${String(Math.floor(secs / 60) % 60).padStart(2, '0')}:${String(secs % 60).padStart(2, '0')}`, hr: 152 + Math.round(Math.sin(lt * 2) * 2), lap: lt > 5.3 ? 13 : 12 })),
    bloom: 0.45, alpha: 1 - out, press: { r2: pressAt(t - lapT) },
  });
}

function finale(ctx, t) {
  const lt = t - CH.finale;
  const dur = CH.end - CH.finale;
  const out = prog(lt, dur - 0.7, dur);
  const cam = { x: lerp(900, 700, lt / dur), y: -300, k: lerp(0.16, 0.12, E.outSine(lt / dur)), rot: lerp(-0.12, 0.06, lt / dur) };
  contours(ctx, cam, 0.9 * (1 - out));
  // The whole line, dim, and the runner finishing it, bright behind them.
  const f = lerp(0.72, 1, ep(lt, 0.0, 8.5, E.inOutSine));
  routeLine(ctx, cam, { alpha: 0.35 * (1 - out), width: 4, glow: 0 });
  routeLine(ctx, cam, { from: 0, to: f, alpha: 1 - out, width: 5, glow: 22 });
  const here = routeAt(f * ROUTE.length);
  const [x, y] = toScreen(cam, here.x, here.y);
  if (f < 1) dot(ctx, x, y, t, C.white, 9);
  else {
    const [sx, sy] = toScreen(cam, START.x, START.y);
    dot(ctx, sx, sy, t, PINK, 10, false);
    ripple(ctx, sx, sy, lt - 8.5, PINK, { rings: 4, spread: 0.2, speed: 460, life: 1.6, width: 2 });
  }
  kinetic(ctx, 'RIDGE LOOP · 14.2 KM · COMPLETE', W / 2, H - 170, { ...T.label(20, PINK), align: 'center' }, prog(lt, 8.6, 9.4), out, 'scramble', { t: lt });
  kinetic(ctx, 'Follow the line.', W / 2, 250, { ...T.hero(150), align: 'center' }, prog(lt, 9.8, 10.6), out, 'rise');
}

// --- The film ---------------------------------------------------------------------

const SCENES = [
  [0, LOCK - 0.95, coldOpen], [LOCK - 0.95, CH.gpx, title], [CH.gpx, CH.read, gpx], [CH.read, CH.usb, read],
  [CH.usb, CH.list, usb], [CH.list, CH.zoom, list], [CH.zoom, CH.heading, zoom], [CH.heading, CH.off, heading],
  [CH.off, CH.along, offCourse], [CH.along, CH.run, along], [CH.run, CH.finale, runScene], [CH.finale, CH.end, finale],
];
const WIPES = [CH.gpx, CH.list, CH.off, CH.finale];

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
    let dissolved = false;
    for (const tb of [CH.read, CH.usb, CH.zoom, CH.heading, CH.along, CH.run]) {
      const p = prog(t, tb - 0.35, tb + 0.35);
      if (p > 0 && p < 1) {
        sceneAt(tb - 0.001)(ctx, Math.min(t, tb - 0.001));
        ctx.save();
        ctx.globalAlpha = E.inOutSine(p);
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, W, H);
        sceneAt(tb + 0.001)(ctx, t);
        ctx.restore();
        dissolved = true;
        break;
      }
    }
    if (!dissolved) {
      const f = sceneAt(t);
      if (f) f(ctx, t);
    }
  }
  if (t >= CH.end - 0.05) {
    ctx.save();
    ctx.globalAlpha = 1 - prog(t, DURATION - 0.9, DURATION - 0.05);
    endCard(ctx, t - CH.end, { app: 'trail', accent: PINK, accent2: PINK, name: 'Trail', note: 'COMING SOON' });
    ctx.restore();
  }
  const a = prog(t, CH.gpx, CH.gpx + 0.6) * (1 - prog(t, CH.end - 0.8, CH.end - 0.2));
  if (a > 0) {
    const ticks = [CH.read, CH.usb, CH.list, CH.zoom, CH.heading, CH.off, CH.along, CH.run, CH.finale].map((x) => (x - CH.gpx) / (CH.end - CH.gpx));
    const along = t < CH.zoom ? 0 : t < CH.heading ? S_ZOOM : t < CH.off ? S_HEAD : t < CH.along ? 4200 : t < CH.run ? 7400 : t < CH.finale ? 12840 : ROUTE.length * lerp(0.72, 1, ep(t - CH.finale, 0, 8.5, E.inOutSine));
    hud(ctx, t, {
      app: 'TRAIL', accent: PINK, alpha: a,
      topRight: 'RIDGE LOOP · 14.2 KM', topRightColor: PINK,
      progress: (t - CH.gpx) / (CH.end - CH.gpx), ticks,
      bottomRight: `${(along / 1000).toFixed(2)} KM`, bottomRightLabel: 'ALONG THE LINE',
    });
  }
}

function cues() {
  const q = [];
  ['NO MAP TILES.', 'NO TURN-BY-TURN.', 'NO REROUTING.'].forEach((_, i) => q.push({ t: B(1) + i * 2.5 - 1.5, type: 'type', n: 14 }));
  q.push({ t: B(4), type: 'line' });
  q.push({ t: LOCK - 0.95, type: 'draw' }, { t: LOCK, type: 'lock' });
  for (const tb of WIPES) q.push({ t: tb - 0.3, type: 'whoosh' });
  q.push({ t: CH.read + 5.0, type: 'thin', until: CH.read + 7.0 });
  q.push({ t: CH.usb + 2.2, type: 'copy', until: CH.usb + 3.4 });
  [1.8, 2.6, 3.6, 4.4].forEach((x) => q.push({ t: CH.list + x, type: 'click' }));
  q.push({ t: CH.list + 5.2, type: 'select' });
  [0, 1, 2, 3].forEach((i) => q.push({ t: CH.zoom + 2.0 + i * 2.2 - 0.05, type: 'zoom', i }));
  q.push({ t: CH.heading + 4.6, type: 'toggle' });
  const { offAt, backAt } = OFF_TIMES;
  q.push({ t: CH.off + offAt, type: 'offCourse' }, { t: CH.off + backAt, type: 'backOnCourse' });
  q.push({ t: CH.off + 2.0, type: 'tension', until: CH.off + offAt });
  q.push({ t: CH.run + 5.2 - 0.05, type: 'lap' });
  q.push({ t: CH.finale + 8.5, type: 'complete' });
  q.push({ t: CH.finale + 9.8, type: 'line' });
  q.push({ t: CH.end + 0.95, type: 'logo' });
  return q.sort((a, b) => a.t - b.t);
}

// Pieces the portrait reel reuses.
export { contours, routeLine, dot, shade, toScreen, OFF, offRunner, distToRoute, START };

export default {
  id: 'trail',
  title: 'HybridX Trail',
  duration: DURATION,
  bpm: 96,
  sections: [
    { name: 'intro', bar: 0 }, { name: 'title', bar: 6 }, { name: 'gpx', bar: 9 }, { name: 'read', bar: 14 }, { name: 'usb', bar: 19 },
    { name: 'list', bar: 22 }, { name: 'zoom', bar: 25 }, { name: 'heading', bar: 31 }, { name: 'off', bar: 35 },
    { name: 'along', bar: 43 }, { name: 'run', bar: 47 }, { name: 'finale', bar: 51 }, { name: 'end', bar: 57 },
  ],
  draw,
  cues,
};
