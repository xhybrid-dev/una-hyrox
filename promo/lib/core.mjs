// Core maths for the films: frame constants, easing, deterministic noise.
//
// Every frame is a pure function of its time, so any frame can be rendered on
// its own and the film can be cut into parts that render in parallel. Nothing
// here keeps state between frames.

export const W = 1920;
export const H = 1080;
export const FPS = 60;
export const TAU = Math.PI * 2;

export const clamp = (x, a = 0, b = 1) => (x < a ? a : x > b ? b : x);
export const lerp = (a, b, t) => a + (b - a) * t;
/** Progress of x through [a, b], clamped to 0..1. */
export const prog = (x, a, b) => clamp((x - a) / (b - a));
export const remap = (x, a, b, c, d) => lerp(c, d, prog(x, a, b));
export const mix = (a, b, t) => a.map((v, i) => lerp(v, b[i], t));
export const smoothstep = (a, b, x) => {
  const t = prog(x, a, b);
  return t * t * (3 - 2 * t);
};
/** 0 → 1 → 0 over [a, d], flat at 1 between b and c. */
export const window4 = (x, a, b, c, d) => Math.min(prog(x, a, b), 1 - prog(x, c, d));

// --- Easing -----------------------------------------------------------------

export const E = {
  linear: (t) => t,
  inQuad: (t) => t * t,
  outQuad: (t) => 1 - (1 - t) * (1 - t),
  inOutQuad: (t) => (t < 0.5 ? 2 * t * t : 1 - Math.pow(-2 * t + 2, 2) / 2),
  inCubic: (t) => t * t * t,
  outCubic: (t) => 1 - Math.pow(1 - t, 3),
  inOutCubic: (t) => (t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2),
  inQuart: (t) => t * t * t * t,
  outQuart: (t) => 1 - Math.pow(1 - t, 4),
  inOutQuart: (t) => (t < 0.5 ? 8 * t * t * t * t : 1 - Math.pow(-2 * t + 2, 4) / 2),
  inQuint: (t) => t * t * t * t * t,
  outQuint: (t) => 1 - Math.pow(1 - t, 5),
  inOutQuint: (t) => (t < 0.5 ? 16 * t * t * t * t * t : 1 - Math.pow(-2 * t + 2, 5) / 2),
  inExpo: (t) => (t <= 0 ? 0 : Math.pow(2, 10 * t - 10)),
  outExpo: (t) => (t >= 1 ? 1 : 1 - Math.pow(2, -10 * t)),
  inOutExpo: (t) =>
    t <= 0 ? 0 : t >= 1 ? 1 : t < 0.5 ? Math.pow(2, 20 * t - 10) / 2 : (2 - Math.pow(2, -20 * t + 10)) / 2,
  inSine: (t) => 1 - Math.cos((t * Math.PI) / 2),
  outSine: (t) => Math.sin((t * Math.PI) / 2),
  inOutSine: (t) => -(Math.cos(Math.PI * t) - 1) / 2,
  outBack: (t, s = 1.70158) => 1 + (s + 1) * Math.pow(t - 1, 3) + s * Math.pow(t - 1, 2),
  inBack: (t, s = 1.70158) => (s + 1) * t * t * t - s * t * t,
  inOutBack: (t, s = 1.70158 * 1.525) =>
    t < 0.5
      ? (Math.pow(2 * t, 2) * ((s + 1) * 2 * t - s)) / 2
      : (Math.pow(2 * t - 2, 2) * ((s + 1) * (t * 2 - 2) + s) + 2) / 2,
  outCirc: (t) => Math.sqrt(1 - Math.pow(t - 1, 2)),
  inOutCirc: (t) =>
    t < 0.5 ? (1 - Math.sqrt(1 - Math.pow(2 * t, 2))) / 2 : (Math.sqrt(1 - Math.pow(-2 * t + 2, 2)) + 1) / 2,
};

/** Eased progress of x through [a, b]. */
export const ep = (x, a, b, ease = E.inOutCubic) => ease(prog(x, a, b));

/**
 * A damped spring's response to a unit step at time 0, evaluated at t
 * seconds: 0 before the step, settling on 1 with an overshoot.
 */
export function spring(t, freq = 2.4, damping = 0.42) {
  if (t <= 0) return 0;
  const w = TAU * freq;
  const z = damping;
  if (z >= 1) return 1 - Math.exp(-w * t) * (1 + w * t);
  const wd = w * Math.sqrt(1 - z * z);
  return 1 - Math.exp(-z * w * t) * (Math.cos(wd * t) + ((z * w) / wd) * Math.sin(wd * t));
}

// --- Deterministic randomness ---------------------------------------------

/** mulberry32: a small, fast, seeded PRNG. */
export function rng(seed) {
  let a = seed >>> 0;
  return () => {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

/** A stable pseudo-random number in 0..1 for an integer (or pair). */
export function hash(n, m = 0) {
  let h = Math.imul(n | 0, 0x27d4eb2d) ^ Math.imul(m | 0, 0x165667b1);
  h = Math.imul(h ^ (h >>> 15), 0x85ebca6b);
  h = Math.imul(h ^ (h >>> 13), 0xc2b2ae35);
  h ^= h >>> 16;
  return (h >>> 0) / 4294967296;
}

// 2D gradient noise (Perlin-style, with a fixed permutation) for terrain.
const PERM = (() => {
  const r = rng(1729);
  const p = Array.from({ length: 256 }, (_, i) => i);
  for (let i = 255; i > 0; i--) {
    const j = Math.floor(r() * (i + 1));
    [p[i], p[j]] = [p[j], p[i]];
  }
  return Uint8Array.from([...p, ...p]);
})();
const GRAD = [
  [1, 0], [-1, 0], [0, 1], [0, -1],
  [0.7071, 0.7071], [-0.7071, 0.7071], [0.7071, -0.7071], [-0.7071, -0.7071],
];
const fade = (t) => t * t * t * (t * (t * 6 - 15) + 10);

export function noise2(x, y) {
  const xi = Math.floor(x), yi = Math.floor(y);
  const xf = x - xi, yf = y - yi;
  const X = xi & 255, Y = yi & 255;
  const g = (ix, iy, dx, dy) => {
    const v = GRAD[PERM[PERM[ix] + iy] & 7];
    return v[0] * dx + v[1] * dy;
  };
  const n00 = g(X, Y, xf, yf);
  const n10 = g(X + 1, Y, xf - 1, yf);
  const n01 = g(X, Y + 1, xf, yf - 1);
  const n11 = g(X + 1, Y + 1, xf - 1, yf - 1);
  const u = fade(xf), v = fade(yf);
  return lerp(lerp(n00, n10, u), lerp(n01, n11, u), v);
}

export function fbm(x, y, octaves = 4, lac = 2.0, gain = 0.5) {
  let a = 1, f = 1, s = 0, n = 0;
  for (let i = 0; i < octaves; i++) {
    s += a * noise2(x * f, y * f);
    n += a;
    a *= gain;
    f *= lac;
  }
  return s / n;
}

// --- Music time ---------------------------------------------------------------

/** Bar/beat arithmetic for a film's tempo (4/4). */
export function tempo(bpm) {
  const beat = 60 / bpm;
  const bar = beat * 4;
  return {
    bpm,
    beat,
    bar,
    /** Seconds at bar b (0-based), plus an optional number of beats. */
    at: (b, beats = 0) => b * bar + beats * beat,
    /** Which beat (fractional) a time falls on. */
    beatOf: (t) => t / beat,
    /** 1 on each beat, decaying over `len` seconds: for pulses on the beat. */
    pulse: (t, len = 0.25, every = 1) => {
      const period = beat * every;
      const ph = ((t % period) + period) % period;
      return Math.exp(-ph / (len / 4));
    },
  };
}

// --- Formatting -------------------------------------------------------------

export const pad2 = (n) => String(Math.floor(n)).padStart(2, '0');
/** m:ss, as the race face shows a segment. */
export function mss(sec) {
  sec = Math.max(0, Math.floor(sec));
  return `${Math.floor(sec / 60)}:${pad2(sec % 60)}`;
}
/** h:mm:ss, as the race face shows the total. */
export function hmss(sec) {
  sec = Math.max(0, Math.floor(sec));
  return `${Math.floor(sec / 3600)}:${pad2((sec / 60) % 60)}:${pad2(sec % 60)}`;
}
