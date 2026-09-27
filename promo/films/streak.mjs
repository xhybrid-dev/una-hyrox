// HybridX Streak: "The climb".
//
// 112 BPM, 70 bars (150 s). Night sky, teal rock and lime for everything
// earned. The film climbs: every chapter sits a little higher in the sky, the
// wipes point up, and the counter in the corner (weeks climbed) only ever
// goes up, because in Streak a reset never takes you down the mountain.

import { W, H, clamp, lerp, prog, ep, E, hash, tempo, TAU, spring } from '../lib/core.mjs';
import { text, kinetic, measure, poly, rgba, light, roundRect, strokePart } from '../lib/gfx.mjs';
import { C, T, sting, hud, chevronWipe, endCard, xMark } from '../lib/brand.mjs';
import { watch, buttonPos, ripple, buzz } from '../lib/watch.mjs';
import { headline, wrap, pressAt } from '../lib/film.mjs';
import * as S from '../lib/ui-streak.mjs';

const TM = tempo(112);
const B = (b, beats = 0) => TM.at(b, beats);
const DURATION = B(70);

const LOCK = B(6);
const CH = {
  counts: B(9),
  coach: B(16),
  complete: B(21),
  ladder: B(25),
  shield: B(31),
  fresh: B(36),
  montage: B(41),
  trophy: B(53),
  everest: B(59),
  end: B(65),
};

// --- The sky ------------------------------------------------------------------

const STARS = Array.from({ length: 320 }, (_, i) => ({
  x: hash(i, 11) * W,
  y: hash(i, 12) * H * 3,
  r: 0.6 + Math.pow(hash(i, 13), 6) * 2.4,
  tw: hash(i, 14) * TAU,
  depth: 0.25 + hash(i, 15) * 0.75,
}));

/** How far the camera has climbed: drives the stars' drift. */
function altitude(t) {
  // Slow and steady, then a rush during the montage.
  const base = t * 22;
  const rush = ep(t, CH.montage + 8.5, CH.trophy, E.inOutSine) * 5200;
  return base + rush;
}

function sky(ctx, t, alpha = 1) {
  if (alpha <= 0) return;
  const alt = altitude(t);
  ctx.save();
  ctx.globalAlpha *= alpha;
  for (const s of STARS) {
    const y = ((s.y + alt * s.depth) % (H * 3)) - H;
    if (y < -10 || y > H + 10) continue;
    const tw = 0.55 + 0.45 * Math.sin(t * (0.8 + s.depth * 1.6) + s.tw);
    ctx.globalAlpha = alpha * tw * (0.35 + 0.65 * s.depth);
    ctx.fillStyle = s.r > 1.6 ? C.white : '#9aa3ad';
    ctx.fillRect(s.x, y, s.r, s.r);
  }
  ctx.restore();
  // A teal glow along the horizon.
  const g = ctx.createRadialGradient(W / 2, H * 1.25, 0, W / 2, H * 1.25, H * 1.1);
  g.addColorStop(0, rgba(C.teal, 0.16 * alpha));
  g.addColorStop(1, 'rgba(0,0,0,0)');
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, W, H);
}

/** The app's own scene, drawn large: k times the watch's size, base at (cx, baseY). */
function bigScene(ctx, o, cx, baseY, k) {
  ctx.save();
  ctx.translate(cx - 120 * k, baseY - (o.baseY ?? 118) * k);
  ctx.scale(k, k);
  const r = S.scene(ctx, { stars: false, far: false, ...o });
  ctx.restore();
  return { here: [cx - 120 * k + r.here[0] * k, baseY - (o.baseY ?? 118) * k + r.here[1] * k], m: r.m, tr: r.tr };
}

/** A far range of hills across the bottom of the frame. */
function farRange(ctx, y, k = 1, color = '#1a1d20', seed = 3) {
  ctx.save();
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.moveTo(0, H);
  let x = -50;
  let i = 0;
  while (x < W + 100) {
    const w = (140 + hash(i, seed) * 260) * k;
    const h = (60 + hash(i + 7, seed) * 140) * k;
    ctx.lineTo(x + w / 2, y - h);
    ctx.lineTo(x + w, y);
    x += w * 0.7;
    i++;
  }
  ctx.lineTo(W, H);
  ctx.closePath();
  ctx.fill();
  ctx.restore();
}

// --- The home screen's state over the film ---------------------------------------

function homeAt(t, over = {}) {
  return {
    climb: 1, climbed: 3, streak: 7, target: 3, sessions: 0, coach: 'Snowdon: 5 weeks to go', mood: 'soft',
    pulse: 1.5 + 1.5 * Math.sin(t * TAU / 1.8), wave: Math.round(2 * Math.sin(t * TAU / 0.76)), ...over,
  };
}

// --- Scenes ---------------------------------------------------------------------

function coldOpen(ctx, t) {
  sky(ctx, t, prog(t, 0.2, 3.0));
  farRange(ctx, H - 40 + (1 - ep(t, 1.5, 6, E.outCubic)) * 200, 1.1, '#101214', 5);
  const y = 470;
  const q = prog(t, B(4) - 0.4, B(4));
  kinetic(ctx, 'Consistency', W / 2, y, { ...T.hero(150), align: 'center' }, prog(t, B(1), B(1) + 0.7), q, 'rise');
  // "is a climb." with "climb." in lime.
  const s2 = { ...T.heroLight(150), align: 'left' };
  const w1 = measure(ctx, 'is a ', s2);
  const w2 = measure(ctx, 'climb.', s2);
  const x0 = W / 2 - (w1 + w2) / 2;
  kinetic(ctx, 'is a ', x0, y + 160, s2, prog(t, B(2), B(2) + 0.7), q, 'rise');
  kinetic(ctx, 'climb.', x0 + w1, y + 160, { ...s2, color: C.lime }, prog(t, B(2) + 0.25, B(2) + 0.95), q, 'rise');
  // The pips: one lime, two to go. Then the lime one climbs a mountain.
  const pp = prog(t, B(4), B(4) + 0.5);
  if (pp > 0 && t < LOCK - 0.95) {
    const rise = ep(t, B(5), LOCK - 1.0, E.inOutCubic);
    const mtnA = ep(t, B(5) - 0.2, B(5) + 1.0, E.outCubic) * (1 - prog(t, LOCK - 1.3, LOCK - 0.95));
    if (mtnA > 0) {
      ctx.save();
      ctx.globalAlpha *= mtnA;
      bigScene(ctx, { shape: 1, climbed: 0, steps: 8, flag: true, wave: Math.round(2 * Math.sin(t * 8)) }, W / 2, H + 60 - rise * 110, 4.2);
      ctx.restore();
    }
    const pipsA = 1 - prog(t, B(5) + 0.4, B(5) + 0.9);
    ctx.save();
    ctx.globalAlpha *= pipsA;
    for (let i = 0; i < 3; i++) {
      const pi = E.outBack(prog(t, B(4) + i * 0.18, B(4) + 0.4 + i * 0.18));
      const x = W / 2 + (i - 1) * 64;
      if (i === 0) {
        ctx.fillStyle = C.lime;
        ctx.beginPath();
        ctx.arc(x, 540, 20 * pi, 0, TAU);
        ctx.fill();
      } else {
        ctx.strokeStyle = C.gray;
        ctx.lineWidth = 7;
        ctx.beginPath();
        ctx.arc(x, 540, 17 * pi, 0, TAU);
        ctx.stroke();
      }
    }
    ctx.restore();
    kinetic(ctx, '1 OF 3 THIS WEEK', W / 2, 620, { ...T.label(18, C.soft), align: 'center' }, prog(t, B(4) + 0.5, B(4) + 1.2), prog(t, B(5) + 0.3, B(5) + 0.8), 'scramble', { t });
  }
}

function title(ctx, t) {
  sky(ctx, t, 1 - prog(t, LOCK - 1.0, LOCK - 0.6) * 0.6);
  const st = t - (LOCK - 0.95);
  const out = prog(t, B(8, 2), B(9));
  sting(ctx, st, { accent: C.lime, accent2: C.teal, name: 'Streak', sub: 'A WEEKLY STREAK FOR UNA WATCH', out, h: 190 });
  // On the lock, the Burst: eight lime rays out of the mark.
  const lt = t - LOCK;
  if (lt > 0 && lt < 1.4) {
    const len = E.outCubic(clamp(lt / 0.9));
    const a = 1 - prog(lt, 0.6, 1.4);
    ctx.save();
    ctx.strokeStyle = rgba(C.lime, a);
    ctx.lineWidth = 3;
    for (let i = 0; i < 8; i++) {
      const an = (i / 8) * TAU;
      const r0 = 200 + len * 260, r1 = 200 + len * 520;
      ctx.beginPath();
      ctx.moveTo(W / 2 + Math.cos(an) * r0, H / 2 + Math.sin(an) * r0);
      ctx.lineTo(W / 2 + Math.cos(an) * r1, H / 2 + Math.sin(an) * r1);
      ctx.stroke();
    }
    ctx.restore();
  }
}

const CHIPS = [
  { kind: 'Run', detail: '32 MIN · RUNNING' },
  { kind: 'Strength', detail: '45 MIN · WORKOUT' },
  { kind: 'Hybrid', detail: '58 MIN · HYBRIDX' },
  { kind: 'Ride', detail: '64 MIN · CYCLING' },
  { kind: 'Row', detail: '20 MIN · ROWING' },
  { kind: 'Walk', detail: '25 MIN · WALKING' },
];

function chip(ctx, x, y, c, a = 1, scale = 1) {
  ctx.save();
  ctx.globalAlpha *= a;
  ctx.translate(x, y);
  ctx.scale(scale, scale);
  const w = 460, h = 124;
  ctx.fillStyle = '#0f1215';
  roundRect(ctx, 0, -h / 2, w, h, 22);
  ctx.fill();
  ctx.strokeStyle = '#232a31';
  ctx.lineWidth = 2;
  ctx.stroke();
  ctx.fillStyle = C.lime;
  roundRect(ctx, 20, -h / 2 + 24, 8, h - 48, 4);
  ctx.fill();
  text(ctx, c.kind, 56, 4, T.head(48, C.white));
  text(ctx, c.detail, 58, 40, T.label(17, C.soft));
  ctx.restore();
}

function counts(ctx, t) {
  const lt = t - CH.counts;
  const dur = CH.coach - CH.counts;
  sky(ctx, t, 0.8);
  const out = prog(lt, dur - 0.5, dur - 0.1);
  headline(ctx, lt, ['It counts', 'everything.'], 'Every activity your watch records, from any app, counted for you. Nothing to start.', 150, 300, { p0: 0.3, q0: dur - 0.5, size: 104, width: 640 });
  // The watch.
  const wIn = ep(lt, 0.2, 1.3, E.outExpo);
  const wo = { cx: lerp(W + 400, 1440, wIn), cy: 560, d: 540, rot: lerp(0.25, -0.03, wIn) };
  // Activities arrive: each chip slides in, then shoots into the watch.
  const land = [];
  CHIPS.forEach((c, i) => {
    const t0 = 2.2 + i * 1.6;
    const pin = ep(lt, t0, t0 + 0.6, E.outExpo);
    const fly = ep(lt, t0 + 0.9, t0 + 1.45, E.inOutCubic);
    if (pin <= 0) return;
    const sx = lerp(-520, 150, pin), sy = 760;
    const tx = wo.cx - 170, ty = wo.cy + 60;
    const x = lerp(sx, tx, fly), y = lerp(sy, ty, fly) - Math.sin(fly * Math.PI) * 140;
    const a = (1 - prog(fly, 0.75, 1)) * (1 - out);
    chip(ctx, x, y, c, a, lerp(1, 0.3, fly));
    if (fly >= 1) land.push({ i, t: t0 + 1.45 });
  });
  const landed = land.filter((l) => lt >= l.t);
  const sessions = Math.min(2, landed.length);
  const last = landed[landed.length - 1];
  const since = last ? lt - last.t : 99;
  const toast = last && since < 1.3 ? `+1 ${CHIPS[last.i].kind} · ${CHIPS[last.i].detail.split(' · ')[0].toLowerCase()}` : null;
  const pop = since < 0.44 ? 4 * Math.sin(Math.PI * clamp(since / 0.44)) : 0;
  const bz = buzz(since, 5, 0.2);
  watch(ctx, {
    ...wo, cx: wo.cx + bz[0], cy: wo.cy + bz[1],
    screen: (s) => S.home(s, homeAt(t, { sessions: Math.min(sessions + (landed.length > 2 ? 1 : 0), 2), toast, popIdx: Math.max(0, sessions - 1), pop, coach: sessions >= 1 ? `${3 - sessions} more · ${4 - sessions} days left` : 'Snowdon: 5 weeks to go' })),
    bloom: 0.5, alpha: 1 - out,
  });
  kinetic(ctx, 'RUN · STRENGTH · HYBRID · RIDE · ROW · WALK', 154, 900, T.label(16, C.mute), prog(lt, 1.4, 2.2), out, 'scramble', { t: lt });
}

const COACH_LINES = [
  { s: '1 more · 3 days left', c: C.gray, mood: 'soft' },
  { s: '2 more in 2 days. Go!', c: C.amber, mood: 'risk' },
  { s: 'Week banked. Rest up.', c: C.lime, mood: 'win' },
];

function coach(ctx, t) {
  const lt = t - CH.coach;
  const dur = CH.complete - CH.coach;
  sky(ctx, t, 0.8);
  const out = prog(lt, dur - 0.4, dur);
  headline(ctx, lt, 'A coach, not a nag.', 'Warm, short lines with the numbers that matter: what’s left, and the days to do it in.', 150, 300, { p0: 0.2, q0: dur - 0.4, size: 96, width: 760 });
  // The coach lines, large, one after another.
  const per = 2.3;
  COACH_LINES.forEach((cl, i) => {
    const t0 = 1.6 + i * per;
    kinetic(ctx, cl.s, 150, 640, { ...T.heroLight(78, cl.c) }, prog(lt, t0, t0 + 0.9), i < 2 ? prog(lt, t0 + per - 0.3, t0 + per) : out, 'type', { t: lt });
  });
  // The week: Monday to Sunday, sessions landing.
  const days = ['MON', 'TUE', 'WED', 'THU', 'FRI', 'SAT', 'SUN'];
  const dx = 150, dy = 820, dw = 104;
  const wp = ep(lt, 0.6, 1.4, E.outCubic);
  ctx.save();
  ctx.globalAlpha *= wp * (1 - out);
  days.forEach((d, i) => {
    const x = dx + i * dw;
    text(ctx, d, x, dy + 50, T.label(14, C.mute));
    ctx.strokeStyle = C.rule;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.arc(x + 14, dy, 14, 0, TAU);
    ctx.stroke();
  });
  const done = [[0, 1.6], [2, 1.6 + per], [5, 1.6 + per * 2]];
  done.forEach(([d, at]) => {
    const p = E.outBack(prog(lt, at - 0.2, at + 0.25));
    if (p <= 0) return;
    ctx.fillStyle = C.lime;
    ctx.beginPath();
    ctx.arc(dx + d * dw + 14, dy, 14 * p, 0, TAU);
    ctx.fill();
  });
  // Today.
  const today = lt < 1.6 + per ? 3 : 4;
  ctx.fillStyle = C.white;
  ctx.fillRect(dx + today * dw + 4, dy + 62, 20, 2);
  ctx.restore();
  // The watch mirrors the coach lines.
  const wIn = ep(lt, 0.3, 1.3, E.outExpo);
  const idx = lt < 1.6 + per ? 0 : lt < 1.6 + per * 2 ? 1 : 2;
  const sessions = idx === 0 ? 2 : idx === 1 ? 2 : 3;
  watch(ctx, {
    cx: lerp(W + 400, 1500, wIn), cy: 520, d: 500, rot: lerp(0.25, 0.04, wIn),
    screen: (s) => S.home(s, homeAt(t, { sessions, target: idx === 1 ? 4 : 3, coach: COACH_LINES[idx].s, mood: COACH_LINES[idx].mood, streak: idx === 1 ? 23 : 7, climb: idx === 1 ? 3 : 1, climbed: idx === 1 ? 14 : 3 })),
    bloom: 0.5, alpha: 1 - out,
  });
}

function complete(ctx, t) {
  const lt = t - CH.complete;
  const dur = CH.ladder - CH.complete;
  sky(ctx, t, 0.8);
  const out = prog(lt, dur - 0.45, dur);
  // The third session lands at 0.8 s; the glide takes 1.1 s; the burst follows.
  const landT = 0.8;
  const glide = ep(lt, landT + 0.3, landT + 1.4, E.inOutCubic);
  const burstT = landT + 1.4;
  const d = 560;
  const wo = { cx: 600, cy: H / 2 + 10, d, rot: 0 };
  const since = lt - landT;
  const bz = buzz(since, 6, 0.3);
  const k = d / 240;
  const m = S.mountain(1, 118, 100);
  const tr = S.trail(m);
  const here = S.stepPos(tr, 3, 8, glide);
  const hx = wo.cx + (here[0] - 120) * k, hy = wo.cy + (here[1] - 120) * k;
  // The burst, from the climber, out across the frame (behind everything).
  const bl = lt - burstT;
  if (bl > 0) {
    const e = E.outExpo(clamp(bl / 1.2));
    const a = 1 - prog(bl, 0.5, 1.6);
    ctx.save();
    // Keep the rays on the watch's side, clear of the words.
    ctx.beginPath();
    ctx.rect(0, 0, 960, H);
    ctx.clip();
    ctx.strokeStyle = rgba(C.lime, a);
    ctx.lineWidth = 4;
    for (let i = 0; i < 8; i++) {
      const an = (i / 8) * TAU;
      const r0 = 30 + e * 600, r1 = 30 + e * 1500;
      ctx.beginPath();
      ctx.moveTo(hx + Math.cos(an) * r0, hy + Math.sin(an) * r0);
      ctx.lineTo(hx + Math.cos(an) * r1, hy + Math.sin(an) * r1);
      ctx.stroke();
    }
    ctx.restore();
    light(ctx, hx, hy, 520, rgba(C.lime, 0.4), a);
  }
  // The words, huge and lime, beside the watch.
  kinetic(ctx, 'Week', 1000, 490, T.hero(150, C.lime), prog(lt, landT + 0.15, landT + 0.65), out, 'rise');
  kinetic(ctx, 'complete!', 1000, 650, T.hero(150, C.lime), prog(lt, landT + 0.3, landT + 0.8), out, 'rise');
  kinetic(ctx, 'ONE STEP UP THE MOUNTAIN', 1008, 730, T.label(20, C.soft), prog(lt, burstT + 0.4, burstT + 1.2), out, 'scramble', { t: lt });
  const wIn = ep(lt, -0.3, 0.5, E.outCubic);
  watch(ctx, {
    ...wo, cx: wo.cx + bz[0], cy: wo.cy + bz[1], alpha: wIn * (1 - out),
    screen: (s) => {
      const banked = lt > burstT + 1.2;
      S.home(s, homeAt(t, {
        sessions: lt > landT ? 3 : 2, climbed: 3, frac: glide,
        streak: banked ? 8 : lt > landT ? null : 7,
        words: lt > landT && !banked ? 'Week complete!' : 'week streak', wordsColor: lt > landT && !banked ? C.lime : C.white,
        toast: lt > landT && lt < landT + 1.2 ? '+1 Strength · 45 min' : null,
        coach: banked ? 'Week banked. Rest up.' : '1 more · 3 days left', mood: banked ? 'win' : 'soft',
        popIdx: 2, pop: since > 0 && since < 0.44 ? 4 * Math.sin(Math.PI * since / 0.44) : 0,
        burst: bl > 0 && bl < 0.6 ? 12 * E.outCubic(bl / 0.5) : 0, burstA: 1 - prog(bl, 0.35, 0.6),
        mline: banked ? 'Snowdon · 4 weeks to go' : 'Snowdon · 5 weeks to go',
      }));
    },
    bloom: 0.6,
  });
}

function ladder(ctx, t) {
  const lt = t - CH.ladder;
  const dur = CH.shield - CH.ladder;
  sky(ctx, t, 0.9);
  const out = prog(lt, dur - 0.45, dur);
  headline(ctx, lt, 'One week. One step.', 'Hit your target and you climb a step. Reach the top and a taller mountain is waiting.', 150, 250, { p0: 0.2, q0: 5.4, size: 96, width: 900 });
  // The five mountains as one range, growing left to right.
  const baseY = 900;
  const xs = [250, 610, 980, 1360, 1690];
  const ks = [1.55, 1.8, 2.1, 2.4, 2.75];
  const zoom = ep(lt, 5.4, 7.0, E.inOutQuint);
  // Camera: focus moves from the frame centre to Snowdon while zooming in.
  const Z = lerp(1, 2.9, zoom);
  const fx = lerp(W / 2, xs[1], zoom), fy = lerp(H / 2, baseY - 55 * ks[1], zoom);
  ctx.save();
  ctx.translate(W / 2, H / 2 + 40 * zoom);
  ctx.scale(Z, Z);
  ctx.translate(-fx, -fy);
  // Back to front: biggest (furthest right) first so the range overlaps nicely.
  [4, 3, 2, 1, 0].forEach((i) => {
    const c = S.CLIMBS[i];
    const rise = ep(lt, 1.0 + i * 0.3, 1.9 + i * 0.3, E.outCubic);
    if (rise <= 0) return;
    ctx.save();
    ctx.beginPath();
    ctx.rect(-2000, -2000, W + 4000, baseY + 2000);
    ctx.clip();
    ctx.globalAlpha *= i === 1 ? 1 : 1 - zoom;
    bigScene(ctx, { shape: c.shape, climbed: i === 0 ? c.steps : i === 1 ? 7 : 0, steps: c.steps, flag: true, wave: Math.round(2 * Math.sin(t * 8 + i)) }, xs[i], baseY + (1 - rise) * 420, ks[i]);
    ctx.restore();
  });
  ctx.restore();
  // Labels under the range (screen space, before the zoom).
  S.CLIMBS.forEach((c, i) => {
    const rise = ep(lt, 1.3 + i * 0.3, 2.1 + i * 0.3, E.outCubic);
    const a = rise * (1 - zoom);
    if (a <= 0) return;
    text(ctx, c.name.toUpperCase(), xs[i], baseY + 48, { ...T.label(17, i === 0 ? C.lime : C.white), align: 'center', alpha: a });
    text(ctx, `${c.weeks} WEEKS`, xs[i], baseY + 76, { ...T.label(14, C.mute), align: 'center', alpha: a });
  });
  // Then the summit, on the watch, with confetti spilling out.
  const sw = ep(lt, 6.6, 7.6, E.outExpo);
  if (sw > 0) {
    const st = lt - 7.2;
    const wo = { cx: W / 2, cy: H / 2 + 20, d: lerp(200, 560, sw), rot: lerp(-0.4, 0, sw) };
    // Dim the zoomed range behind the watch.
    ctx.save();
    ctx.globalAlpha = 0.7 * sw;
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, W, H);
    ctx.restore();
    watch(ctx, {
      ...wo, alpha: clamp(sw * 2) * (1 - out),
      screen: (s) => (st < 1.0 ? S.home(s, homeAt(t, { climbed: 7, frac: ep(st, 0, 1.0, E.inOutCubic), streak: 12, sessions: 3, coach: 'Week banked. Rest up.', mood: 'win', mline: 'Snowdon · 0 weeks to go' })) : S.summitScreen(s, { climb: 1, t: st - 1.0 })),
      bloom: 0.6,
    });
    if (st > 1.0) confetti(ctx, st - 1.0, 70, 1 - out, 11);
  }
}

/** Confetti over the frame: the SummitScreen's colours, tumbling. */
function confetti(ctx, t, n = 120, alpha = 1, seed = 1) {
  ctx.save();
  for (let i = 0; i < n; i++) {
    const x0 = hash(i, seed) * W;
    const delay = hash(i, seed + 1) * 1.2;
    const speed = 260 + hash(i, seed + 2) * 380;
    const tt = t - delay;
    if (tt < 0) continue;
    const y = -40 + tt * speed;
    if (y > H + 40) continue;
    const x = x0 + Math.sin(tt * (1.5 + hash(i, seed + 3) * 2) + i) * 40;
    const rot = tt * (2 + hash(i, seed + 4) * 5) + i;
    const flip = Math.abs(Math.cos(tt * (3 + hash(i, seed + 5) * 4) + i));
    ctx.globalAlpha = alpha;
    ctx.fillStyle = S.CONF[i % 4];
    ctx.save();
    ctx.translate(x, y);
    ctx.rotate(rot);
    ctx.fillRect(-6, -10 * flip, 12, 20 * flip + 1);
    ctx.restore();
  }
  ctx.restore();
}

function shield(ctx, t) {
  const lt = t - CH.shield;
  const dur = CH.fresh - CH.shield;
  sky(ctx, t, 0.6);
  const out = prog(lt, dur - 0.45, dur);
  headline(ctx, lt, 'Life happens.', 'Miss a week with a shield in hand, and you choose: spend it and the streak climbs on.', 150, 250, { p0: 0.3, q0: dur - 0.45, size: 104, width: 720 });
  // Four weeks: three banked, one short.
  const wy = 720, wx = 160;
  for (let w = 0; w < 4; w++) {
    const p = ep(lt, 0.8 + w * 0.15, 1.3 + w * 0.15, E.outCubic);
    text(ctx, `WEEK ${w + 6}`, wx + w * 220 - 14, wy + 70, { ...T.label(15, C.mute), alpha: p * (1 - out) });
    for (let s = 0; s < 3; s++) {
      const x = wx + w * 220 + s * 46, y = wy;
      const done = w < 3 || s === 0;
      ctx.save();
      ctx.globalAlpha *= p * (1 - out);
      if (done) {
        ctx.fillStyle = C.lime;
        ctx.beginPath();
        ctx.arc(x, y, 15, 0, TAU);
        ctx.fill();
      } else {
        ctx.strokeStyle = C.gray;
        ctx.lineWidth = 4;
        ctx.beginPath();
        ctx.arc(x, y, 13, 0, TAU);
        ctx.stroke();
      }
      ctx.restore();
    }
  }
  // The shield comes down over the short week.
  const sp = ep(lt, 5.2, 6.0, E.outBack);
  const saved = lt > 6.2;
  if (sp > 0) {
    ctx.save();
    ctx.globalAlpha *= 1 - out;
    S.shieldGlyph(ctx, wx + 3 * 220 + 46, wy - 230 - (1 - sp) * 80, 2.2);
    ctx.restore();
    light(ctx, wx + 3 * 220 + 46, wy - 80, 300, rgba(C.shield, 0.35), sp * (1 - out) * (saved ? 1 : 0.6));
  }
  kinetic(ctx, 'STREAK SAVED · 1 SHIELD LEFT', wx - 14, wy + 130, T.label(18, C.lime), prog(lt, 6.4, 7.0), out, 'scramble', { t: lt });
  // The watch: the offer, the tick, "Streak saved."
  const wIn = ep(lt, 0.3, 1.3, E.outExpo);
  const pressT = CH.shield + 6.0;
  const wo = { cx: lerp(W + 400, 1480, wIn), cy: 540, d: 520, rot: lerp(0.25, 0.03, wIn) };
  const bz = buzz(t - pressT - 0.05, 3, 0.25);
  watch(ctx, {
    ...wo, cx: wo.cx + bz[0], cy: wo.cy + bz[1],
    screen: (s) => (lt < 2.2 ? S.home(s, homeAt(t, { streak: 9, sessions: 1, climb: 2, climbed: 2, coach: '2 more · 1 day left', mood: 'risk', mline: 'Ben Nevis · 12 weeks to go' })) : S.shieldScreen(s, { saved, glow: { r1: 0.5 + 0.5 * Math.sin(lt * 5) } })),
    bloom: 0.5, alpha: 1 - out, press: { r1: pressAt(t - pressT) },
  });
  if (t > pressT) ripple(ctx, ...buttonPos(wo, 'r1', 1.3), t - pressT, C.chartreuse, { rings: 2, speed: 360, life: 0.7 });
}

function fresh(ctx, t) {
  const lt = t - CH.fresh;
  const dur = CH.montage - CH.fresh;
  const out = prog(lt, dur - 0.45, dur);
  const rise = ep(lt, 0.0, 4.6, E.outCubic);
  sky(ctx, t, (1 - rise * 0.7) * (1 - out));
  const hz = H - 130; // the horizon
  const sx = 1440; // the sun rises behind the watch
  const sunY = lerp(hz + 420, 520, rise);
  ctx.save();
  ctx.globalAlpha *= 1 - out;
  light(ctx, sx, sunY, 1100, rgba(C.amber, 0.30), rise);
  // Rays, long, from behind the watch.
  ctx.save();
  ctx.beginPath();
  ctx.rect(0, 0, W, hz);
  ctx.clip();
  ctx.strokeStyle = C.lemon;
  ctx.lineWidth = 5;
  ctx.lineCap = 'round';
  const rr = ep(lt, 1.0, 3.6, E.outCubic);
  for (let i = 0; i < 8; i++) {
    const an = (i / 8) * TAU;
    ctx.beginPath();
    ctx.moveTo(sx + Math.cos(an) * 400 * rr, sunY + Math.sin(an) * 400 * rr);
    ctx.lineTo(sx + Math.cos(an) * 520 * rr, sunY + Math.sin(an) * 520 * rr);
    ctx.stroke();
  }
  ctx.fillStyle = C.amber;
  ctx.beginPath();
  ctx.arc(sx, sunY, 350, 0, TAU);
  ctx.fill();
  ctx.restore();
  // The app's three hills, stretched across the frame, in front of the sun.
  const hx = (x) => (x + 20) / 270 * W;
  const hy = (y) => hz + (y - 116) * 5.2;
  const triH = (a, b, c, col) => {
    ctx.fillStyle = col;
    ctx.beginPath();
    ctx.moveTo(hx(a[0]), hy(a[1]));
    ctx.lineTo(hx(b[0]), hy(b[1]));
    ctx.lineTo(hx(c[0]), hy(c[1]));
    ctx.closePath();
    ctx.fill();
  };
  triH([-20, 116], [40, 90], [110, 116], '#2b2f33');
  triH([-10, 116], [80, 74], [170, 116], C.tealDark);
  triH([70, 116], [170, 70], [250, 116], C.teal);
  ctx.fillStyle = '#000';
  ctx.fillRect(0, hz, W, H - hz);
  ctx.restore();
  // The watch, in front of the sun, with the app's own sunrise.
  const wIn = ep(lt, 0.6, 1.8, E.outExpo);
  watch(ctx, {
    cx: sx, cy: 520, d: 470, strap: false, rot: lerp(0.2, 0, wIn),
    screen: (s) => S.freshStart(s, { p: prog(lt, 1.2, 2.8) }),
    bloom: 0.5, alpha: (1 - out) * clamp(wIn * 2),
  });
  headline(ctx, lt, 'Fresh start.', 'When a streak ends, you keep your climb. A reset never takes you down the mountain.', 150, 280, { p0: 1.6, q0: dur - 0.45, size: 110, width: 700 });
  // The two numbers: the streak resets, the climb stays.
  const nums = prog(lt, 4.0, 4.6);
  const streakN = lt < 5.4 ? 9 : 0;
  const flip = ep(lt, 5.2, 5.6, E.inOutCubic);
  kinetic(ctx, 'STREAK', 154, 640, T.label(18, C.mute), nums, out, 'fade');
  kinetic(ctx, String(streakN), 146, 780, T.num(150, streakN ? C.white : C.gray), nums, out, 'fade');
  if (flip > 0 && flip < 1) {
    ctx.fillStyle = rgba(C.white, 0.5 * Math.sin(Math.PI * flip));
    ctx.fillRect(150, 700, 120, 3);
  }
  kinetic(ctx, 'CLIMB', 474, 640, T.label(18, C.mute), nums, out, 'fade');
  kinetic(ctx, '18', 466, 780, T.num(150, C.lime), nums, out, 'fade');
  kinetic(ctx, 'WEEKS · KEPT', 474, 820, T.label(16, C.lime), prog(lt, 5.8, 6.4), out, 'scramble', { t: lt });
}

/** A soft band of mist: stretched radial glows. */
function mist(ctx, y, seed, alpha) {
  if (alpha <= 0 || y < -300 || y > H + 300) return;
  ctx.save();
  for (let i = 0; i < 6; i++) {
    const x = hash(i, seed) * W;
    const r = 180 + hash(i + 3, seed) * 220;
    ctx.save();
    ctx.translate(x, y + (hash(i + 5, seed) - 0.5) * 80);
    ctx.scale(3.2, 1);
    const g = ctx.createRadialGradient(0, 0, 0, 0, 0, r);
    g.addColorStop(0, `rgba(170,210,215,${0.075 * alpha})`);
    g.addColorStop(1, 'rgba(170,210,215,0)');
    ctx.fillStyle = g;
    ctx.fillRect(-r, -r, r * 2, r * 2);
    ctx.restore();
  }
  ctx.restore();
}

// The montage: 104 weeks, then the five mountains stacked, climbed.
const STACK = (() => {
  // World space: y grows upwards from 0 (the grid) into the sky.
  const ks = [2.9, 3.3, 3.7, 4.1, 4.7];
  const gaps = [0, 1000, 1040, 1080, 1140];
  let base = 330;
  return S.CLIMBS.map((c, i) => {
    base += gaps[i] + (i ? 0 : 0);
    const m = S.mountain(c.shape, 118, 100);
    const tr = S.trail(m);
    const cx = W / 2 + [-300, 280, -260, 260, 60][i];
    return { ...c, k: ks[i], base, cx, m, tr };
  });
})();

/** A step's world position: climb i, step s (1-based). */
function stepWorld(i, s) {
  const st = STACK[i];
  const p = S.stepPos(st.tr, s, st.steps);
  return [st.cx + (p[0] - 120) * st.k, st.base + (118 - p[1]) * st.k];
}

const WEEK_STEPS = (() => {
  const out = [];
  STACK.forEach((st, i) => {
    for (let s = 1; s <= st.steps; s++) out.push({ i, s });
  });
  return out;
})();

function montage(ctx, t) {
  const lt = t - CH.montage;
  const dur = CH.trophy - CH.montage;
  sky(ctx, t, 1);
  // Camera: world y at the frame's bottom edge.
  const climb = ep(lt, 8.0, dur - 0.6, E.inOutSine);
  const camEnd = STACK[4].base + 110 * STACK[4].k - 0.62 * H;
  const camY = lerp(0, camEnd, climb);
  const toScreen = ([x, y]) => [x, H - (y - camY)];
  // Grid: 26 x 4 = 104 weeks.
  const gx0 = W / 2 - (26 * 52 - 8) / 2, gy0 = 390;
  const fillP = prog(lt, 0.6, 6.6);
  const filled = Math.floor(E.inQuad(fillP) * 104 + (fillP >= 1 ? 1 : 0));
  const flyP = (w) => ep(lt, 6.8 + w * 0.012, 8.4 + w * 0.012, E.inOutCubic);
  const gridA = 1 - prog(lt, 8.6, 9.2);
  // Grid caption.
  kinetic(ctx, 'TWO YEARS · 104 WEEKS', gx0, gy0 - 50, T.label(18, C.soft), prog(lt, 0.4, 1.2), prog(lt, 6.6, 7.2), 'scramble', { t: lt });
  const shown = Math.min(104, filled);
  kinetic(ctx, `WEEK ${String(Math.max(1, shown)).padStart(3, '0')}`, gx0 + 26 * 52 - 8, gy0 - 50, { ...T.label(18, C.lime), align: 'right' }, prog(lt, 0.4, 1.2), prog(lt, 6.6, 7.2), 'fade');
  // The mountains (behind the flying squares).
  STACK.forEach((st, i) => {
    const [sx, sy] = toScreen([st.cx, st.base]);
    const topY = sy - 118 * st.k;
    if (topY > H + 50 || sy < -100) return;
    const appear = ep(lt, 7.0 + i * 0.15, 8.4 + i * 0.15, E.outCubic);
    ctx.save();
    ctx.globalAlpha *= appear;
    bigScene(ctx, { shape: st.shape, climbed: 0, steps: 999, flag: true, wave: Math.round(2 * Math.sin(t * 8 + i)) }, st.cx, sy, st.k);
    ctx.restore();
    // Name and weeks, set beside the mountain as the camera passes it.
    const nameY = sy - 70 * st.k;
    const side = st.cx < W / 2 ? 1 : -1;
    const nx = side > 0 ? st.cx + 118 * st.k : st.cx - 118 * st.k;
    const vis = clamp(1.6 - Math.abs(nameY - H * 0.45) / (H * 0.45)) * appear * ep(lt, 8.3, 9.0, E.outCubic);
    if (vis > 0) {
      text(ctx, st.name, nx, nameY, { ...T.head(92, C.white), align: side > 0 ? 'left' : 'right', alpha: vis });
      text(ctx, `${st.weeks} WEEKS`, nx + (side > 0 ? 6 : -6), nameY + 52, { ...T.label(18, C.lime), align: side > 0 ? 'left' : 'right', alpha: vis });
    }
  });
  // The weeks: squares filling the grid, then flying to their steps.
  const climberWeek = Math.floor(lerp(0, 104, clamp(climb * 1.02)));
  for (let w = 0; w < 104; w++) {
    if (w >= filled && fillP < 1) continue;
    const col = w % 26, row = Math.floor(w / 26);
    const g = [gx0 + col * 52 + 22, gy0 + row * 52 + 22];
    const fp = flyP(w);
    const ws = WEEK_STEPS[w];
    const target = toScreen(stepWorld(ws.i, ws.s));
    const x = lerp(g[0], target[0], fp), y = lerp(g[1], target[1], fp);
    const size = lerp(44, 18, fp);
    const fresh = lt - (6.0 * Math.sqrt((w + 1) / 104) + 0.6);
    const flash = fillP < 1 && fresh < 0.25 && fresh > 0 ? 1 - fresh / 0.25 : 0;
    ctx.save();
    ctx.globalAlpha *= fp < 1 ? Math.max(gridA, fp) : 1;
    ctx.fillStyle = flash > 0.5 ? C.white : C.lime;
    ctx.translate(x, y);
    ctx.rotate(fp * Math.PI / 4);
    ctx.fillRect(-size / 2, -size / 2, size, size);
    ctx.restore();
    // Lime trail behind the climber.
    if (fp >= 1 && w > 0 && w <= climberWeek) {
      const prev = WEEK_STEPS[w - 1];
      if (prev.i === ws.i) {
        const a = toScreen(stepWorld(prev.i, prev.s));
        ctx.strokeStyle = rgba(C.lime, 0.85);
        ctx.lineWidth = 6;
        ctx.beginPath();
        ctx.moveTo(a[0], a[1]);
        ctx.lineTo(target[0], target[1]);
        ctx.stroke();
      }
    }
  }
  // The climber.
  if (lt > 8.2) {
    const ws = WEEK_STEPS[Math.min(103, climberWeek)];
    const [cx, cy] = toScreen(stepWorld(ws.i, ws.s));
    const pulse = 1 + 0.25 * Math.sin(t * 6);
    ctx.strokeStyle = C.lime;
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.arc(cx, cy, 30 * pulse, 0, TAU);
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
  // The altimeter: a ruler of weeks up the right edge, at each week's real
  // height on the climb, scrolling as the camera rises.
  const rulerA = ep(lt, 8.2, 9.2, E.outCubic);
  if (rulerA > 0) {
    const rx = W - 96;
    const y0 = 130, y1 = H - 150;
    ctx.save();
    ctx.beginPath();
    ctx.rect(rx - 200, y0, 240, y1 - y0);
    ctx.clip();
    ctx.globalAlpha *= rulerA;
    ctx.fillStyle = C.rule;
    ctx.fillRect(rx, y0, 2, y1 - y0);
    const summits = new Set([4, 12, 26, 52, 104]);
    for (let w = 1; w <= 104; w++) {
      const ws = WEEK_STEPS[w - 1];
      const y = toScreen(stepWorld(ws.i, ws.s))[1];
      if (y < y0 - 20 || y > y1 + 20) continue;
      const done = w <= climberWeek + 1;
      const big = summits.has(w) || w % 10 === 0;
      ctx.fillStyle = done ? C.lime : C.mute;
      ctx.fillRect(rx - (big ? 26 : 12), y - 1, big ? 26 : 12, 2);
      if (big) {
        text(ctx, String(w).padStart(3, '0'), rx - 38, y + 6, { ...T.label(15, done ? C.lime : C.mute), align: 'right' });
      }
    }
    ctx.restore();
  }
}

function trophy(ctx, t) {
  const lt = t - CH.trophy;
  const dur = CH.everest - CH.trophy;
  sky(ctx, t, 0.7);
  const out = prog(lt, dur - 0.4, dur);
  const half = 6.2; // trophies, then the glance
  if (lt < half + 0.5) {
    const q = prog(lt, half - 0.4, half);
    headline(ctx, lt, 'Every climb, kept.', 'The trophy case holds every summit and badge, your best week and your longest streak.', 150, 280, { p0: 0.2, q0: half - 0.4, size: 96, width: 760 });
    const badges = [['Trailhead', 10], ['Ridge Walker', 50], ['Centurion', 100], ['Mountaineer', 250]];
    badges.forEach(([nm, nmb], i) => {
      const p = ep(lt, 1.2 + i * 0.25, 1.9 + i * 0.25, E.outBack);
      if (p <= 0) return;
      const x = 250 + i * 270, y = 730;
      ctx.save();
      ctx.globalAlpha *= clamp(p) * (1 - q);
      ctx.translate(x, y);
      ctx.scale(p, p);
      // A hexagonal badge.
      ctx.fillStyle = i < 3 ? C.tealDark : '#15181b';
      ctx.strokeStyle = i < 3 ? C.lime : C.rule;
      ctx.lineWidth = 3;
      ctx.beginPath();
      for (let k = 0; k < 6; k++) {
        const a = (k / 6) * TAU - Math.PI / 2;
        ctx[k ? 'lineTo' : 'moveTo'](Math.cos(a) * 88, Math.sin(a) * 88);
      }
      ctx.closePath();
      ctx.fill();
      ctx.stroke();
      ctx.restore();
      text(ctx, String(nmb), x, y + 20, { ...T.num(64, i < 3 ? C.white : C.mute), align: 'center', alpha: clamp(p) * (1 - q) });
      text(ctx, nm.toUpperCase(), x, y + 138, { ...T.label(15, i < 3 ? C.white : C.mute), align: 'center', alpha: clamp(p) * (1 - q) });
      text(ctx, 'SESSIONS', x, y + 164, { ...T.label(12, C.mute), align: 'center', alpha: clamp(p) * (1 - q) });
    });
    const wIn = ep(lt, 0.3, 1.3, E.outExpo);
    watch(ctx, {
      cx: lerp(W + 400, 1540, wIn), cy: 520, d: 500, rot: lerp(0.25, 0.03, wIn),
      screen: (s) => S.trophyScreen(s, { rows: [
        { name: "Arthur's Seat", detail: 'Climbed · 4 weeks', done: true },
        { name: 'Snowdon', detail: 'Climbed · 12 weeks', done: true },
        { name: 'Ben Nevis', detail: 'Climbed · 26 weeks', done: true },
        { name: 'Mont Blanc', detail: '44 of 52 weeks', done: false },
      ] }),
      bloom: 0.45, alpha: 1 - q,
    });
  }
  if (lt > half - 0.2) {
    const g = lt - half;
    headline(ctx, g, 'At a glance.', 'Your streak and your week on the watch’s glance, without opening a thing.', 150, 300, { p0: 0.1, q0: dur - half - 0.4, size: 96, width: 700 });
    const p = ep(g, 0.2, 1.2, E.outExpo);
    const cw = 240 * 3.4, chh = 60 * 3.4;
    const x = lerp(W + 100, W - 150 - cw, p), y = 560;
    ctx.save();
    ctx.globalAlpha *= 1 - out;
    ctx.translate(x, y);
    ctx.scale(3.4, 3.4);
    glance(ctx, lt);
    ctx.restore();
    kinetic(ctx, 'THE GLANCE · 240 × 60', x + 4, y + chh + 44, T.label(16, C.mute), prog(g, 1.0, 1.6), out, 'scramble', { t: lt });
  }
}

/** The glance's full layout (glance_preview.py): mountain, flag, three lines. */
function glance(ctx, t) {
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, 240, 60);
  ctx.strokeStyle = '#3a3f44';
  ctx.lineWidth = 0.6;
  ctx.strokeRect(0, 0, 240, 60);
  ctx.strokeStyle = C.teal;
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(4, 57);
  ctx.lineTo(32, 16);
  ctx.lineTo(60, 57);
  ctx.stroke();
  ctx.strokeStyle = C.white;
  ctx.beginPath();
  ctx.moveTo(32, 3);
  ctx.lineTo(32, 16);
  ctx.moveTo(27, 23);
  ctx.lineTo(37, 23);
  ctx.stroke();
  ctx.fillStyle = '#00FF00';
  ctx.fillRect(33, 4, 9, 6);
  const f = (w, s) => `${w} ${s}px Poppins`;
  ctx.font = f(600, 16);
  ctx.letterSpacing = '0px';
  ctx.fillStyle = '#00FF00';
  ctx.fillText('7 week streak', 68, 20);
  ctx.font = f(500, 13);
  ctx.fillStyle = C.white;
  ctx.fillText('2 of 3 this week', 68, 38);
  ctx.font = f(400, 8);
  ctx.fillStyle = C.gray;
  ctx.fillText('Snowdon: 5 weeks to go', 68, 52);
}

function everest(ctx, t) {
  const lt = t - CH.everest;
  const dur = CH.end - CH.everest;
  sky(ctx, t, 1);
  const out = prog(lt, dur - 0.6, dur);
  const wIn = ep(lt, 0.0, 0.9, E.outExpo);
  let bz = [0, 0];
  [0.1, 0.35].forEach((p0) => {
    const b = buzz(lt - p0, 8, 0.3);
    bz = [bz[0] + b[0], bz[1] + b[1]];
  });
  const wo = { cx: 640 + bz[0], cy: H / 2 + 20 + bz[1], d: lerp(300, 560, wIn), rot: lerp(0.5, 0, wIn) };
  light(ctx, wo.cx, wo.cy, 700, rgba(C.lime, 0.18), wIn * (1 - out));
  watch(ctx, {
    ...wo,
    screen: (s) => S.summitScreen(s, { climb: 4, t: lt, next: 'Next: Everest again' }),
    bloom: 0.65, alpha: 1 - out,
  });
  // "Summit!" then the line to leave on, in the right-hand column.
  const swap = dur - 4.2;
  kinetic(ctx, 'Summit!', 1000, 520, T.hero(190, C.white), prog(lt, 0.05, 0.6), prog(lt, swap, swap + 0.4), 'rise');
  kinetic(ctx, 'EVEREST · 104 WEEKS', 1008, 600, T.label(24, C.lime), prog(lt, 1.0, 1.8), prog(lt, swap, swap + 0.4), 'scramble', { t: lt });
  kinetic(ctx, 'NEXT: EVEREST AGAIN', 1008, 640, T.label(18, C.soft), prog(lt, 1.6, 2.4), prog(lt, swap, swap + 0.4), 'scramble', { t: lt + 1 });
  kinetic(ctx, 'Every week', 1000, 470, T.hero(130, C.white), prog(lt, swap + 0.35, swap + 0.95), out, 'rise');
  kinetic(ctx, 'counts.', 1000, 610, T.hero(130, C.lime), prog(lt, swap + 0.5, swap + 1.1), out, 'rise');
  confetti(ctx, lt - 0.2, 170, 1 - out, 23);
}

// --- The film ---------------------------------------------------------------------

/** Weeks climbed, for the corner: it only ever goes up. */
function weeksAt(t) {
  const keys = [[0, 7], [CH.complete + 0.8, 8], [CH.ladder + 7.0, 12], [CH.shield, 17], [CH.fresh, 18], [CH.montage + 8.4, 18], [CH.trophy - 0.6, 104]];
  let v = keys[0][1];
  for (let i = 1; i < keys.length; i++) {
    const [ta, va] = keys[i - 1], [tb, vb] = keys[i];
    if (t >= tb) v = vb;
    else if (t > ta) {
      v = Math.floor(lerp(va, vb, clamp((t - ta) / (tb - ta)) ** (vb - va > 20 ? 1.0 : 20)));
      break;
    }
  }
  return v;
}

function climbAt(t) {
  if (t < CH.shield) return 'SNOWDON · 12 WEEKS';
  if (t < CH.montage) return 'BEN NEVIS · 26 WEEKS';
  if (t < CH.trophy) return 'THE LADDER';
  return 'EVEREST · 104 WEEKS';
}

const SCENES = [
  [0, LOCK - 0.95, coldOpen], [LOCK - 0.95, CH.counts, title], [CH.counts, CH.coach, counts], [CH.coach, CH.complete, coach],
  [CH.complete, CH.ladder, complete], [CH.ladder, CH.shield, ladder], [CH.shield, CH.fresh, shield], [CH.fresh, CH.montage, fresh],
  [CH.montage, CH.trophy, montage], [CH.trophy, CH.everest, trophy], [CH.everest, CH.end, everest],
];
const WIPES = [CH.counts, CH.complete, CH.ladder, CH.montage, CH.trophy];

function sceneAt(t) {
  for (const [a, b, f] of SCENES) if (t >= a && t < b) return f;
  return null;
}

function draw(ctx, t) {
  let done = false;
  for (const tb of WIPES) {
    const p = prog(t, tb - 0.3, tb + 0.35);
    if (p > 0 && p < 1) {
      const prev = sceneAt(tb - 0.001);
      const next = sceneAt(tb + 0.001);
      prev(ctx, Math.min(t, tb - 0.001));
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
    // Soft cross-dissolves into the quieter chapters.
    const soft = [CH.coach, CH.shield, CH.fresh, CH.everest];
    let dissolved = false;
    for (const tb of soft) {
      const p = prog(t, tb - 0.35, tb + 0.35);
      if (p > 0 && p < 1 && tb !== CH.everest) {
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
    sky(ctx, t, 0.5 * (1 - prog(t, DURATION - 1.5, DURATION)));
    ctx.save();
    ctx.globalAlpha = 1 - prog(t, DURATION - 0.9, DURATION - 0.05);
    endCard(ctx, t - CH.end, { app: 'streak', accent: C.lime, accent2: C.teal, name: 'Streak' });
    ctx.restore();
  }
  // HUD.
  const a = prog(t, CH.counts, CH.counts + 0.6) * (1 - prog(t, CH.end - 0.8, CH.end - 0.2));
  if (a > 0) {
    const ticks = [CH.coach, CH.complete, CH.ladder, CH.shield, CH.fresh, CH.montage, CH.trophy, CH.everest].map((x) => (x - CH.counts) / (CH.end - CH.counts));
    hud(ctx, t, {
      app: 'STREAK', accent: C.lime, alpha: a,
      topRight: climbAt(t), topRightColor: C.lime,
      progress: (t - CH.counts) / (CH.end - CH.counts), ticks,
      bottomRight: String(weeksAt(t)).padStart(3, '0'), bottomRightLabel: 'WEEKS CLIMBED',
    });
  }
}

function cues() {
  const q = [];
  q.push({ t: LOCK - 0.95, type: 'draw' }, { t: LOCK, type: 'lock' });
  for (const tb of WIPES) q.push({ t: tb - 0.3, type: 'whoosh' });
  // Sessions landing in "counts": chips land at 2.2 + i*1.6 + 1.45.
  for (let i = 0; i < CHIPS.length; i++) q.push({ t: CH.counts + 2.2 + i * 1.6 + 1.45, type: 'session', v: i < 2 ? 1 : 0.6 });
  [1.6, 1.6 + 2.3, 1.6 + 4.6].forEach((x, i) => q.push({ t: CH.coach + x, type: 'coach', mood: ['soft', 'risk', 'win'][i] }));
  q.push({ t: CH.complete + 0.8, type: 'session', v: 1 });
  q.push({ t: CH.complete + 0.8 + 0.02, type: 'weekComplete' });
  q.push({ t: CH.complete + 2.2, type: 'burst' });
  q.push({ t: CH.ladder + 8.2, type: 'summit' });
  q.push({ t: CH.shield + 6.0, type: 'shieldPress' });
  q.push({ t: CH.fresh, type: 'sunrise', until: CH.fresh + 4.2 });
  q.push({ t: CH.fresh + 5.6, type: 'reset' });
  for (let w = 0; w < 104; w++) q.push({ t: CH.montage + 0.6 + 6.0 * Math.sqrt((w + 1) / 104), type: 'week', w });
  q.push({ t: CH.montage + 6.8, type: 'fly', until: CH.montage + 9.6 });
  S.CLIMBS.forEach((c, i) => q.push({ t: CH.montage + 8.0 + i * 3.1, type: 'passSummit', i }));
  for (let i = 0; i < 4; i++) q.push({ t: CH.trophy + 1.2 + i * 0.25, type: 'badge' });
  q.push({ t: CH.everest, type: 'everest' });
  q.push({ t: CH.everest + 0.1, type: 'haptic' }, { t: CH.everest + 0.35, type: 'haptic' });
  q.push({ t: CH.end + 0.95, type: 'logo' });
  return q.sort((a, b) => a.t - b.t);
}

export default {
  id: 'streak',
  title: 'HybridX Streak',
  duration: DURATION,
  bpm: 112,
  sections: [
    { name: 'intro', bar: 0 }, { name: 'title', bar: 6 }, { name: 'counts', bar: 9 }, { name: 'coach', bar: 16 },
    { name: 'complete', bar: 21 }, { name: 'ladder', bar: 25 }, { name: 'shield', bar: 31 }, { name: 'fresh', bar: 36 },
    { name: 'montage', bar: 41 }, { name: 'trophy', bar: 53 }, { name: 'everest', bar: 59 }, { name: 'end', bar: 65 },
  ],
  draw,
  cues,
};
