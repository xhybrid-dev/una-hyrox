// The HybridX brand kit shared by all three films: palette, the X mark, type
// styles, the title sting, the HUD, the chevron wipe and the end card.
//
// The X mark is a vector rebuild of Jon's mark, fitted to the committed
// hybridx-race/Resources/icon_60x60.png (97% overlap at icon size): a ">"
// chevron with a flat tip, and two right-hand arms that stop short of it,
// leaving a gap. The gap is where each app's colour lives in these films.

import { W, H, clamp, lerp, prog, E, ep, spring, hash, TAU } from './core.mjs';
import { text, kinetic, measure, poly, rgba, light, layer } from './gfx.mjs';

// --- Palette ------------------------------------------------------------------
// Watch colours are the SDK's 64-colour palette as the simulator and screen
// show them (0x40 steps rendered as 85, 170, 255). Film chrome adds greys.

export const C = {
  black: '#000000',
  white: '#FFFFFF',
  gray: '#AAAAAA', // GRAY
  grayDark: '#555555', // GRAY_DARK
  // Race
  cyan: '#55FFFF', // CYAN: runs
  lemon: '#FFFF55', // LEMON: stations
  orchid: '#FF55FF', // ORCHID: Roxzone
  amber: '#FFAA00', // YELLOW_DARK: the amber button hint
  yellow: '#FFFF00',
  chartreuse: '#55FF00',
  red: '#FF0000',
  // Streak
  teal: '#00AAAA',
  tealDark: '#005555',
  lime: '#AAFF00',
  shield: '#55FFFF',
  // Trail
  magenta: '#FF00FF',
  pink: '#FF55FF',
  viridian: '#00AA55',
  greenDark: '#005500',
  sageDark: '#55AA55',
  // Film chrome
  ink: '#07080A',
  ink2: '#0E1013',
  ink3: '#171A1E',
  rule: '#23272C',
  mute: '#6E747C',
  soft: '#9BA1A8',
};

// --- Type styles ----------------------------------------------------------------

export const T = {
  hero: (size = 150, color = C.white) => ({ size, weight: 600, tracking: -0.025, color }),
  heroLight: (size = 150, color = C.white) => ({ size, weight: 300, tracking: -0.02, color }),
  head: (size = 84, color = C.white) => ({ size, weight: 600, tracking: -0.015, color }),
  sub: (size = 34, color = C.soft) => ({ size, weight: 300, tracking: 0, color }),
  body: (size = 30, color = C.soft) => ({ size, weight: 400, tracking: 0, color }),
  label: (size = 18, color = C.mute) => ({ size, weight: 500, mono: true, tracking: 0.22, color }),
  mono: (size = 24, color = C.white) => ({ size, weight: 400, mono: true, tracking: 0.02, color }),
  num: (size = 200, color = C.white) => ({ size, weight: 300, tracking: -0.03, color }),
};

// --- The X mark ---------------------------------------------------------------

export const XM = { w: 1.3346, t: 0.3347, tip: 0.4054, gap: 0.0812, cut: 0.3489 };
const yc = (XM.w - XM.t - (XM.t + XM.gap)) / 2;
export const XPARTS = {
  chev: [
    [0, 0], [XM.t, 0], [XM.t + XM.tip, XM.tip], [XM.t + XM.tip, 1 - XM.tip],
    [XM.t, 1], [0, 1], [0.5, 0.5],
  ],
  armU: [[XM.w - XM.t, 0], [XM.w, 0], [XM.w - XM.cut, XM.cut], [XM.t + XM.cut + XM.gap, XM.cut], [XM.t + yc + XM.gap, yc]],
  armD: null,
};
XPARTS.armD = XPARTS.armU.map(([x, y]) => [x, 1 - y]);
/** Where the arms meet the gap, in mark units: the sparks come from here. */
export const XJOINTS = {
  up: [XM.t + (yc + XM.cut) / 2 + XM.gap / 2, (yc + XM.cut) / 2],
  down: [XM.t + (yc + XM.cut) / 2 + XM.gap / 2, 1 - (yc + XM.cut) / 2],
};

function xpts(part, cx, cy, h, dx = 0, dy = 0) {
  return part.map(([u, v]) => [cx + (u - XM.w / 2) * h + dx, cy + (v - 0.5) * h + dy]);
}

/**
 * The X mark centred on (cx, cy), h pixels tall. o may move or fade each part:
 * { chev: {dx, dy, a}, armU: {...}, armD: {...} }.
 */
export function xMark(ctx, cx, cy, h, color = C.white, o = {}) {
  ctx.save();
  ctx.fillStyle = color;
  const base = ctx.globalAlpha;
  for (const k of ['chev', 'armU', 'armD']) {
    const p = o[k] || {};
    const a = p.a ?? 1;
    if (a <= 0) continue;
    ctx.globalAlpha = base * a;
    poly(ctx, xpts(XPARTS[k], cx, cy, h, p.dx || 0, p.dy || 0));
    ctx.fill();
  }
  ctx.restore();
}

/** The mark drawn as an outline only (for ghosts and wireframes). */
export function xMarkOutline(ctx, cx, cy, h, color, width = 2) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = width;
  ctx.lineJoin = 'miter';
  for (const k of ['chev', 'armU', 'armD']) {
    poly(ctx, xpts(XPARTS[k], cx, cy, h));
    ctx.stroke();
  }
  ctx.restore();
}

/**
 * The title sting: the chevron is drawn on, the arms fly in and lock with a
 * spark of the app's colour in each gap, then the name comes up beside it.
 * t is seconds since the sting began. Lasts about 4 s before `out` starts.
 *
 * opt: { accent, accent2, name ('Race'), sub, out (0..1 exit progress),
 *        cx, cy, h, dir ('right'|'up'|'draw') }
 */
export function sting(ctx, t, opt) {
  const accent = opt.accent;
  const accent2 = opt.accent2 || accent;
  const h = opt.h ?? 150;
  const out = opt.out ?? 0;
  const lock = 0.95; // when the arms lock
  // Lockup geometry: mark on the left, words on the right, centred as one.
  const nameS = { size: h * 0.62, weight: 600, tracking: -0.02, color: C.white };
  const prodS = { size: h * 0.62, weight: 300, tracking: -0.02, color: C.white };
  const wHyb = measure(ctx, 'HybridX', nameS);
  const wSp = h * 0.16;
  const wProd = measure(ctx, opt.name, prodS);
  const markW = XM.w * h;
  const gapMW = h * 0.42;
  const total = markW + gapMW + wHyb + wSp + wProd;
  // The mark starts centred, then slides left to make room for the words.
  const slide = ep(t, 1.35, 2.25, E.inOutQuint);
  const cx0 = opt.cx ?? W / 2;
  const cy = opt.cy ?? H / 2;
  const markCx = lerp(cx0, cx0 - total / 2 + markW / 2, slide);
  // A small push on impact.
  const bump = 1 + 0.035 * Math.exp(-Math.max(0, t - lock) * 9) * (t > lock ? 1 : 0);
  const hh = h * bump * (1 - 0.06 * E.inQuad(out));

  ctx.save();
  ctx.globalAlpha *= 1 - E.inQuad(out);

  // Accent light behind the mark at the lock.
  const flash = t > lock ? Math.exp(-(t - lock) * 3.2) : 0;
  light(ctx, markCx, cy, h * 2.6, rgba(accent, 0.55), flash);

  // 1. The chevron, drawn on along its spine.
  const drawP = ep(t, 0.0, 0.62, E.inOutQuart);
  const L = layer('sting-chev');
  L.ctx.fillStyle = C.white;
  poly(L.ctx, xpts(XPARTS.chev, markCx, cy, hh));
  L.ctx.fill();
  L.ctx.globalCompositeOperation = 'destination-in';
  L.ctx.lineWidth = hh * 0.62;
  L.ctx.lineCap = 'butt';
  L.ctx.lineJoin = 'miter';
  L.ctx.strokeStyle = '#fff';
  const spine = [
    [markCx + (-XM.w / 2 - 0.1) * hh, cy + (-0.5 - 0.1) * hh],
    [markCx + (0.62 - XM.w / 2) * hh, cy],
    [markCx + (-XM.w / 2 - 0.1) * hh, cy + (0.5 + 0.1) * hh],
  ];
  // strokePart without importing it twice: straight segments.
  const segLen = Math.hypot(spine[1][0] - spine[0][0], spine[1][1] - spine[0][1]);
  const tot = segLen * 2;
  const e = drawP * tot;
  L.ctx.beginPath();
  L.ctx.moveTo(spine[0][0], spine[0][1]);
  if (e <= segLen) {
    const f = e / segLen;
    L.ctx.lineTo(lerp(spine[0][0], spine[1][0], f), lerp(spine[0][1], spine[1][1], f));
  } else {
    const f = (e - segLen) / segLen;
    L.ctx.lineTo(spine[1][0], spine[1][1]);
    L.ctx.lineTo(lerp(spine[1][0], spine[2][0], f), lerp(spine[1][1], spine[2][1], f));
  }
  if (drawP > 0) L.ctx.stroke();
  L.ctx.globalCompositeOperation = 'source-over';
  ctx.drawImage(L.canvas, 0, 0);

  // 2. The arms fly in along their own axes and lock.
  const armA = prog(t, 0.5, 0.62);
  const far = h * 3.2;
  // The upper arm travels up-right to down-left, the lower down-right to up-left.
  const ghosts = t < lock + 0.05 ? 5 : 1;
  for (let g = ghosts - 1; g >= 0; g--) {
    const lag = g * 0.035;
    const pg = ep(t - lag, 0.5, lock, E.outExpo);
    const d = (1 - pg) * far;
    const a = armA * (g === 0 ? 1 : 0.22 * (1 - g / ghosts));
    const col = g === 0 ? C.white : accent;
    xMark(ctx, markCx, cy, hh, col, {
      chev: { a: 0 },
      armU: { dx: d * 0.7071, dy: -d * 0.7071, a },
      armD: { dx: d * 0.7071, dy: d * 0.7071, a },
    });
    if (g === 0) break;
  }

  // 3. The spark in each gap at the lock: a hairline of the accent along the
  // gap, and a few short streaks flying out.
  if (t > lock - 0.02) {
    const st = t - lock;
    const glowA = Math.exp(-st * 2.4);
    for (const [key, col, sgn] of [['up', accent, -1], ['down', accent2, 1]]) {
      const [u, v] = XJOINTS[key];
      const jx = markCx + (u - XM.w / 2) * hh;
      const jy = cy + (v - 0.5) * hh;
      light(ctx, jx, jy, hh * 0.9, rgba(col, 0.9), glowA);
      // Hairline filling the gap: a short line at 45 degrees.
      ctx.save();
      ctx.strokeStyle = col;
      ctx.globalAlpha *= clamp(glowA * 1.6);
      ctx.lineWidth = Math.max(1.5, hh * 0.022);
      const gl = hh * 0.1 * (1 + st * 0.5);
      ctx.beginPath();
      ctx.moveTo(jx - gl * 0.7071, jy + sgn * gl * 0.7071 * -1);
      ctx.lineTo(jx + gl * 0.7071, jy - sgn * gl * 0.7071 * -1);
      ctx.stroke();
      ctx.restore();
      // Streaks.
      for (let i = 0; i < 7; i++) {
        const ang = (sgn < 0 ? -Math.PI / 4 : Math.PI / 4) + (hash(i, key.length) - 0.5) * 1.6;
        const sp = hh * (0.9 + hash(i + 9, 3) * 1.4);
        const r0 = sp * E.outCubic(clamp(st / 0.5));
        const r1 = r0 * 0.55;
        const a = clamp(1 - st / 0.55);
        if (a <= 0) continue;
        ctx.save();
        ctx.strokeStyle = col;
        ctx.globalAlpha *= a;
        ctx.lineWidth = Math.max(1, hh * 0.012);
        ctx.beginPath();
        ctx.moveTo(jx + Math.cos(ang) * r1, jy + Math.sin(ang) * r1);
        ctx.lineTo(jx + Math.cos(ang) * r0, jy + Math.sin(ang) * r0);
        ctx.stroke();
        ctx.restore();
      }
    }
  }

  // 4. The words: "HybridX" then the product, rising behind a mask.
  const wx = markCx + markW / 2 + gapMW;
  const baseY = cy + nameS.size * 0.36;
  const pIn = prog(t, 1.75, 2.6);
  const pIn2 = prog(t, 1.95, 2.8);
  kinetic(ctx, 'HybridX', wx, baseY, nameS, pIn, 0, 'rise');
  kinetic(ctx, opt.name, wx + wHyb + wSp, baseY, prodS, pIn2, 0, 'rise');
  if (opt.sub) {
    const subS = T.label(Math.round(h * 0.15), C.soft);
    kinetic(ctx, opt.sub, wx + 2, baseY + h * 0.46, subS, prog(t, 2.35, 3.3), 0, 'scramble', { t });
  }
  ctx.restore();
}

// --- The HUD ------------------------------------------------------------------

/**
 * The frame furniture every film shares. opt: { app ('RACE'), accent, alpha,
 * topRight (string), topRightColor, bottomRight (string), progress (0..1),
 * ticks ([0..1] chapter marks), tickColors }
 */
export function hud(ctx, t, opt) {
  const a = opt.alpha ?? 1;
  if (a <= 0) return;
  ctx.save();
  ctx.globalAlpha *= a;
  const m = 64;
  // Top left: the mark and the app.
  xMark(ctx, m + 14, m + 6, 20, C.white);
  const lab = T.label(16, C.soft);
  let x = m + 42;
  x += text(ctx, 'HYBRIDX', x, m + 12, lab) + 14;
  text(ctx, opt.app, x, m + 12, { ...lab, color: opt.accent });
  // Top right: context.
  if (opt.topRight) {
    text(ctx, opt.topRight, W - m, m + 12, { ...T.label(16, opt.topRightColor || C.soft), align: 'right' });
  }
  if (opt.topRight2) {
    text(ctx, opt.topRight2, W - m, m + 36, { ...T.label(14, C.mute), align: 'right' });
  }
  // Bottom: the film's own progress line with chapter ticks.
  const y = H - m;
  const x0 = m, x1 = m + 420;
  ctx.fillStyle = C.rule;
  ctx.fillRect(x0, y, x1 - x0, 2);
  const pr = clamp(opt.progress ?? 0);
  ctx.fillStyle = opt.accent;
  ctx.fillRect(x0, y, (x1 - x0) * pr, 2);
  if (opt.ticks) {
    opt.ticks.forEach((tk, i) => {
      const tx = lerp(x0, x1, tk);
      ctx.fillStyle = tk <= pr ? (opt.tickColors ? opt.tickColors[i] : opt.accent) : C.rule;
      ctx.fillRect(tx - 1, y - 5, 2, 12);
    });
  }
  if (opt.bottomLeft) text(ctx, opt.bottomLeft, x1 + 24, y + 6, T.label(14, C.mute));
  if (opt.bottomRight) {
    text(ctx, opt.bottomRight, W - m, y + 8, { ...T.mono(24, C.white), align: 'right' });
  }
  if (opt.bottomRightLabel) {
    text(ctx, opt.bottomRightLabel, W - m, y - 26, { ...T.label(13, C.mute), align: 'right' });
  }
  ctx.restore();
}

// --- The chevron wipe ---------------------------------------------------------

/**
 * A wipe shaped like the mark's chevron. Draws `next` into the region the
 * chevron has passed, and a band of `color` on its leading edge.
 * dir: 'right' (pointing right, sweeping right), 'up' (pointing up, sweeping
 * up) or 'diag' (pointing up-right).
 */
export function chevronWipe(ctx, p, next, { dir = 'right', color = C.white, band = 70, band2 = 0, color2 = null } = {}) {
  if (p <= 0) return;
  if (p >= 1) {
    next(ctx);
    return;
  }
  const e = E.inOutQuart(p);
  const base = ctx.getTransform();
  ctx.save();
  // Work in a frame where the chevron points along +x.
  let span, len, top;
  if (dir === 'up') {
    ctx.translate(0, H);
    ctx.rotate(-Math.PI / 2);
    span = W; len = H; top = 0;
  } else if (dir === 'diag') {
    const d = Math.hypot(W, H);
    ctx.translate(W / 2, H / 2);
    ctx.rotate(-Math.PI / 4);
    ctx.translate(-d / 2, -d / 2);
    span = d; len = d; top = 0;
  } else {
    span = H; len = W; top = 0;
  }
  const x0 = lerp(-band - band2, len + span / 2 + band + band2, e);
  const mid = top + span / 2;
  const bot = top + span;
  const region = (xTip) => [
    [-4000, top - 10], [xTip - span / 2 - 10, top - 10], [xTip, mid], [xTip - span / 2 - 10, bot + 10], [-4000, bot + 10],
  ];
  // The next scene, behind the band, drawn in the caller's own frame.
  ctx.save();
  poly(ctx, region(x0 - band - band2));
  ctx.clip();
  ctx.setTransform(base);
  next(ctx);
  ctx.restore();
  // The band(s) on the leading edge.
  const bandPoly = (xa, xb) => [
    [xb - span / 2 - 10, top - 10], [xa - span / 2 - 10, top - 10], [xa, mid], [xa - span / 2 - 10, bot + 10],
    [xb - span / 2 - 10, bot + 10], [xb, mid],
  ];
  ctx.fillStyle = color;
  poly(ctx, bandPoly(x0, x0 - band));
  ctx.fill();
  if (band2 > 0) {
    ctx.fillStyle = color2 || C.white;
    poly(ctx, bandPoly(x0 - band, x0 - band - band2));
    ctx.fill();
  }
  ctx.restore();
}

/**
 * Trail's wipe: a line draws across the frame and the next scene flows in
 * behind it, widening in its wake. p: 0..1.
 */
export function lineWipe(ctx, p, next, { color = C.white, seed = 1 } = {}) {
  if (p <= 0) return;
  if (p >= 1) {
    next(ctx);
    return;
  }
  const e = E.inOutCubic(p);
  const curve = [];
  for (let x = -300; x <= W + 1400; x += 16) {
    const y = H * 0.64 - (x / W) * H * 0.22 + 70 * Math.sin(x / 210 + seed) + 26 * Math.sin(x / 71 + seed * 2.3);
    curve.push([x, y]);
  }
  const lens = [0];
  for (let i = 1; i < curve.length; i++) lens.push(lens[i - 1] + Math.hypot(curve[i][0] - curve[i - 1][0], curve[i][1] - curve[i - 1][1]));
  const total = lens[lens.length - 1];
  const tipL = e * total;
  const base = ctx.getTransform();
  ctx.save();
  ctx.beginPath();
  for (let i = 0; i < curve.length; i++) {
    const behind = tipL - lens[i];
    if (behind <= 0) break;
    const r = Math.min(1500, behind * 0.9);
    ctx.moveTo(curve[i][0] + r, curve[i][1]);
    ctx.arc(curve[i][0], curve[i][1], r, 0, TAU);
  }
  ctx.clip();
  ctx.setTransform(base);
  next(ctx);
  ctx.restore();
  // The line itself, with a bright tip.
  let tip = curve[0];
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = 5;
  ctx.lineCap = 'round';
  ctx.lineJoin = 'round';
  ctx.shadowColor = rgba(color, 0.9);
  ctx.shadowBlur = 20;
  ctx.beginPath();
  for (let i = 0; i < curve.length; i++) {
    if (lens[i] > tipL) {
      const f = (tipL - lens[i - 1]) / (lens[i] - lens[i - 1]);
      tip = [lerp(curve[i - 1][0], curve[i][0], f), lerp(curve[i - 1][1], curve[i][1], f)];
      ctx.lineTo(tip[0], tip[1]);
      break;
    }
    if (i) ctx.lineTo(curve[i][0], curve[i][1]);
    else ctx.moveTo(curve[i][0], curve[i][1]);
  }
  ctx.stroke();
  ctx.restore();
  ctx.fillStyle = C.white;
  ctx.beginPath();
  ctx.arc(tip[0], tip[1], 8, 0, TAU);
  ctx.fill();
}

// --- The end card ---------------------------------------------------------------

const FAMILY = [
  { id: 'race', name: 'Race', color: C.lemon, color2: C.cyan },
  { id: 'streak', name: 'Streak', color: C.lime },
  { id: 'trail', name: 'Trail', color: C.pink },
];

/**
 * The closing card, the same in every film: the lockup, "for UNA Watch", the
 * family of three with this film's app lit, and the address.
 * t: seconds since the card began.
 */
export function endCard(ctx, t, opt) {
  const { app, accent, accent2 = accent, name } = opt;
  const h = opt.h ?? 170;
  const cy = H / 2 - 80;
  // Lockup (static version of the sting's final frame, rebuilt so it can be
  // animated independently here).
  const nameS = { size: h * 0.62, weight: 600, tracking: -0.02, color: C.white };
  const prodS = { size: h * 0.62, weight: 300, tracking: -0.02, color: C.white };
  const wHyb = measure(ctx, 'HybridX', nameS);
  const wSp = h * 0.16;
  const wProd = measure(ctx, name, prodS);
  const markW = XM.w * h;
  const gapMW = h * 0.42;
  const total = markW + gapMW + wHyb + wSp + wProd;
  const left = W / 2 - total / 2;
  const markCx = left + markW / 2;

  const pMark = ep(t, 0, 0.9, E.outExpo);
  const far = h * 1.6 * (1 - pMark);
  xMark(ctx, markCx, cy, h, C.white, {
    chev: { dx: -far, a: prog(t, 0, 0.3) },
    armU: { dx: far * 0.7071, dy: -far * 0.7071, a: prog(t, 0.05, 0.35) },
    armD: { dx: far * 0.7071, dy: far * 0.7071, a: prog(t, 0.05, 0.35) },
  });
  // The gaps glow softly in the app's colours and breathe.
  const br = 0.55 + 0.45 * Math.sin(t * 2.2);
  for (const [key, col] of [['up', accent], ['down', accent2]]) {
    const [u, v] = XJOINTS[key];
    light(ctx, markCx + (u - XM.w / 2) * h, cy + (v - 0.5) * h, h * 0.55, rgba(col, 0.75),
      prog(t, 0.6, 1.2) * br * (opt.fade ?? 1));
  }
  const wx = markCx + markW / 2 + gapMW;
  const baseY = cy + nameS.size * 0.36;
  kinetic(ctx, 'HybridX', wx, baseY, nameS, prog(t, 0.35, 1.0), 0, 'rise');
  kinetic(ctx, name, wx + wHyb + wSp, baseY, prodS, prog(t, 0.5, 1.15), 0, 'rise');

  // "for UNA Watch": a nominative reference, set plainly (SDK TRADEMARK.md).
  const sub = T.label(22, C.soft);
  kinetic(ctx, 'FOR UNA WATCH', wx + 2, baseY + h * 0.5, sub, prog(t, 0.9, 1.7), 0, 'scramble', { t });

  // The family.
  const fy = H / 2 + 170;
  const lab = T.label(18, C.mute);
  const items = FAMILY.map((f) => ({ ...f, w: measure(ctx, f.name.toUpperCase(), lab) }));
  const dotW = 18, spacing = 64;
  const rowW = items.reduce((s, f) => s + f.w + dotW, 0) + spacing * (items.length - 1);
  let x = W / 2 - rowW / 2;
  items.forEach((f, i) => {
    const pa = ep(t, 1.2 + i * 0.12, 1.9 + i * 0.12, E.outCubic);
    const on = f.id === app;
    ctx.save();
    ctx.globalAlpha *= pa;
    const col = on ? f.color : C.rule;
    ctx.fillStyle = col;
    ctx.fillRect(x, fy - 12, 8, 8);
    if (f.color2) {
      ctx.fillStyle = on ? f.color2 : C.rule;
      ctx.fillRect(x, fy - 3, 8, 8);
      ctx.fillStyle = col;
    }
    text(ctx, f.name.toUpperCase(), x + dotW, fy, { ...lab, color: on ? C.white : C.mute });
    ctx.restore();
    x += f.w + dotW + spacing;
  });
  // A hairline between the lockup and the family.
  const rl = ep(t, 1.0, 1.8, E.inOutQuart) * 520;
  ctx.fillStyle = C.rule;
  ctx.fillRect(W / 2 - rl / 2, H / 2 + 110, rl, 1);

  // The address.
  const addr = T.label(18, C.soft);
  kinetic(ctx, 'HYBRIDX.CLUB', W / 2, H - 110, { ...addr, align: 'center' }, prog(t, 1.7, 2.5), 0, 'fade');
  if (opt.note) {
    kinetic(ctx, opt.note, W / 2, H - 80, { ...T.label(14, C.mute), align: 'center' }, prog(t, 2.0, 2.8), 0, 'fade');
  }
}

export { FAMILY };
