// HybridX Streak's screens, redrawn from the app's own code
// (hybridx-streak/Software/Apps/LVGL-GUI/gui/src): the summit scene and its
// five mountains, the climber, the week pips, and the moments.

import { C } from './brand.mjs';
import { F, label, buttons, disc, ring, tri, line, tick, cross, textWidth, wheel, hline, title } from './lvgl.mjs';
import { clamp, lerp, E, hash, TAU } from './core.mjs';

// Summits.hpp and DESIGN.md §5: the ladder.
export const CLIMBS = [
  { name: "Arthur's Seat", weeks: 4, steps: 4, shape: 0 },
  { name: 'Snowdon', weeks: 12, steps: 8, shape: 1 },
  { name: 'Ben Nevis', weeks: 26, steps: 14, shape: 2 },
  { name: 'Mont Blanc', weeks: 52, steps: 26, shape: 3 },
  { name: 'Everest', weeks: 104, steps: 52, shape: 4 },
];

// SummitGeometry.cpp kDesigns: apex, left, right, shoulders, snow %.
const DESIGNS = [
  { apex: [26, -66], left: [-76, 0], right: [96, 0], sh: [[[-28, -38], [-98, 0], [30, 0]]], snow: 0 },
  { apex: [0, -84], left: [-86, 0], right: [86, 0], sh: [[[52, -46], [0, 0], [102, 0]], [[-54, -34], [-104, 0], [-10, 0]]], snow: 18 },
  { apex: [8, -76], left: [-98, 0], right: [96, 0], sh: [[[-40, -50], [-104, 0], [10, 0]]], snow: 24 },
  { apex: [0, -82], left: [-94, 0], right: [94, 0], sh: [[[-50, -44], [-104, 0], [0, 0]], [[52, -48], [0, 0], [104, 0]]], snow: 44 },
  { apex: [-4, -92], left: [-72, 0], right: [70, 0], sh: [[[44, -58], [-10, 0], [102, 0]]], snow: 32 },
];

/** mountainFor(): the mountain for a shape with its base on baseY. */
export function mountain(shape, baseY = 118, scale = 100, cx = 120) {
  const d = DESIGNS[shape];
  const P = ([dx, dy]) => [cx + (dx * scale) / 100, baseY + (dy * scale) / 100];
  return { apex: P(d.apex), left: P(d.left), right: P(d.right), sh: d.sh.map((s) => s.map(P)), snow: d.snow };
}

function edgeX(m, base, y) {
  const h = base[1] - m.apex[1];
  if (h <= 0) return m.apex[0];
  return m.apex[0] + ((base[0] - m.apex[0]) * (y - m.apex[1])) / h;
}

/** trailFor(): switchbacks up the main peak. */
export function trail(m) {
  const yFoot = m.left[1] - 5;
  const yTop = m.apex[1] + 9;
  const h = yFoot - yTop;
  const legs = h > 70 ? 4 : 3;
  const pts = [];
  for (let j = 0; j < legs; j++) {
    const y = yFoot - (h * j) / legs;
    const xl = edgeX(m, m.left, y);
    const xr = edgeX(m, m.right, y);
    const pct = j % 2 === 0 ? 0.64 : 0.32;
    pts.push([xl + (xr - xl) * pct, y]);
  }
  pts.push([m.apex[0], yTop]);
  const along = [0];
  for (let i = 1; i < pts.length; i++) along.push(along[i - 1] + Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]));
  return { pts, along };
}

/** stepPosition(): where step s of n sits on the trail (fraction adds part of a step). */
export function stepPos(tr, s, n, frac = 0) {
  const total = tr.along[tr.along.length - 1];
  const target = (total * (s + frac)) / n;
  for (let i = 1; i < tr.pts.length; i++) {
    if (target <= tr.along[i]) {
      const f = (target - tr.along[i - 1]) / (tr.along[i] - tr.along[i - 1] || 1);
      return [lerp(tr.pts[i - 1][0], tr.pts[i][0], f), lerp(tr.pts[i - 1][1], tr.pts[i][1], f)];
    }
  }
  return tr.pts[tr.pts.length - 1];
}

const STARS = [[62, 34], [88, 20], [158, 24], [186, 44], [44, 58], [204, 70], [136, 12]];

/**
 * The summit scene (SummitScene.cpp). o: { shape, climbed, steps, frac,
 * baseY, scale, stars, flag, wave, far, t } — t drives the flag.
 */
export function scene(ctx, o) {
  const baseY = o.baseY ?? 118;
  const scale = o.scale ?? 100;
  const m = mountain(o.shape, baseY, scale);
  if (o.stars !== false) {
    STARS.forEach(([x, y], i) => {
      ctx.fillStyle = i % 3 === 0 ? C.gray : C.grayDark;
      ctx.fillRect(x, y, 2, 2);
    });
  }
  if (o.far !== false) {
    const y = (u) => baseY - u;
    [[[38, y(32)], [-8, baseY], [86, baseY]], [[98, y(40)], [48, baseY], [152, baseY]],
      [[164, y(34)], [112, baseY], [222, baseY]], [[214, y(26)], [168, baseY], [252, baseY]]].forEach(([a, b, c]) => tri(ctx, a, b, c, C.grayDark));
  }
  m.sh.forEach(([a, b, c]) => tri(ctx, a, b, c, C.tealDark));
  const foot = [m.apex[0], m.left[1]];
  tri(ctx, m.apex, m.left, foot, C.teal);
  tri(ctx, m.apex, foot, m.right, C.tealDark);
  if (m.snow > 0) {
    const a = m.apex;
    const l = [lerp(a[0], m.left[0], m.snow / 100), lerp(a[1], m.left[1], m.snow / 100)];
    const r = [lerp(a[0], m.right[0], m.snow / 100), lerp(a[1], m.right[1], m.snow / 100)];
    const dip = (l[1] - a[1]) / 5 + 2;
    const mid = [a[0], l[1] + dip];
    tri(ctx, a, l, mid, C.white);
    tri(ctx, a, mid, r, C.gray);
  }
  // The trail: lime behind the climber, grey ahead.
  const tr = trail(m);
  const steps = o.steps ?? 8;
  const climbed = o.climbed ?? 0;
  const frac = o.frac ?? 0;
  const total = tr.along[tr.along.length - 1];
  const here = (total * (climbed + frac)) / steps;
  const hereP = stepPos(tr, climbed, steps, frac);
  for (let i = 1; i < tr.pts.length; i++) {
    const a = tr.pts[i - 1], b = tr.pts[i];
    if (tr.along[i] <= here) line(ctx, a, b, 3, C.lime);
    else if (tr.along[i - 1] >= here) line(ctx, a, b, 1, C.gray);
    else {
      line(ctx, hereP, b, 1, C.gray);
      line(ctx, a, hereP, 3, C.lime);
    }
  }
  if (steps <= 8) {
    for (let s = 1; s < steps; s++) {
      const p = stepPos(tr, s, steps);
      if (s <= climbed) disc(ctx, p[0], p[1], 3, C.lime);
      else disc(ctx, p[0], p[1], 2, C.gray);
    }
  }
  if (o.flag !== false) {
    const top = m.apex;
    const poleTop = [top[0], top[1] - 15 * (scale / 100)];
    line(ctx, top, poleTop, 2, C.white, 'butt');
    const wave = o.wave ?? 0;
    tri(ctx, [top[0] + 1, poleTop[1]], [top[0] + 12 * (scale / 100), poleTop[1] + (4 + wave) * (scale / 100)], [top[0] + 1, poleTop[1] + 8 * (scale / 100)], C.lime);
  }
  return { m, tr, here: hereP };
}

/** Climber.cpp: a lime dot in a white ring, with a pulsing lime halo. */
export function climber(ctx, x, y, pulse = 0) {
  ring(ctx, x, y, 9 + pulse, 1, C.lime);
  disc(ctx, x, y, 6, C.white);
  disc(ctx, x, y, 4, C.lime);
}

/** Burst.cpp: eight lime rays, growing out (len 0..12). */
export function burst(ctx, x, y, len, alpha = 1) {
  if (len <= 0 || alpha <= 0) return;
  ctx.save();
  ctx.globalAlpha *= alpha;
  const r0 = 10 + len / 2, r1 = 10 + len;
  for (let i = 0; i < 8; i++) {
    const a = (i / 8) * TAU;
    line(ctx, [x + Math.cos(a) * r0, y + Math.sin(a) * r0], [x + Math.cos(a) * r1, y + Math.sin(a) * r1], 2, C.lime);
  }
  ctx.restore();
}

/** WeekPips: target pips, lime when done, grey rings when not, cyan bonus. */
export function pips(ctx, target, sessions, centreX, y, popIdx = -1, pop = 0) {
  const bonus = Math.min(3, Math.max(0, sessions - target));
  const count = target + bonus;
  const width = (count - 1) * 16 + 20;
  const x0 = centreX - width / 2;
  for (let i = 0; i < count; i++) {
    const cx = x0 + 10 + i * 16, cy = y + 10;
    const r = 5 + (i === popIdx ? pop / 2 : 0);
    if (i >= target) disc(ctx, cx, cy, r - 1, C.shield);
    else if (i < sessions) disc(ctx, cx, cy, r, C.lime);
    else ring(ctx, cx, cy, 5, 2, C.gray);
  }
  return width;
}

const COACH_COL = { soft: C.gray, risk: C.amber, win: C.lime };

/**
 * The home screen. s: { climb (0-4), climbed, frac, streak, words, wordsColor,
 * target, sessions, coach, mood ('soft'|'risk'|'win'), toast, pulse, burst,
 * popIdx, pop, t }
 */
export function home(ctx, s) {
  const c = CLIMBS[s.climb];
  const sc = scene(ctx, { shape: c.shape, climbed: s.climbed, steps: c.steps, frac: s.frac || 0, wave: s.wave || 0 });
  climber(ctx, sc.here[0], sc.here[1], s.pulse || 0);
  if (s.burst) burst(ctx, sc.here[0], sc.here[1], s.burst, s.burstA ?? 1);
  // Mountain line.
  // Weeks to go are cumulative: each climb starts where the last one ended.
  const prevWeeks = s.climb > 0 ? CLIMBS[s.climb - 1].weeks : 0;
  const left = c.weeks - (prevWeeks + (s.climbed ?? 0));
  const mline = s.mline || `${c.name} · ${left} week${left === 1 ? '' : 's'} to go`;
  label(ctx, F.Regular14, mline, 20, 119, 200, 'center', C.gray);
  // Headline on baseline 163.
  const num = s.streak != null ? String(s.streak) : '';
  const words = s.words ?? 'week streak';
  const wf = num ? F.Medium18 : (textWidth(ctx, F.SemiBold25, words) > 196 ? F.SemiBold20 : F.SemiBold25);
  const numW = num ? textWidth(ctx, F.SemiBold30, num) : 0;
  const wordW = textWidth(ctx, wf, words);
  const total = numW + (num ? 6 : 0) + wordW;
  const lx = 120 - total / 2;
  if (num) label(ctx, F.SemiBold30, num, lx, 163 - F.SemiBold30.asc, numW + 2, 'left', C.lime);
  label(ctx, wf, words, lx + numW + (num ? 6 : 0), 163 - wf.asc, wordW + 2, 'left', s.wordsColor || C.white);
  // Week row: pips and "this week", centred together.
  const tw = textWidth(ctx, F.Medium18, 'this week');
  const bonus = Math.min(3, Math.max(0, s.sessions - s.target));
  const pw = (s.target + bonus - 1) * 16 + 20;
  const rowL = 120 - (pw + 8 + tw) / 2;
  pips(ctx, s.target, s.sessions, rowL + pw / 2, 173, s.popIdx ?? -1, s.pop || 0);
  label(ctx, F.Medium18, 'this week', rowL + pw + 8, 172, tw + 2, 'left', C.white);
  // Coach line (or a toast).
  const coach = s.toast || s.coach || '';
  const col = s.toast ? C.lime : COACH_COL[s.mood || 'soft'];
  label(ctx, F.Regular14, coach, 24, 197, 192, 'center', col);
  buttons(ctx, { l1: 'white', l2: 'white', r1: 'amber', r2: 'white' });
}

/** SummitScreen: "Summit!", the hero peak and its waving flag, confetti. */
const PIECES = [
  [46, -10, 0, 2200, 0], [70, -40, 150, 2600, 1], [94, -20, 300, 2000, 2], [118, -60, 60, 2400, 3],
  [142, -15, 420, 2100, 0], [166, -45, 210, 2500, 1], [190, -25, 360, 2300, 2], [58, -70, 520, 2700, 3],
  [82, -30, 640, 2200, 1], [106, -50, 780, 2600, 0], [130, -35, 900, 2000, 3], [154, -65, 700, 2400, 2],
  [178, -20, 580, 2150, 1], [202, -55, 820, 2550, 0], [34, -45, 960, 2300, 2], [214, -30, 1040, 2250, 3],
];
const CONF = [C.lime, C.shield, C.lemon, C.white];

export function summitScreen(ctx, s) {
  const c = CLIMBS[s.climb];
  const wave = Math.round(2 * Math.sin(((s.t || 0) * TAU) / 0.76));
  scene(ctx, { shape: c.shape, climbed: c.steps, steps: c.steps, baseY: 176, scale: 120, wave, far: true });
  label(ctx, F.SemiBold30, 'Summit!', 20, 14, 200, 'center', C.white);
  label(ctx, F.Medium18, `${c.name} · ${c.weeks} weeks`, 10, 180, 220, 'center', C.lime);
  label(ctx, F.Regular14, s.next || (s.climb < 4 ? `Next: ${CLIMBS[s.climb + 1].name}` : 'Next: Everest again'), 40, 204, 160, 'center', C.gray);
  const ms = (s.t || 0) * 1000;
  PIECES.forEach(([x, y0, delay, fall, col], i) => {
    const k = (ms - delay) / fall;
    if (k < 0) return;
    const kk = k % 1;
    const y = y0 + kk * 260;
    ctx.fillStyle = CONF[col];
    ctx.save();
    ctx.translate(x + Math.sin(kk * 9 + i) * 4, y);
    ctx.rotate(Math.sin(kk * 7 + i) * 0.6);
    ctx.fillRect(-2, -3.5, 4, 7);
    ctx.restore();
  });
}

/** Widgets::shieldGlyph at (centreX, topY), optionally scaled. */
export function shieldGlyph(ctx, cx, top, k = 1) {
  ctx.save();
  ctx.translate(cx - 32 * k, top);
  ctx.scale(k, k);
  const T = (a, b, c, col) => tri(ctx, a, b, c, col);
  T([4, 4], [60, 4], [60, 40], C.shield); T([4, 4], [60, 40], [4, 40], C.shield); T([4, 40], [60, 40], [32, 70], C.shield);
  T([10, 10], [54, 10], [54, 38], C.tealDark); T([10, 10], [54, 38], [10, 38], C.tealDark); T([10, 38], [54, 38], [32, 62], C.tealDark);
  T([32, 22], [16, 46], [32, 46], C.white); T([32, 22], [32, 46], [48, 46], C.gray);
  line(ctx, [32, 22], [32, 13], 2, C.white, 'butt');
  T([33, 13], [41, 16], [33, 19], C.lime);
  ctx.restore();
}

/** ShieldScreen: the offer, then "Streak saved." */
export function shieldScreen(ctx, s) {
  shieldGlyph(ctx, 120, 18);
  if (!s.saved) {
    label(ctx, F.SemiBold20, 'Life happens.', 20, 96, 200, 'center', C.white);
    const body = ['1 of 3 last week.', 'Use a shield to', 'keep your streak', 'of 9 weeks?'];
    body.forEach((ln, i) => label(ctx, F.Regular16, ln, 40, 122 + i * 19, 140, 'center', C.gray));
    label(ctx, F.Regular14, '2 shields left', 50, 202, 140, 'center', C.shield);
    tick(ctx, 186, 60, C.chartreuse);
    cross(ctx, 187, 163, C.white);
    buttons(ctx, { r1: 'green', r2: 'white' }, s.glow || {});
  } else {
    label(ctx, F.SemiBold20, 'Streak saved.', 20, 96, 200, 'center', C.lime);
    ['Your streak of 9', 'weeks climbs on.', 'Rest well.'].forEach((ln, i) => label(ctx, F.Regular16, ln, 20, 122 + i * 19, 200, 'center', C.gray));
    label(ctx, F.Regular14, '1 shield left', 50, 202, 140, 'center', C.shield);
  }
}

/** FreshStartScreen: the sun rises (y 116 → 80) over the hills. p: 0..1 */
export function freshStart(ctx, s) {
  const sunY = lerp(116, 80, E.outCubic(clamp(s.p ?? 1)));
  ctx.save();
  ctx.beginPath();
  ctx.rect(0, 0, 240, 116);
  ctx.clip();
  const rays = [[16, 0], [11, 11], [0, 16], [-11, 11], [-16, 0], [-11, -11], [0, -16], [11, -11]];
  rays.forEach(([dx, dy]) => line(ctx, [120 + (dx * 32) / 16, sunY + (dy * 32) / 16], [120 + (dx * 41) / 16, sunY + (dy * 41) / 16], 2, C.lemon));
  disc(ctx, 120, sunY, 24, C.amber);
  tri(ctx, [-20, 116], [40, 90], [110, 116], C.grayDark);
  tri(ctx, [-10, 116], [80, 74], [170, 116], C.tealDark);
  tri(ctx, [70, 116], [170, 70], [250, 116], C.teal);
  ctx.restore();
  label(ctx, F.SemiBold25, 'Fresh start.', 20, 122, 200, 'center', C.white);
  ['Your climb is safe: 6 of', '14 on Ben Nevis. A new', 'streak starts today.'].forEach((ln, i) => label(ctx, F.Regular14, ln, 36, 156 + i * 18, 168, 'center', C.gray));
}

/** The menu (MenuScreen): a WheelMenu with the app's items. */
export function menu(ctx, s) {
  title(ctx, 'HybridX Streak', C.white, C.grayDark, F.Italic18);
  wheel(ctx, [{ label: 'This week', tip: '3 of 3 counted' }, { label: 'Log a session' }, { label: 'Trophy case' }, { label: 'Settings' }], s.sel || 0, s.shift || 0);
  buttons(ctx, { l1: 'white', l2: 'white', r1: 'amber', r2: 'white' });
}

/** The week screen: each session with its minutes and app. */
export function weekScreen(ctx, s) {
  title(ctx, 'This week');
  const rows = s.rows || [];
  rows.slice(0, 4).forEach((r, i) => {
    const y = 52 + i * 40;
    const a = r.a ?? 1;
    label(ctx, F.Medium18, r.kind, 0, y, 240, 'center', C.white, a);
    label(ctx, F.Regular14, r.detail, 0, y + 21, 240, 'center', r.counts ? C.lime : C.gray, a);
  });
  buttons(ctx, { l1: 'white', l2: 'white', r1: 'amber', r2: 'white' });
}

/** The trophy case: a summit per line. */
export function trophyScreen(ctx, s) {
  title(ctx, 'Trophy case');
  const rows = s.rows || [];
  rows.slice(0, 4).forEach((r, i) => {
    const y = 52 + i * 40;
    label(ctx, F.Medium18, r.name, 0, y, 240, 'center', r.done ? C.lime : C.white);
    label(ctx, F.Regular14, r.detail, 0, y + 21, 240, 'center', C.gray);
  });
  buttons(ctx, { l1: 'white', l2: 'white', r2: 'white' });
}

/** Settings: the weekly target. */
export function targetScreen(ctx, s) {
  title(ctx, 'Settings');
  wheel(ctx, [{ label: 'Weekly target', tip: s.value || '3' }, { label: 'Week starts', tip: 'Monday' }], 0, 0);
  buttons(ctx, { l1: 'white', l2: 'white', r1: 'amber', r2: 'white' });
}

export { CONF };
