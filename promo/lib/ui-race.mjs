// HybridX Race's screens, redrawn from the app's own layout code
// (hybridx-race/Software/Apps/LVGL-GUI/gui/src/screens) in 240-space.

import { C } from './brand.mjs';
import { F, label, title, buttons, arc, hline, tick, wheel, disc, textWidth } from './lvgl.mjs';
import { mss, hmss } from './core.mjs';

// RaceData.hpp: name, work, short name.
export const STATIONS = [
  { name: 'SKIERG', work: '1000 m', short: 'SKIERG' },
  { name: 'SLED PUSH', work: '50 m', short: 'SLED PUSH' },
  { name: 'SLED PULL', work: '50 m', short: 'SLED PULL' },
  { name: 'BURPEE BROAD JUMPS', work: '80 m', short: 'BURPEES' },
  { name: 'ROW', work: '1000 m', short: 'ROW' },
  { name: 'FARMERS CARRY', work: '200 m', short: 'CARRY' },
  { name: 'SANDBAG LUNGES', work: '100 m', short: 'LUNGES' },
  { name: 'WALL BALLS', work: '100 reps', short: 'WALL BALLS' },
];

/**
 * An example race, for illustration only: 8 runs and 8 stations with
 * plausible times for a mid-pack finisher. Seconds per segment.
 */
export const EXAMPLE = (() => {
  const runs = [280, 305, 312, 318, 322, 326, 330, 314];
  const st = [272, 198, 245, 290, 281, 118, 252, 365];
  const segs = [];
  for (let i = 0; i < 8; i++) {
    segs.push({ type: 'run', name: `RUN ${i + 1}/8`, short: `RUN ${i + 1}`, work: '1 km', sec: runs[i], hr: 150 + i * 2 });
    segs.push({ type: 'station', name: STATIONS[i].name, short: STATIONS[i].short, work: STATIONS[i].work, sec: st[i], hr: 158 + i * 2 });
  }
  let acc = 0;
  segs.forEach((s) => {
    s.start = acc;
    acc += s.sec;
    s.end = acc;
  });
  return { segs, total: acc, runs: runs.reduce((a, b) => a + b, 0), stations: st.reduce((a, b) => a + b, 0) };
})();

export const accentFor = (type) => (type === 'station' ? C.lemon : type === 'rox' ? C.orchid : C.cyan);

// --- Heart-rate zone bar (Widgets.cpp HeartRateZone) --------------------------

const HR_START = [-64, -38, -12, 14, 40];
const HR_COL = [C.gray, C.chartreuse, C.yellow, C.amber, C.red];

/** The five-zone bar at the bottom of the race face; zone -1 shows none. */
export function hrZone(ctx, zone, x = 15, y = 186, markerPulse = 0) {
  const cx = x + 105, cy = y + 116;
  for (let i = 0; i < 5; i++) arc(ctx, cx, cy, 113, 8, HR_START[i], HR_START[i] + 24, HR_COL[i], false);
  if (zone < 0) return;
  const z = Math.max(0, Math.min(4, Math.floor(zone)));
  arc(ctx, cx, cy, 111, 12 + markerPulse * 2, HR_START[z], HR_START[z] + 24, HR_COL[z], false);
  const a = ((HR_START[z] + 12) * Math.PI) / 180;
  const rx = Math.sin(a), ry = -Math.cos(a), tx = Math.cos(a), ty = Math.sin(a);
  const ay = cy + 0.4;
  ctx.fillStyle = HR_COL[z];
  ctx.beginPath();
  ctx.moveTo(cx + 99 * rx, ay + 99 * ry);
  ctx.lineTo(cx + 89 * rx + 5.5 * tx, ay + 89 * ry + 5.5 * ty);
  ctx.lineTo(cx + 89 * rx - 5.5 * tx, ay + 89 * ry - 5.5 * ty);
  ctx.closePath();
  ctx.fill();
}

export function zoneFor(bpm) {
  // Illustrative thresholds for an athlete with a max around 190.
  const th = [95, 114, 133, 152, 171];
  let z = -1;
  for (let i = 0; i < 5; i++) if (bpm > th[i]) z = i;
  return z;
}

// --- Screens ------------------------------------------------------------------

/** TrackScreen: the race face. */
export function raceFace(ctx, st) {
  title(ctx, 'Race');
  const acc = accentFor(st.type);
  label(ctx, F.Italic18, st.name, 0, 42, 240, 'center', acc);
  label(ctx, F.Medium18, st.work ? `${st.work} · ${st.idx + 1} of ${st.count}` : `${st.idx + 1} of ${st.count}`, 0, 64, 240, 'center', C.gray);
  label(ctx, F.SemiBold40, mss(st.segSec), 0, 80, 240, 'center', st.paused ? C.gray : C.white);
  label(ctx, F.Medium18, hmss(st.totalSec), 0, 124, 240, 'center', C.white);
  label(ctx, F.Regular14, st.next ? `Next: ${st.next}` : 'Last segment', 0, 148, 240, 'center', C.gray);
  label(ctx, F.Medium18, st.hr ? `${st.hr} bpm` : '--', 0, 160, 240, 'center', C.white);
  hrZone(ctx, st.hr ? zoneFor(st.hr) : -1, 15, 186, st.pulse || 0);
  buttons(ctx, { r1: 'white', r2: 'amber' }, st.glow || {});
}

/** TrackLapScreen: the two-second split toast. */
export function splitToast(ctx, st) {
  title(ctx, 'Split');
  label(ctx, F.Italic18, st.name, 0, 86, 240, 'center', C.white);
  label(ctx, F.SemiBold40, mss(st.sec), 0, 116, 240, 'center', C.white);
}

/** TrackStartConfirmScreen. */
export function onYourMarks(ctx, st = {}) {
  title(ctx, 'HYBRIDX RACE');
  label(ctx, F.SemiBold25, 'On your marks', 0, 86, 240, 'center', C.white);
  label(ctx, F.Medium18, st.format || 'Full race', 0, 116, 240, 'center', C.cyan);
  label(ctx, F.Regular14, st.segments || '16 segments', 0, 140, 240, 'center', C.gray);
  label(ctx, F.Regular14, st.rox || 'Roxzone merged', 0, 158, 240, 'center', C.gray);
  tick(ctx, 186, 60, C.amber);
  buttons(ctx, { r1: 'amber', r2: 'white' }, st.glow || {});
}

/** The left-hand ScrollIndicator: rail and handle. */
export function scrollRail(ctx, index, count, frac = 0) {
  const a0 = 238, a1 = 302;
  arc(ctx, 120, 120, 112, 9, a0, a1, C.grayDark, true);
  const span = (a1 - a0) / count;
  const top = a1 - (index + frac) * span;
  arc(ctx, 120, 120, 112, 9, top - span, top, C.white, true);
}

export const MAIN_ITEMS = [
  { label: 'Start race' },
  { label: 'Format', tip: 'Full' },
  { label: 'Last race' },
  { label: 'Settings' },
];

/** MainScreen: the wheel menu. */
export function mainMenu(ctx, st = {}) {
  title(ctx, 'HYBRIDX RACE');
  wheel(ctx, st.items || MAIN_ITEMS, st.sel || 0, st.shift || 0);
  scrollRail(ctx, st.sel || 0, (st.items || MAIN_ITEMS).length, st.shift || 0);
  buttons(ctx, { r1: 'amber', r2: 'white' }, st.glow || {});
}

/** A settings item with the SDK Toggle drawn beside it. */
export function settingsToggle(ctx, st) {
  title(ctx, 'Settings');
  ctx.save();
  ctx.beginPath();
  ctx.arc(120, 120, 110, 0, Math.PI * 2);
  ctx.clip();
  ctx.fillStyle = C.tealDark;
  ctx.fillRect(0, 87, 240, 66);
  ctx.restore();
  label(ctx, F.SemiBold30, 'Roxzone', 21, 90, 128, 'left', C.white);
  label(ctx, F.SemiBold30, 'splits', 21, 118, 128, 'left', C.white);
  // Toggle: a 60 x 30 pill; knob slides right when on.
  const on = st.on ?? 0;
  const tx = 151, ty = 105;
  ctx.fillStyle = on > 0.5 ? C.chartreuse : '#000';
  ctx.beginPath();
  ctx.roundRect ? ctx.roundRect(tx, ty, 60, 30, 15) : ctx.rect(tx, ty, 60, 30);
  ctx.fill();
  disc(ctx, tx + 15 + on * 30, ty + 15, 12, C.white);
  label(ctx, F.Medium18, 'Split lock', 0, 164, 240, 'center', C.white);
  label(ctx, F.Italic18, st.lock || '3 s', 0, 184, 240, 'center', C.white);
  scrollRail(ctx, 0, 5, 0);
  buttons(ctx, { r1: 'amber', r2: 'white' }, st.glow || {});
}

/** A value picker, as the settings screens show one (e.g. "Split lock / 3 s"). */
export function settingValue(ctx, st) {
  title(ctx, 'Settings');
  wheel(ctx, [{ label: st.label, tip: st.value }, { label: st.nextLabel || '', tip: st.nextTip }], 0, 0);
  scrollRail(ctx, st.index ?? 1, st.count ?? 5, 0);
  buttons(ctx, { r1: 'amber', r2: 'white' }, st.glow || {});
}

/** TrackActionScreen: the in-race action menu. */
export function actionMenu(ctx, st) {
  title(ctx, 'Race');
  const items = st.items || [{ label: 'Resume' }, { label: 'Undo last', tip: null }, { label: 'Pause' }];
  wheel(ctx, items, st.sel || 0, st.shift || 0);
  scrollRail(ctx, st.sel || 0, items.length, st.shift || 0);
  buttons(ctx, { r1: 'amber', r2: 'white' }, st.glow || {});
}

/** TrackResultScreen, finished state. */
export function finished(ctx, st) {
  title(ctx, 'Finished');
  label(ctx, F.Italic18, 'Race time', 0, 62, 240, 'center', C.white);
  label(ctx, F.SemiBold40, hmss(st.sec), 0, 92, 240, 'center', C.white);
  label(ctx, F.Medium18, 'R1 Save      L2 Undo', 0, 160, 240, 'center', C.white);
  buttons(ctx, { l2: 'white', r1: 'green' }, st.glow || {});
}

/** TrackResultScreen, saved state. */
export function saved(ctx, st = {}) {
  title(ctx, 'HYBRIDX RACE');
  label(ctx, F.SemiBold30, 'Saved', 41, 47, 159, 'center', C.white);
  // img_circletick_50x50 tinted amber.
  const p = st.p ?? 1;
  ctx.save();
  ctx.strokeStyle = C.amber;
  ctx.lineWidth = 5;
  ctx.beginPath();
  ctx.arc(120, 120, 22, -Math.PI / 2, -Math.PI / 2 + Math.PI * 2 * Math.min(1, p * 1.4));
  ctx.stroke();
  if (p > 0.55) {
    const q = Math.min(1, (p - 0.55) / 0.35);
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    ctx.beginPath();
    ctx.moveTo(109, 121);
    const mx = 117, my = 129, ex = 132, ey = 113;
    if (q < 0.4) ctx.lineTo(109 + (mx - 109) * (q / 0.4), 121 + (my - 121) * (q / 0.4));
    else {
      ctx.lineTo(mx, my);
      const r = (q - 0.4) / 0.6;
      ctx.lineTo(mx + (ex - mx) * r, my + (ey - my) * r);
    }
    ctx.stroke();
  }
  ctx.restore();
  label(ctx, F.Medium18, 'Race has', 0, 153, 240, 'center', C.white);
  label(ctx, F.Medium18, 'been saved', 0, 174, 240, 'center', C.white);
}

/** TrackSummaryScreen: page 0 is the totals, then five splits a page. */
export function summary(ctx, st) {
  title(ctx, 'Summary');
  const race = st.race || EXAMPLE;
  const rows = [];
  if ((st.page || 0) === 0) {
    label(ctx, F.Italic18, 'Race complete', 0, 42, 240, 'center', C.white);
    rows.push(['Total', hmss(race.total)], ['Runs', hmss(race.runs)], ['Stations', hmss(race.stations)],
      ['Avg HR', `${st.avg || 161} bpm`], ['Max HR', `${st.max || 183} bpm`]);
  } else {
    const first = (st.page - 1) * 5;
    label(ctx, F.Italic18, `Splits ${first + 1}-${Math.min(first + 5, 16)} of 16`, 0, 42, 240, 'center', C.white);
    for (let i = first; i < Math.min(first + 5, 16); i++) {
      rows.push([`${i + 1} ${race.segs[i].short}`, mss(race.segs[i].sec)]);
    }
  }
  rows.forEach(([n, v], i) => {
    const y = 64 + i * 24;
    label(ctx, F.Regular16, n, 34, y, 110, 'left', C.gray);
    label(ctx, F.Regular16, v, 96, y, 110, 'right', C.white);
  });
  buttons(ctx, { l1: 'white', l2: 'white', r2: 'amber' }, st.glow || {});
}

/** A status face (TrackScreen's second face): the time of day and battery. */
export function statusFace(ctx, st) {
  title(ctx, 'Race');
  label(ctx, F.SemiBold40, st.clock || '09:41', 0, 84, 240, 'center', C.white);
  label(ctx, F.Medium18, st.battery || '86%', 0, 136, 240, 'center', C.gray);
  buttons(ctx, { r1: 'white', r2: 'amber' });
}

export { textWidth, hline };
