// Terrain for the Trail film: a height field, its contour lines (marching
// squares), and a loop route across it. Units are metres; x east, y north.
// Everything is computed once, deterministically, when the module loads.

import { fbm, noise2, clamp, lerp, TAU } from './core.mjs';

export const SIZE = 16000; // the map is 16 km square, centred on (0, 0)
const CELL = 50; // contour grid spacing, m
const N = SIZE / CELL;

/** Height in metres at (x, y). */
export function height(x, y) {
  const u = x / 3800 + 11.3, v = y / 3800 + 4.7;
  let h = fbm(u, v, 5, 2.0, 0.5); // about -0.7..0.7
  // A long ridge running north-east.
  const ridge = Math.exp(-Math.pow((x * 0.6 - y * 0.8 - 400) / 2600, 2));
  h = 330 + h * 520 + ridge * 280 + noise2(x / 900 + 3, y / 900 - 7) * 35;
  return h;
}

// --- Contours -----------------------------------------------------------------

const LEVEL_STEP = 25;

function buildContours() {
  const H = new Float32Array((N + 1) * (N + 1));
  let lo = Infinity, hi = -Infinity;
  for (let j = 0; j <= N; j++) {
    for (let i = 0; i <= N; i++) {
      const v = height(-SIZE / 2 + i * CELL, -SIZE / 2 + j * CELL);
      H[j * (N + 1) + i] = v;
      if (v < lo) lo = v;
      if (v > hi) hi = v;
    }
  }
  const minor = [];
  const index = [];
  const at = (i, j) => H[j * (N + 1) + i];
  const X = (i) => -SIZE / 2 + i * CELL;
  const Y = (j) => -SIZE / 2 + j * CELL;
  for (let level = Math.ceil(lo / LEVEL_STEP) * LEVEL_STEP; level <= hi; level += LEVEL_STEP) {
    const out = level % 100 === 0 ? index : minor;
    for (let j = 0; j < N; j++) {
      for (let i = 0; i < N; i++) {
        const a = at(i, j), b = at(i + 1, j), c = at(i + 1, j + 1), d = at(i, j + 1);
        const code = (a > level ? 1 : 0) | (b > level ? 2 : 0) | (c > level ? 4 : 0) | (d > level ? 8 : 0);
        if (code === 0 || code === 15) continue;
        const e = (p, q) => (level - p) / (q - p);
        const top = [X(i) + e(a, b) * CELL, Y(j)];
        const right = [X(i + 1), Y(j) + e(b, c) * CELL];
        const bottom = [X(i) + e(d, c) * CELL, Y(j + 1)];
        const left = [X(i), Y(j) + e(a, d) * CELL];
        const seg = (p, q) => out.push(p[0], p[1], q[0], q[1]);
        switch (code) {
          case 1: case 14: seg(left, top); break;
          case 2: case 13: seg(top, right); break;
          case 3: case 12: seg(left, right); break;
          case 4: case 11: seg(right, bottom); break;
          case 5: seg(left, top); seg(right, bottom); break;
          case 6: case 9: seg(top, bottom); break;
          case 7: case 8: seg(left, bottom); break;
          case 10: seg(top, right); seg(left, bottom); break;
          default: break;
        }
      }
    }
  }
  return { minor: Float32Array.from(minor), index: Float32Array.from(index) };
}

export const CONTOURS = buildContours();

// --- The route --------------------------------------------------------------------

/** Resample a polyline at a fixed spacing. */
function resample(pts, step) {
  const out = [pts[0]];
  let carry = 0;
  for (let i = 1; i < pts.length; i++) {
    let [x0, y0] = pts[i - 1];
    const [x1, y1] = pts[i];
    let seg = Math.hypot(x1 - x0, y1 - y0);
    let pos = step - carry;
    while (pos <= seg) {
      const f = pos / seg;
      out.push([x0 + (x1 - x0) * f, y0 + (y1 - y0) * f]);
      pos += step;
    }
    carry = seg - (pos - step);
  }
  return out;
}

function catmull(p0, p1, p2, p3, t) {
  const t2 = t * t, t3 = t2 * t;
  const f = (a, b, c, d) => 0.5 * (2 * b + (-a + c) * t + (2 * a - 5 * b + 4 * c - d) * t2 + (-a + 3 * b - 3 * c + d) * t3);
  return [f(p0[0], p1[0], p2[0], p3[0]), f(p0[1], p1[1], p2[1], p3[1])];
}

function buildRoute() {
  // Waypoints (km) for a horseshoe: out along the valley, up a switchback
  // climb to the top, along the ridge, and down to the start.
  const W_ = [[0, 0], [1.1, 0.7], [1.9, 1.9], [2.6, 2.8], [3.9, 2.6], [4.9, 1.9], [5.1, 0.7],
    [4.3, -0.4], [3.0, -1.0], [1.6, -1.1], [0.5, -0.7]].map(([x, y]) => [x * 1000 - 2500, y * 1000 - 700]);
  const n = W_.length;
  let pts = [];
  for (let i = 0; i < n; i++) {
    const p0 = W_[(i - 1 + n) % n], p1 = W_[i], p2 = W_[(i + 1) % n], p3 = W_[(i + 2) % n];
    for (let k = 0; k < 60; k++) pts.push(catmull(p0, p1, p2, p3, k / 60));
  }
  pts.push(pts[0]);
  pts = resample(pts, 5);
  // Wiggle it like a real path, and zigzag the climb (between waypoints 1 and 3).
  const L0 = pts.reduce((acc, p, i) => (i ? acc + Math.hypot(p[0] - pts[i - 1][0], p[1] - pts[i - 1][1]) : 0), 0);
  let sAcc = 0;
  const out = [];
  for (let i = 0; i < pts.length; i++) {
    if (i) sAcc += Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]);
    const a = pts[Math.max(0, i - 1)], b = pts[Math.min(pts.length - 1, i + 1)];
    const dx = b[0] - a[0], dy = b[1] - a[1];
    const l = Math.hypot(dx, dy) || 1;
    const nx = -dy / l, ny = dx / l;
    const f = sAcc / L0;
    // A window over the climb: roughly 9% to 24% of the way round.
    const win = Math.max(0, Math.sin(Math.PI * clamp((f - 0.09) / 0.15)));
    // A rounded triangle wave: straight legs, hairpin turns.
    const tri = (x) => (2 / Math.PI) * Math.asin(0.985 * Math.sin(2 * Math.PI * x));
    const zig = tri(sAcc / 300) * 115 * win;
    const wig = noise2(sAcc / 380, 3.3) * 70 + noise2(sAcc / 110, 8.1) * 16;
    const edge = Math.min(f, 1 - f) < 0.01 ? Math.min(f, 1 - f) / 0.01 : 1; // close the loop cleanly
    out.push([pts[i][0] + nx * (zig + wig) * edge, pts[i][1] + ny * (zig + wig) * edge]);
  }
  pts = out;
  // Scale to exactly 14.2 km about the start.
  const len = pts.reduce((acc, p, i) => (i ? acc + Math.hypot(p[0] - pts[i - 1][0], p[1] - pts[i - 1][1]) : 0), 0);
  const k = 14200 / len;
  const [sx, sy] = pts[0];
  pts = pts.map(([x, y]) => [sx + (x - sx) * k, sy + (y - sy) * k]);
  pts = resample(pts, 10);
  const along = [0];
  for (let i = 1; i < pts.length; i++) along.push(along[i - 1] + Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]));
  let ascent = 0;
  for (let i = 1; i < pts.length; i++) {
    const d = height(...pts[i]) - height(...pts[i - 1]);
    if (d > 0) ascent += d;
  }
  return { pts, along, length: along[along.length - 1], ascent };
}

export const ROUTE = buildRoute();

/** Position and heading (radians, 0 = north, clockwise) at s metres along. */
export function routeAt(s, route = ROUTE) {
  const L = route.length;
  s = ((s % L) + L) % L;
  const A = route.along;
  let lo = 0, hi = A.length - 1;
  while (hi - lo > 1) {
    const mid = (lo + hi) >> 1;
    if (A[mid] <= s) lo = mid;
    else hi = mid;
  }
  const f = (s - A[lo]) / (A[hi] - A[lo] || 1);
  const p = route.pts[lo], q = route.pts[hi];
  const x = lerp(p[0], q[0], f), y = lerp(p[1], q[1], f);
  // Smooth the heading over ~60 m so it doesn't flick at each vertex.
  const back = pointAtRaw(route, s - 30), ahead = pointAtRaw(route, s + 30);
  const hd = Math.atan2(ahead[0] - back[0], ahead[1] - back[1]);
  return { x, y, heading: hd, i: lo };
}

function pointAtRaw(route, s) {
  const L = route.length;
  s = ((s % L) + L) % L;
  const A = route.along;
  let lo = 0, hi = A.length - 1;
  while (hi - lo > 1) {
    const mid = (lo + hi) >> 1;
    if (A[mid] <= s) lo = mid;
    else hi = mid;
  }
  const f = (s - A[lo]) / (A[hi] - A[lo] || 1);
  const p = route.pts[lo], q = route.pts[hi];
  return [lerp(p[0], q[0], f), lerp(p[1], q[1], f)];
}

/** The route's points between two distances along it. */
export function routeSlice(s0, s1, route = ROUTE) {
  const out = [pointAtRaw(route, s0)];
  for (let i = 0; i < route.pts.length; i++) {
    if (route.along[i] > s0 && route.along[i] < s1) out.push(route.pts[i]);
  }
  out.push(pointAtRaw(route, s1));
  return out;
}

export { pointAtRaw as routePoint };
