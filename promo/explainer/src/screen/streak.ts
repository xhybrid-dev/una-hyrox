// X Streak's screens, from hybridx-streak/Software/Apps/LVGL-GUI/gui: the summit scene and its
// mountains (SummitGeometry.cpp, by way of promo/lib/ui-streak.mjs), the climber, the week pips,
// Home, Summit! and Training load. Layout, strings and colours are the code's.

import { WEEK, LOAD_VIEW } from '../config';
import { C, label, buttons, disc, ring, tri, line, wheel, title, STREAK_WHEEL, type Ctx, type Pt } from './lvgl';
import { textWidth, ascent } from './font';

// Palette.hpp, semantic.
const P = {
  star: C.GRAY_DARK, far: C.GRAY_DARK, shade: C.TEAL_DARK, lit: C.TEAL, snowLit: C.WHITE, snowShade: C.GRAY,
  trail: C.GRAY, win: C.LIME, text: C.WHITE, soft: C.GRAY, atRisk: C.YELLOW_DARK, shield: C.CYAN,
};

// Summits.hpp kClimbs.
export const CLIMBS = [
  { name: "Arthur's Seat", summitAt: 4, steps: 4, shape: 0 },
  { name: 'Snowdon', summitAt: 12, steps: 8, shape: 1 },
  { name: 'Ben Nevis', summitAt: 26, steps: 14, shape: 2 },
  { name: 'Mont Blanc', summitAt: 52, steps: 26, shape: 3 },
  { name: 'Everest', summitAt: 104, steps: 52, shape: 4 },
];

/** climbFor(): the climb in progress after `weeks` achieved weeks. */
export function climbFor(weeks: number) {
  let floor = 0;
  for (let i = 0; i < CLIMBS.length; i++) {
    if (weeks < CLIMBS[i].summitAt) return { climb: i, stepsClimbed: weeks - floor, steps: CLIMBS[i].steps };
    floor = CLIMBS[i].summitAt;
  }
  return { climb: 4, stepsClimbed: (weeks - floor) % 52, steps: 52 };
}

// SummitGeometry.cpp kDesigns: apex, left and right feet, shoulders, snow %.
const DESIGNS: { apex: Pt; left: Pt; right: Pt; sh: [Pt, Pt, Pt][]; snow: number }[] = [
  { apex: [26, -66], left: [-76, 0], right: [96, 0], sh: [[[-28, -38], [-98, 0], [30, 0]]], snow: 0 },
  { apex: [0, -84], left: [-86, 0], right: [86, 0], sh: [[[52, -46], [0, 0], [102, 0]], [[-54, -34], [-104, 0], [-10, 0]]], snow: 18 },
  { apex: [8, -76], left: [-98, 0], right: [96, 0], sh: [[[-40, -50], [-104, 0], [10, 0]]], snow: 24 },
  { apex: [0, -82], left: [-94, 0], right: [94, 0], sh: [[[-50, -44], [-104, 0], [0, 0]], [[52, -48], [0, 0], [104, 0]]], snow: 44 },
  { apex: [-4, -92], left: [-72, 0], right: [70, 0], sh: [[[44, -58], [-10, 0], [102, 0]]], snow: 32 },
];

const lerp = (a: number, b: number, t: number) => a + (b - a) * t;

function mountain(shape: number, baseY: number, scale: number, cx = 120) {
  const d = DESIGNS[shape];
  const Q = ([dx, dy]: Pt): Pt => [cx + (dx * scale) / 100, baseY + (dy * scale) / 100];
  return { apex: Q(d.apex), left: Q(d.left), right: Q(d.right), sh: d.sh.map((s) => s.map(Q) as [Pt, Pt, Pt]), snow: d.snow };
}
type Mountain = ReturnType<typeof mountain>;

function edgeX(m: Mountain, base: Pt, y: number) {
  const h = base[1] - m.apex[1];
  return h <= 0 ? m.apex[0] : m.apex[0] + ((base[0] - m.apex[0]) * (y - m.apex[1])) / h;
}

/** trailFor(): switchbacks up the main peak. */
function trail(m: Mountain) {
  const yFoot = m.left[1] - 5;
  const yTop = m.apex[1] + 9;
  const h = yFoot - yTop;
  const legs = h > 70 ? 4 : 3;
  const pts: Pt[] = [];
  for (let j = 0; j < legs; j++) {
    const y = yFoot - (h * j) / legs;
    const xl = edgeX(m, m.left, y), xr = edgeX(m, m.right, y);
    pts.push([xl + (xr - xl) * (j % 2 === 0 ? 0.64 : 0.32), y]);
  }
  pts.push([m.apex[0], yTop]);
  const along = [0];
  for (let i = 1; i < pts.length; i++) along.push(along[i - 1] + Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]));
  return { pts, along };
}

function stepPos(tr: ReturnType<typeof trail>, s: number, n: number): Pt {
  const target = (tr.along[tr.along.length - 1] * s) / n;
  for (let i = 1; i < tr.pts.length; i++) {
    if (target <= tr.along[i]) {
      const f = (target - tr.along[i - 1]) / (tr.along[i] - tr.along[i - 1] || 1);
      return [lerp(tr.pts[i - 1][0], tr.pts[i][0], f), lerp(tr.pts[i - 1][1], tr.pts[i][1], f)];
    }
  }
  return tr.pts[tr.pts.length - 1];
}

const STARS: Pt[] = [[62, 34], [88, 20], [158, 24], [186, 44], [44, 58], [204, 70], [136, 12]];

/** SummitScene: stars, far ranges, the mountain, its trail lime behind the climber, the flag. */
function scene(ctx: Ctx, shape: number, climbed: number, steps: number, o: { baseY?: number; scale?: number; wave?: number; flag?: boolean } = {}) {
  const baseY = o.baseY ?? 118, scale = o.scale ?? 100;
  const m = mountain(shape, baseY, scale);
  STARS.forEach(([x, y], i) => { ctx.fillStyle = i % 3 === 0 ? C.GRAY : P.star; ctx.fillRect(x, y, 2, 2); });
  const y = (u: number) => baseY - u;
  ([[[38, y(32)], [-8, baseY], [86, baseY]], [[98, y(40)], [48, baseY], [152, baseY]],
    [[164, y(34)], [112, baseY], [222, baseY]], [[214, y(26)], [168, baseY], [252, baseY]]] as [Pt, Pt, Pt][])
    .forEach(([a, b, c]) => tri(ctx, a, b, c, P.far));
  m.sh.forEach(([a, b, c]) => tri(ctx, a, b, c, P.shade));
  const foot: Pt = [m.apex[0], m.left[1]];
  tri(ctx, m.apex, m.left, foot, P.lit);
  tri(ctx, m.apex, foot, m.right, P.shade);
  if (m.snow > 0) {
    const a = m.apex;
    const l: Pt = [lerp(a[0], m.left[0], m.snow / 100), lerp(a[1], m.left[1], m.snow / 100)];
    const r: Pt = [lerp(a[0], m.right[0], m.snow / 100), lerp(a[1], m.right[1], m.snow / 100)];
    const mid: Pt = [a[0], l[1] + (l[1] - a[1]) / 5 + 2];
    tri(ctx, a, l, mid, P.snowLit);
    tri(ctx, a, mid, r, P.snowShade);
  }
  const tr = trail(m);
  const total = tr.along[tr.along.length - 1];
  const here = (total * climbed) / steps;
  const hereP = stepPos(tr, climbed, steps);
  for (let i = 1; i < tr.pts.length; i++) {
    const a = tr.pts[i - 1], b = tr.pts[i];
    if (tr.along[i] <= here) line(ctx, a, b, 3, P.win);
    else if (tr.along[i - 1] >= here) line(ctx, a, b, 1, P.trail);
    else { line(ctx, hereP, b, 1, P.trail); line(ctx, a, hereP, 3, P.win); }
  }
  if (steps <= 8) {
    for (let s = 1; s < steps; s++) {
      const p = stepPos(tr, s, steps);
      if (s <= climbed) disc(ctx, p[0], p[1], 3, P.win);
      else disc(ctx, p[0], p[1], 2, P.trail);
    }
  }
  if (o.flag !== false) {
    const top = m.apex, k = scale / 100, wave = o.wave ?? 0;
    const poleTop: Pt = [top[0], top[1] - 15 * k];
    line(ctx, top, poleTop, 2, C.WHITE, 'butt');
    tri(ctx, [top[0] + 1, poleTop[1]], [top[0] + 12 * k, poleTop[1] + (4 + wave) * k], [top[0] + 1, poleTop[1] + 8 * k], P.win);
  }
  return { here: hereP };
}

/** Climber: a lime dot in a white ring, its halo pulsing 0..3 every 0.9 s. */
function climber(ctx: Ctx, [x, y]: Pt, t: number) {
  const ph = (t % 1.8) / 0.9;
  const pulse = Math.round(3 * (ph < 1 ? ph : 2 - ph));
  ring(ctx, x, y, 9 + pulse, 1, P.win);
  disc(ctx, x, y, 6, C.WHITE);
  disc(ctx, x, y, 4, P.win);
}

/** Burst: eight lime rays, 0..12 over 0.5 s, ease out. */
function burst(ctx: Ctx, [x, y]: Pt, k: number) {
  if (k <= 0 || k >= 1) return;
  const len = Math.round(12 * (1 - (1 - k) * (1 - k)));
  const rays: Pt[] = [[16, 0], [11, 11], [0, 16], [-11, 11], [-16, 0], [-11, -11], [0, -16], [11, -11]];
  const r0 = 10 + len / 2, r1 = 10 + len;
  rays.forEach(([dx, dy]) => line(ctx, [x + (dx * r0) / 16, y + (dy * r0) / 16], [x + (dx * r1) / 16, y + (dy * r1) / 16], 2, P.win));
}

/** WeekPips: lime when done, grey rings when not; the newest pops by up to 4 px. */
function pips(ctx: Ctx, target: number, sessions: number, centreX: number, y: number, pop: number) {
  const width = (target - 1) * 16 + 20;
  const x0 = centreX - width / 2;
  for (let i = 0; i < target; i++) {
    const cx = x0 + 10 + i * 16, cy = y + 10;
    if (i < sessions) disc(ctx, cx, cy, 5 + (i === sessions - 1 ? pop / 2 : 0), P.win);
    else ring(ctx, cx, cy, 5, 2, P.trail);
  }
  return width;
}

export type HomeState = {
  weeks: number; // achieved weeks before any glide
  glide: number; // 0..1 up one step
  target: number; // sessions a week
  streak: number | null; // the headline's number, or null for words alone
  words: string;
  wordsColor?: string;
  sessions: number;
  pop: number; // 0..4
  coach: string;
  coachColor: string;
  burst: number; // 0..1 through the burst, 0 for none
};

/** HomeScreen: the mountain, the mountain line, the headline, this week, the coach. */
export function home(ctx: Ctx, s: HomeState, t: number) {
  const pos = climbFor(s.weeks);
  const c = CLIMBS[pos.climb];
  const climbed = pos.stepsClimbed + s.glide;
  const sc = scene(ctx, c.shape, climbed, pos.steps);
  climber(ctx, sc.here, t);
  burst(ctx, sc.here, s.burst);
  const left = pos.steps - pos.stepsClimbed;
  label(ctx, 'regular_14', `${c.name} · ${left} week${left === 1 ? '' : 's'} to go`, 20, 119, 200, 'center', P.soft);
  // The headline on baseline 163: the number in SemiBold 30 lime, then the words.
  const num = s.streak != null ? String(s.streak) : '';
  let wf: 'medium_18' | 'semibold_25' | 'semibold_20' = 'medium_18';
  if (!num) wf = textWidth('semibold_25', s.words) > 196 ? 'semibold_20' : 'semibold_25';
  const numW = num ? textWidth('semibold_30', num) : 0;
  const wordW = textWidth(wf, s.words);
  const left0 = 120 - Math.trunc((numW + (num ? 6 : 0) + wordW) / 2);
  if (num) label(ctx, 'semibold_30', num, left0, 163 - ascent('semibold_30'), numW + 2, 'left', P.win);
  label(ctx, wf, s.words, left0 + numW + (num ? 6 : 0), 163 - ascent(wf), wordW + 2, 'left', s.wordsColor ?? P.text);
  // The week row at y 172: pips and "this week", centred together.
  const tw = textWidth('medium_18', 'this week');
  const pw = (s.target - 1) * 16 + 20;
  const rowL = 120 - Math.trunc((pw + 8 + tw) / 2);
  pips(ctx, s.target, s.sessions, rowL + pw / 2, 173, s.pop);
  label(ctx, 'medium_18', 'this week', rowL + pw + 8, 172, tw + 2, 'left');
  label(ctx, 'regular_14', s.coach, 24, 197, 192, 'center', s.coachColor);
  buttons(ctx, { r1: 'amber', r2: 'white' });
}

// SummitScreen's confetti: 16 pieces (x, start y, delay ms, fall ms, colour).
const PIECES = [
  [46, -10, 0, 2200, 0], [70, -40, 150, 2600, 1], [94, -20, 300, 2000, 2], [118, -60, 60, 2400, 3],
  [142, -15, 420, 2100, 0], [166, -45, 210, 2500, 1], [190, -25, 360, 2300, 2], [58, -70, 520, 2700, 3],
  [82, -30, 640, 2200, 1], [106, -50, 780, 2600, 0], [130, -35, 900, 2000, 3], [154, -65, 700, 2400, 2],
  [178, -20, 580, 2150, 1], [202, -55, 820, 2550, 0], [34, -45, 960, 2300, 2], [214, -30, 1040, 2250, 3],
];
const CONF = [P.win, P.shield, C.LEMON, C.WHITE];

/** SummitScreen: the hero peak, its waving flag, confetti, "Summit!". t: seconds on screen. */
export function summit(ctx: Ctx, climb: number, t: number) {
  const c = CLIMBS[climb];
  const wave = Math.round(2 * Math.sin((t * Math.PI * 2) / 0.76));
  scene(ctx, c.shape, c.steps, c.steps, { baseY: 176, scale: 120, wave });
  const ms = t * 1000;
  PIECES.forEach(([x, y0, delay, fall, col], i) => {
    const k = (ms - delay) / fall;
    if (k < 0) return;
    const kk = k % 1;
    ctx.save();
    ctx.fillStyle = CONF[col];
    ctx.translate(x + Math.sin(kk * 9 + i) * 4, y0 + kk * 260);
    ctx.rotate(Math.sin(kk * 7 + i) * 0.6);
    ctx.fillRect(-2, -3.5, 4, 7);
    ctx.restore();
  });
  label(ctx, 'semibold_30', 'Summit!', 20, 14, 200);
  label(ctx, 'medium_18', `${c.name} · ${c.summitAt} weeks`, 10, 180, 220, 'center', P.win);
  label(ctx, 'regular_14', climb < 4 ? `Next: ${CLIMBS[climb + 1].name}` : 'Next: Everest again', 40, 204, 160, 'center', P.soft);
  buttons(ctx, { r1: 'amber' });
}

/** LoadScreen: Form selected, Fitness below. */
export function trainingLoad(ctx: Ctx) {
  const weekLoad = WEEK.sessions.reduce((a, s) => a + s.load, 0);
  title(ctx, 'Training load');
  wheel(ctx, [
    { text: 'Form', tip: LOAD_VIEW.form },
    { text: 'Fitness', tip: LOAD_VIEW.fitness, tipColor: P.soft },
    { text: 'Fatigue', tip: LOAD_VIEW.fatigue, tipColor: P.soft },
    { text: 'This week', tip: `Load ${weekLoad}`, tipColor: P.soft },
    { text: 'Today', tip: `Load ${WEEK.sessions[3].load}`, tipColor: P.soft },
  ], 0, STREAK_WHEEL);
  buttons(ctx, { l1: 'white', l2: 'white', r2: 'white' });
}
