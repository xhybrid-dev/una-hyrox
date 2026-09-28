// HybridX Streak, the reel: "The climb" in 34 seconds, portrait.
//
// 112 BPM, 16 bars (34.3 s), 1080 x 1920 for Instagram Reels and adverts.
// The portrait frame is made for climbing: after the sting, a week banked, a
// shield spent, then the camera rises through all five mountains, 104 weeks
// of them, to Everest's summit.

import { clamp, lerp, prog, ep, E, hash, tempo, TAU } from '../lib/core.mjs';
import { text, kinetic, rgba, light, roundRect } from '../lib/gfx.mjs';
import { C, T, sting, chevronWipe } from '../lib/brand.mjs';
import { watch, setWatchModel, buttonPos, ripple, buzz } from '../lib/watch.mjs';
import { pressAt } from '../lib/film.mjs';
import * as S from '../lib/ui-streak.mjs';
import { RW as W, RH as H, SAFE, fit, reelHud, reelHead, reelEnd } from '../lib/reel.mjs';

const TM = tempo(112);
const B = (b, beats = 0) => TM.at(b, beats);
const LOCK = B(2);
const CH = { week: B(4), shield: B(6), climb: B(8), summit: B(12), end: B(14) };
const END = CH.end + 0.35;
const DURATION = B(16);
const WIPES = [CH.week, CH.shield, CH.climb];

// --- The sky ---------------------------------------------------------------------

const STARS = Array.from({ length: 300 }, (_, i) => ({
  x: hash(i, 11) * W, y: hash(i, 12) * H * 2, r: 0.6 + Math.pow(hash(i, 13), 6) * 2.6,
  tw: hash(i, 14) * TAU, depth: 0.25 + hash(i, 15) * 0.75,
}));

/** How far the camera has climbed; the montage rushes. */
let RUSH = 0;
function sky(ctx, t, alpha = 1) {
  if (alpha <= 0) return;
  const alt = t * 26 + RUSH;
  ctx.save();
  for (const s of STARS) {
    const y = ((s.y + alt * s.depth) % (H * 2)) - H / 2;
    if (y < -10 || y > H + 10) continue;
    ctx.globalAlpha = alpha * (0.55 + 0.45 * Math.sin(t * (0.8 + s.depth * 1.6) + s.tw)) * (0.35 + 0.65 * s.depth);
    ctx.fillStyle = s.r > 1.6 ? C.white : '#9aa3ad';
    ctx.fillRect(s.x, y, s.r, s.r);
  }
  ctx.restore();
  const g = ctx.createRadialGradient(W / 2, H * 1.15, 0, W / 2, H * 1.15, H * 0.8);
  g.addColorStop(0, rgba(C.teal, 0.18 * alpha));
  g.addColorStop(1, 'rgba(0,0,0,0)');
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, W, H);
}

function homeAt(t, over = {}) {
  return {
    climb: 1, climbed: 3, streak: 7, target: 3, sessions: 0, coach: 'Snowdon: 5 weeks to go', mood: 'soft',
    pulse: 1.5 + 1.5 * Math.sin((t * TAU) / 1.8), wave: Math.round(2 * Math.sin((t * TAU) / 0.76)), ...over,
  };
}

const CHIPS = [
  { kind: 'Run', detail: '32 MIN · RUNNING' },
  { kind: 'Strength', detail: '45 MIN · WORKOUT' },
  { kind: 'Ride', detail: '64 MIN · CYCLING' },
];

function chip(ctx, x, y, c, a = 1, scale = 1) {
  ctx.save();
  ctx.globalAlpha *= a;
  ctx.translate(x, y);
  ctx.scale(scale, scale);
  const w = 520, h = 140;
  ctx.fillStyle = '#0f1215';
  roundRect(ctx, -w / 2, -h / 2, w, h, 26);
  ctx.fill();
  ctx.strokeStyle = '#232a31';
  ctx.lineWidth = 2;
  ctx.stroke();
  ctx.fillStyle = C.lime;
  roundRect(ctx, -w / 2 + 24, -h / 2 + 28, 9, h - 56, 4);
  ctx.fill();
  text(ctx, c.kind, -w / 2 + 64, 6, T.head(56, C.white));
  text(ctx, c.detail, -w / 2 + 66, 46, T.label(19, C.soft));
  ctx.restore();
}

/** A chip slides in from the left, then arcs into the watch. */
function chipFlight(ctx, lt, c, t0, to) {
  const pin = ep(lt, t0 - 0.3, t0 + 0.3, E.outExpo);
  const fly = ep(lt, t0 + 0.6, t0 + 1.05, E.inOutCubic);
  if (pin <= 0 || fly >= 1) return;
  const sx = lerp(-400, W / 2, pin), sy = 1440;
  const x = lerp(sx, to[0], fly), y = lerp(sy, to[1], fly) - Math.sin(fly * Math.PI) * 160;
  chip(ctx, x, y, c, 1 - prog(fly, 0.75, 1), lerp(1, 0.25, fly));
}

// --- Scenes -------------------------------------------------------------------------

/** 0 to 3.3 s: every workout counts. Two sessions land from the first frame. */
function hook(ctx, t) {
  sky(ctx, t, 0.8);
  const q = prog(t, LOCK - 1.2, LOCK - 0.95);
  kinetic(ctx, 'Every workout', SAFE.left, 420, T.hero(128), prog(t, -0.3, 0.2), q, 'rise');
  kinetic(ctx, 'counts.', SAFE.left, 552, T.hero(128, C.lime), prog(t, 0.1, 0.55), q, 'rise');
  kinetic(ctx, 'FROM ANY APP · NOTHING TO START', SAFE.left + 4, 630, T.label(22, C.soft), prog(t, 0.4, 1.0), q, 'scramble', { t });
  const wo = { cx: W / 2, cy: 1040, d: 440, rot: 0 };
  const lands = [0.05 + 1.05, 1.2 + 1.05];
  const n = lands.filter((x) => t >= x).length;
  const since = n ? t - lands[n - 1] : 99;
  const bz = buzz(since, 5, 0.2);
  watch(ctx, {
    ...wo, cx: wo.cx + bz[0], cy: wo.cy + bz[1], alpha: 1 - q,
    screen: (s) => S.home(s, homeAt(t, {
      sessions: n, toast: n && since < 1.0 ? `+1 ${CHIPS[n - 1].kind} · ${CHIPS[n - 1].detail.split(' · ')[0].toLowerCase()}` : null,
      popIdx: n - 1, pop: since < 0.44 ? 4 * Math.sin(Math.PI * clamp(since / 0.44)) : 0,
      coach: n ? `${3 - n} more · ${4 - n} days left` : 'Snowdon: 5 weeks to go',
    })),
    bloom: 0.5,
  });
  CHIPS.slice(0, 2).forEach((c, i) => chipFlight(ctx, t, c, [0.05, 1.2][i], [wo.cx, wo.cy + 60]));
}

function title(ctx, t) {
  sky(ctx, t, 1 - prog(t, LOCK - 1.0, LOCK - 0.6) * 0.5);
  const out = prog(t, B(3, 2), B(4) - 0.1);
  sting(ctx, t - (LOCK - 0.95), { accent: C.lime, accent2: C.teal, name: 'Streak', sub: 'A WEEKLY STREAK FOR UNA WATCH', out, h: 150, cy: 900 });
  const lt = t - LOCK;
  if (lt > 0 && lt < 1.4) {
    const len = E.outCubic(clamp(lt / 0.9));
    ctx.save();
    ctx.strokeStyle = rgba(C.lime, 1 - prog(lt, 0.6, 1.4));
    ctx.lineWidth = 3;
    for (let i = 0; i < 8; i++) {
      const an = (i / 8) * TAU + Math.PI / 8;
      const r0 = 260 + len * 240, r1 = 260 + len * 560;
      ctx.beginPath();
      ctx.moveTo(W / 2 + Math.cos(an) * r0, 900 + Math.sin(an) * r0);
      ctx.lineTo(W / 2 + Math.cos(an) * r1, 900 + Math.sin(an) * r1);
      ctx.stroke();
    }
    ctx.restore();
  }
}

/** The third session lands: week complete, one step up the mountain. */
function week(ctx, t) {
  const lt = t - CH.week;
  sky(ctx, t, 0.8);
  const landT = 1.05 + 0.1;
  const glide = ep(lt, landT + 0.3, landT + 1.3, E.inOutCubic);
  const burstT = landT + 1.3;
  const d = 540;
  const wo = { cx: W / 2, cy: 1080, d, rot: 0 };
  const since = lt - landT;
  const bz = buzz(since, 6, 0.3);
  const k = (d * 0.86) / 240;
  const m = S.mountain(1, 118, 100);
  const here = S.stepPos(S.trail(m), 3, 8, glide);
  const hx = wo.cx + (here[0] - 120) * k, hy = wo.cy + (here[1] - 120) * k;
  const bl = lt - burstT;
  if (bl > 0) {
    const e = E.outExpo(clamp(bl / 1.2));
    const a = 1 - prog(bl, 0.5, 1.6);
    ctx.save();
    ctx.strokeStyle = rgba(C.lime, a);
    ctx.lineWidth = 4;
    for (let i = 0; i < 8; i++) {
      const an = (i / 8) * TAU;
      const r0 = 30 + e * 500, r1 = 30 + e * 1400;
      ctx.beginPath();
      ctx.moveTo(hx + Math.cos(an) * r0, hy + Math.sin(an) * r0);
      ctx.lineTo(hx + Math.cos(an) * r1, hy + Math.sin(an) * r1);
      ctx.stroke();
    }
    ctx.restore();
    light(ctx, hx, hy, 520, rgba(C.lime, 0.4), a);
  }
  kinetic(ctx, 'Week', SAFE.left, 440, T.hero(170, C.lime), prog(lt, landT + 0.1, landT + 0.55), 0, 'rise');
  kinetic(ctx, 'complete!', SAFE.left, 610, fit(ctx, 'complete!', T.hero(170, C.lime), W - 2 * SAFE.left), prog(lt, landT + 0.22, landT + 0.7), 0, 'rise');
  kinetic(ctx, 'ONE WEEK. ONE STEP UP.', SAFE.left + 4, 690, T.label(24, C.soft), prog(lt, burstT + 0.2, burstT + 0.9), 0, 'scramble', { t: lt });
  watch(ctx, {
    ...wo, cx: wo.cx + bz[0], cy: wo.cy + bz[1],
    screen: (s) => {
      const banked = lt > burstT + 1.0;
      S.home(s, homeAt(t, {
        sessions: lt > landT ? 3 : 2, climbed: 3, frac: glide,
        streak: banked ? 8 : lt > landT ? null : 7,
        words: lt > landT && !banked ? 'Week complete!' : 'week streak', wordsColor: lt > landT && !banked ? C.lime : C.white,
        toast: lt > landT && lt < landT + 1.1 ? '+1 Ride · 64 min' : null,
        coach: banked ? 'Week banked. Rest up.' : '1 more · 3 days left', mood: banked ? 'win' : 'soft',
        popIdx: 2, pop: since > 0 && since < 0.44 ? 4 * Math.sin((Math.PI * since) / 0.44) : 0,
        burst: bl > 0 && bl < 0.6 ? 12 * E.outCubic(bl / 0.5) : 0, burstA: 1 - prog(bl, 0.35, 0.6),
        mline: banked ? 'Snowdon · 4 weeks to go' : 'Snowdon · 5 weeks to go',
      }));
    },
    bloom: 0.6,
  });
  chipFlight(ctx, lt, CHIPS[2], 0.1, [wo.cx, wo.cy + 60]);
}

/** Life happens: a short week, a shield spent, the streak climbs on. */
function shield(ctx, t) {
  const lt = t - CH.shield;
  sky(ctx, t, 0.6);
  reelHead(ctx, lt, 'Life happens.', 'Miss a week with a shield in hand, and the streak climbs on.', SAFE.left, 420, { p0: 0.15, size: 128 });
  const pressT = 2.3;
  const saved = lt > pressT + 0.15;
  const wo = { cx: W / 2, cy: 940, d: 400, rot: 0 };
  const bz = buzz(lt - pressT - 0.05, 3, 0.25);
  watch(ctx, {
    ...wo, cx: wo.cx + bz[0], cy: lerp(H + 400, wo.cy, ep(lt, 0.05, 0.8, E.outExpo)) + bz[1],
    screen: (s) => (lt < 1.0 ? S.home(s, homeAt(t, { streak: 9, sessions: 1, climb: 2, climbed: 2, coach: '2 more · 1 day left', mood: 'risk', mline: 'Ben Nevis · 12 weeks to go' })) : S.shieldScreen(s, { saved, glow: { r1: 0.5 + 0.5 * Math.sin(lt * 5) } })),
    bloom: 0.5, press: { r1: pressAt(lt - pressT) },
  });
  if (lt > pressT) ripple(ctx, ...buttonPos(wo, 'r1', 1.3), lt - pressT, C.chartreuse, { rings: 2, speed: 360, life: 0.7 });
  // Four weeks: three banked, one short, and the shield coming down over it.
  const wy = 1370, wx = SAFE.left + 30, gw = 234;
  for (let w = 0; w < 4; w++) {
    const p = ep(lt, 0.4 + w * 0.12, 0.9 + w * 0.12, E.outCubic);
    text(ctx, `WEEK ${w + 6}`, wx + w * gw - 16, wy + 70, { ...T.label(18, C.mute), alpha: p });
    for (let s = 0; s < 3; s++) {
      const x = wx + w * gw + s * 50;
      ctx.save();
      ctx.globalAlpha *= p;
      if (w < 3 || s === 0) {
        ctx.fillStyle = C.lime;
        ctx.beginPath();
        ctx.arc(x, wy, 17, 0, TAU);
        ctx.fill();
      } else {
        ctx.strokeStyle = C.gray;
        ctx.lineWidth = 4;
        ctx.beginPath();
        ctx.arc(x, wy, 15, 0, TAU);
        ctx.stroke();
      }
      ctx.restore();
    }
  }
  const sp = ep(lt, pressT + 0.1, pressT + 0.7, E.outBack);
  if (sp > 0) {
    const sx = wx + 3 * gw + 50;
    S.shieldGlyph(ctx, sx, wy - 150 - (1 - sp) * 80, 1.4);
    light(ctx, sx, wy - 70, 260, rgba(C.shield, 0.35), sp);
  }
  kinetic(ctx, 'STREAK SAVED · 1 SHIELD LEFT', SAFE.left, 1525, T.label(22, C.lime), prog(lt, pressT + 0.5, pressT + 1.0), 0, 'scramble', { t: lt });
}

// The climb: five mountains stacked up the frame, one step per week.
const STACK = (() => {
  const ks = [2.2, 2.5, 2.8, 3.1, 3.6];
  const gaps = [0, 820, 860, 900, 960];
  let base = 360;
  return S.CLIMBS.map((c, i) => {
    base += gaps[i];
    const m = S.mountain(c.shape, 118, 100);
    return { ...c, k: ks[i], base, cx: W / 2 + [-150, 140, -130, 130, 0][i], tr: S.trail(m) };
  });
})();

function stepWorld(i, s) {
  const st = STACK[i];
  const p = S.stepPos(st.tr, s, st.steps);
  return [st.cx + (p[0] - 120) * st.k, st.base + (118 - p[1]) * st.k];
}

const WEEK_STEPS = STACK.flatMap((st, i) => Array.from({ length: st.steps }, (_, s) => ({ i, s: s + 1 })));

function bigScene(ctx, o, cx, baseY, k) {
  ctx.save();
  ctx.translate(cx - 120 * k, baseY - 118 * k);
  ctx.scale(k, k);
  S.scene(ctx, { stars: false, far: false, ...o });
  ctx.restore();
}

const CLIMB = { fill: [0.15, 2.3], fly: 2.3, rise: [2.9, 8.2] };

function climbCam(lt) {
  const climb = ep(lt, CLIMB.rise[0], CLIMB.rise[1], E.inOutSine);
  const camEnd = STACK[4].base + 118 * STACK[4].k - 0.72 * H;
  return { climb, camY: lerp(0, camEnd, climb) };
}

function climbScene(ctx, t) {
  const lt = t - CH.climb;
  const { climb, camY } = climbCam(lt);
  RUSH = climb * 4200;
  sky(ctx, t, 1);
  const toScreen = ([x, y]) => [x, H - (y - camY)];
  // The grid: 8 x 13 = 104 weeks, two years.
  const pitch = 64, cols = 13;
  const gx0 = W / 2 - (cols * pitch - 12) / 2, gy0 = 700;
  const fillP = prog(lt, CLIMB.fill[0], CLIMB.fill[1]);
  const filled = Math.floor(E.inQuad(fillP) * 104 + (fillP >= 1 ? 1 : 0));
  const gridA = 1 - prog(lt, CLIMB.fly + 1.4, CLIMB.fly + 1.9);
  kinetic(ctx, 'Two years.', SAFE.left, 440, T.hero(140), prog(lt, 0, 0.4), prog(lt, CLIMB.fly, CLIMB.fly + 0.4), 'rise');
  kinetic(ctx, '104 weeks.', SAFE.left, 580, T.hero(140, C.lime), prog(lt, 0.15, 0.55), prog(lt, CLIMB.fly, CLIMB.fly + 0.4), 'rise');
  // The mountains.
  STACK.forEach((st, i) => {
    const [, sy] = toScreen([st.cx, st.base]);
    if (sy - 118 * st.k > H + 50 || sy < -100) return;
    const appear = ep(lt, CLIMB.fly + 0.2 + i * 0.12, CLIMB.fly + 1.2 + i * 0.12, E.outCubic);
    ctx.save();
    ctx.globalAlpha *= appear;
    bigScene(ctx, { shape: st.shape, climbed: 0, steps: 999, flag: true, wave: Math.round(2 * Math.sin(t * 8 + i)) }, st.cx, sy, st.k);
    ctx.restore();
    // The name, across from the mountain as the camera passes it.
    const nameY = sy - 64 * st.k;
    const left = st.cx >= W / 2;
    const vis = clamp(1.7 - Math.abs(nameY - H * 0.5) / (H * 0.2)) * appear * ep(lt, CLIMB.rise[0], CLIMB.rise[0] + 0.5, E.outCubic);
    if (vis > 0) {
      const nx = left ? SAFE.left : W - SAFE.left;
      const ns = fit(ctx, st.name, { ...T.head(84, C.white), align: left ? 'left' : 'right' }, 520);
      text(ctx, st.name, nx, nameY, { ...ns, alpha: vis });
      text(ctx, `${st.weeks} WEEKS`, nx + (left ? 4 : -4), nameY + 50, { ...T.label(22, C.lime), align: left ? 'left' : 'right', alpha: vis });
    }
  });
  // The weeks: squares filling the grid, then flying to their steps.
  const climberWeek = Math.floor(lerp(0, 104, clamp(climb * 1.02)));
  for (let w = 0; w < 104; w++) {
    if (w >= filled && fillP < 1) continue;
    const g = [gx0 + (w % cols) * pitch + 26, gy0 + Math.floor(w / cols) * pitch + 26];
    const fp = ep(lt, CLIMB.fly + w * 0.008, CLIMB.fly + 1.1 + w * 0.008, E.inOutCubic);
    const target = toScreen(stepWorld(WEEK_STEPS[w].i, WEEK_STEPS[w].s));
    const x = lerp(g[0], target[0], fp), y = lerp(g[1], target[1], fp);
    const size = lerp(52, 16, fp);
    const fresh = lt - (CLIMB.fill[0] + (CLIMB.fill[1] - CLIMB.fill[0]) * Math.sqrt((w + 1) / 104));
    const flash = fillP < 1 && fresh > 0 && fresh < 0.25 ? 1 - fresh / 0.25 : 0;
    ctx.save();
    ctx.globalAlpha *= fp < 1 ? Math.max(gridA, fp) : 1;
    ctx.fillStyle = flash > 0.5 ? C.white : C.lime;
    ctx.translate(x, y);
    ctx.rotate((fp * Math.PI) / 4);
    ctx.fillRect(-size / 2, -size / 2, size, size);
    ctx.restore();
    if (fp >= 1 && w > 0 && w <= climberWeek && WEEK_STEPS[w - 1].i === WEEK_STEPS[w].i) {
      const a = toScreen(stepWorld(WEEK_STEPS[w - 1].i, WEEK_STEPS[w - 1].s));
      ctx.strokeStyle = rgba(C.lime, 0.85);
      ctx.lineWidth = 6;
      ctx.beginPath();
      ctx.moveTo(a[0], a[1]);
      ctx.lineTo(target[0], target[1]);
      ctx.stroke();
    }
  }
  // The climber.
  if (lt > CLIMB.rise[0] - 0.2) {
    const ws = WEEK_STEPS[Math.min(103, climberWeek)];
    const [cx, cy] = toScreen(stepWorld(ws.i, ws.s));
    ctx.strokeStyle = C.lime;
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.arc(cx, cy, 30 * (1 + 0.25 * Math.sin(t * 6)), 0, TAU);
    ctx.stroke();
    ctx.fillStyle = C.white;
    ctx.beginPath();
    ctx.arc(cx, cy, 16, 0, TAU);
    ctx.fill();
    ctx.fillStyle = C.lime;
    ctx.beginPath();
    ctx.arc(cx, cy, 11, 0, TAU);
    ctx.fill();
    light(ctx, cx, cy, 160, rgba(C.lime, 0.35), 1);
  }
  // The week counter, large, low on the left.
  const ca = ep(lt, CLIMB.rise[0], CLIMB.rise[0] + 0.5, E.outCubic);
  if (ca > 0) {
    const wk = Math.min(104, Math.max(1, climberWeek + 1));
    text(ctx, 'WEEK', SAFE.left, 330, { ...T.label(22, C.soft), alpha: ca });
    text(ctx, String(wk).padStart(3, '0'), SAFE.left - 6, 450, { ...T.num(130, C.lime), alpha: ca });
  }
}

/** Everest: the summit, and the confetti. */
function summit(ctx, t) {
  const lt = t - CH.summit;
  RUSH = 4200;
  sky(ctx, t, 1);
  const wIn = ep(lt, 0.0, 0.9, E.outExpo);
  let bz = [0, 0];
  [0.1, 0.35].forEach((p0) => {
    const b = buzz(lt - p0, 8, 0.3);
    bz = [bz[0] + b[0], bz[1] + b[1]];
  });
  const wo = { cx: W / 2 + bz[0], cy: 1120 + bz[1], d: lerp(300, 520, wIn), rot: lerp(0.5, 0, wIn) };
  light(ctx, wo.cx, wo.cy, 700, rgba(C.lime, 0.2), wIn);
  watch(ctx, { ...wo, screen: (s) => S.summitScreen(s, { climb: 4, t: lt, next: 'Next: Everest again' }), bloom: 0.65 });
  kinetic(ctx, 'Summit!', W / 2, 520, { ...T.hero(200, C.white), align: 'center' }, prog(lt, 0.05, 0.5), 0, 'rise');
  kinetic(ctx, 'EVEREST · 104 WEEKS', W / 2, 610, { ...T.label(28, C.lime), align: 'center' }, prog(lt, 0.6, 1.3), 0, 'scramble', { t: lt });
  confetti(ctx, lt - 0.15, 170, 1, 23);
}

function confetti(ctx, t, n, alpha, seed) {
  ctx.save();
  for (let i = 0; i < n; i++) {
    const tt = t - hash(i, seed + 1) * 1.2;
    if (tt < 0) continue;
    const y = -40 + tt * (380 + hash(i, seed + 2) * 520);
    if (y > H + 40) continue;
    const x = hash(i, seed) * W + Math.sin(tt * (1.5 + hash(i, seed + 3) * 2) + i) * 40;
    const flip = Math.abs(Math.cos(tt * (3 + hash(i, seed + 5) * 4) + i));
    ctx.globalAlpha = alpha;
    ctx.fillStyle = S.CONF[i % 4];
    ctx.save();
    ctx.translate(x, y);
    ctx.rotate(tt * (2 + hash(i, seed + 4) * 5) + i);
    ctx.fillRect(-7, -12 * flip, 14, 24 * flip + 1);
    ctx.restore();
  }
  ctx.restore();
}

// --- The reel ------------------------------------------------------------------------

const SCENES = [
  [0, LOCK - 0.95, hook], [LOCK - 0.95, CH.week, title], [CH.week, CH.shield, week], [CH.shield, CH.climb, shield],
  [CH.climb, CH.summit, climbScene], [CH.summit, CH.end, summit],
];

function sceneAt(t) {
  for (const [a, b, f] of SCENES) if (t >= a && t < b) return f;
  return null;
}

/** Weeks climbed, for the top bar: it only ever goes up. */
function weeksAt(t) {
  if (t < CH.week + 1.15) return 7;
  if (t < CH.climb + CLIMB.rise[0]) return 8;
  if (t >= CH.summit) return 104;
  return Math.max(8, Math.min(104, Math.floor(lerp(0, 104, clamp(climbCam(t - CH.climb).climb * 1.02))) + 1));
}

function draw(ctx, t) {
  setWatchModel('teal');
  RUSH = 0;
  let done = false;
  for (const tb of WIPES) {
    const p = prog(t, tb - 0.3, tb + 0.35);
    if (p > 0 && p < 1) {
      sceneAt(tb - 0.001)(ctx, Math.min(t, tb - 0.001));
      const next = sceneAt(tb + 0.001);
      chevronWipe(ctx, p, (c) => {
        c.fillStyle = '#000';
        c.fillRect(0, 0, W, H);
        next(c, t);
      }, { dir: 'up', color: C.lime, band: 56, band2: 14, color2: C.teal });
      done = true;
      break;
    }
  }
  if (!done) {
    const f = sceneAt(t);
    if (f) f(ctx, t);
  }
  // Into the summit: a white flash as the climb tops out.
  const fl = t >= CH.summit ? Math.exp(-(t - CH.summit) * 5) : 0;
  if (fl > 0.01) {
    ctx.save();
    ctx.globalAlpha = 0.8 * fl;
    ctx.fillStyle = C.white;
    ctx.fillRect(0, 0, W, H);
    ctx.restore();
  }
  if (t >= CH.end) {
    RUSH = 4200;
    sky(ctx, t, 0.5 * (1 - prog(t, DURATION - 1.2, DURATION)));
    ctx.save();
    ctx.globalAlpha = 1 - prog(t, DURATION - 0.7, DURATION - 0.05);
    reelEnd(ctx, t - CH.end, { app: 'streak', accent: C.lime, accent2: C.teal, name: 'Streak', tagline: ['Every week', 'counts.'] });
    ctx.restore();
  }
  const a = (1 - prog(t, LOCK - 1.15, LOCK - 0.9)) + prog(t, CH.week, CH.week + 0.5) - prog(t, CH.end - 0.5, CH.end);
  const w = weeksAt(t);
  reelHud(ctx, t, {
    app: 'STREAK', accent: C.lime, alpha: clamp(a),
    right: `${String(w).padStart(3, '0')} WEEKS CLIMBED`, rightColor: C.lime,
    progress: t / CH.end, ticks: [LOCK, CH.week, CH.shield, CH.climb, CH.summit].map((x) => x / CH.end),
  });
}

function cues() {
  const q = [];
  [0.05, 1.2].forEach((x, i) => q.push({ t: x + 1.05, type: 'session', v: 1 - i * 0.1 }));
  q.push({ t: LOCK - 0.95, type: 'draw' }, { t: LOCK, type: 'lock' });
  for (const tb of WIPES) q.push({ t: tb - 0.3, type: 'whoosh' });
  q.push({ t: CH.week + 1.15, type: 'session', v: 1 }, { t: CH.week + 1.17, type: 'weekComplete' }, { t: CH.week + 2.45, type: 'burst' });
  q.push({ t: CH.shield + 2.3, type: 'shieldPress' });
  for (let w = 0; w < 104; w++) q.push({ t: CH.climb + CLIMB.fill[0] + (CLIMB.fill[1] - CLIMB.fill[0]) * Math.sqrt((w + 1) / 104), type: 'week', w });
  q.push({ t: CH.climb + CLIMB.fly, type: 'fly', until: CH.climb + CLIMB.rise[0] + 0.4 });
  // Passing each summit on the way up.
  S.CLIMBS.forEach((c, i) => {
    const wk = c.weeks - 1;
    for (let x = CLIMB.rise[0]; x < CLIMB.rise[1] + 0.3; x += 0.02) {
      if (Math.floor(lerp(0, 104, clamp(climbCam(x).climb * 1.02))) >= wk) {
        q.push({ t: CH.climb + x, type: 'passSummit', i });
        break;
      }
    }
  });
  q.push({ t: CH.summit, type: 'everest' }, { t: CH.summit + 0.1, type: 'haptic' }, { t: CH.summit + 0.35, type: 'haptic' });
  q.push({ t: END + 0.9, type: 'logo' });
  return q.sort((a, b) => a.t - b.t);
}

export default {
  id: 'streak-reel',
  title: 'HybridX Streak (reel)',
  frame: [W, H],
  duration: DURATION,
  bpm: 112,
  sections: [{ name: 'intro', bar: 0 }, { name: 'title', bar: 2 }, { name: 'week', bar: 4 }, { name: 'shield', bar: 6 }, { name: 'climb', bar: 8 }, { name: 'summit', bar: 12 }, { name: 'end', bar: 14 }],
  draw,
  cues,
};
