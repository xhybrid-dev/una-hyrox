// HybridX Race's screens, from hybridx-race/Software/Apps/LVGL-GUI/gui/src/screens.
// Positions, fonts, strings and colours are the code's; the race is config's example race.

import { RACE, type Segment } from '../config';
import { C, label, title, buttons, arc, wheel, tick, circleTick, type Ctx } from './lvgl';

const pad2 = (n: number) => String(Math.floor(n)).padStart(2, '0');
/** Fmt::shortTime: m:ss, or h:mm past an hour. */
export const shortTime = (s: number) => {
  s = Math.floor(s);
  const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60);
  return h > 0 ? `${h}:${pad2(m)}` : `${m}:${pad2(s % 60)}`;
};
/** Fmt::hms: h:mm:ss. */
export const hms = (s: number) => {
  s = Math.floor(s);
  return `${Math.floor(s / 3600)}:${pad2((s % 3600) / 60)}:${pad2(s % 60)}`;
};

/** MainScreen: the wheel, "Start race" selected. */
export function mainMenu(ctx: Ctx) {
  title(ctx, 'HybridX Race');
  wheel(ctx, [
    { text: 'Start race' },
    { text: 'Format', tip: 'Full' },
    { text: 'Pace plan', tip: 'Off' },
    { text: 'Last race' },
    { text: 'Settings' },
  ], 0);
  buttons(ctx, { r1: 'amber', r2: 'white' });
}

/** TrackStartConfirmScreen. */
export function onYourMarks(ctx: Ctx) {
  label(ctx, 'semibold_25', 'On your marks', 0, 86, 240);
  label(ctx, 'medium_18', 'Full race', 0, 116, 240, 'center', C.CYAN);
  label(ctx, 'regular_14', '16 segments', 0, 140, 240, 'center', C.GRAY);
  label(ctx, 'regular_14', 'Roxzone merged', 0, 158, 240, 'center', C.GRAY);
  tick(ctx, 186, 60, C.YELLOW_DARK);
  title(ctx, 'HybridX Race');
  buttons(ctx, { r1: 'amber', r2: 'white' });
}

// Widgets::HeartRateZone: five 24-degree segments on a radius-113 arc centred at (120, 302).
const HR_START = [-64, -38, -12, 14, 40];
const HR_COL = [C.GRAY, C.CHARTREUSE, C.YELLOW, C.YELLOW_DARK, C.RED];
/** Illustrative zone edges for the face's arc (the watch uses the athlete's own). */
const HR_EDGES = [95, 114, 133, 152, 171];

function hrZone(ctx: Ctx, bpm: number) {
  const cx = 120, cy = 302;
  for (let i = 0; i < 5; i++) arc(ctx, cx, cy, 113, 8, HR_START[i], HR_START[i] + 24, HR_COL[i], false);
  let z = -1;
  HR_EDGES.forEach((e, i) => { if (bpm > e) z = i; });
  if (z < 0) return;
  arc(ctx, cx, cy, 111, 12, HR_START[z], HR_START[z] + 24, HR_COL[z], false);
  const a = ((HR_START[z] + 12) * Math.PI) / 180;
  const rx = Math.sin(a), ry = -Math.cos(a), tx = Math.cos(a), ty = Math.sin(a);
  ctx.fillStyle = HR_COL[z];
  ctx.beginPath();
  ctx.moveTo(cx + 99 * rx, cy + 99 * ry);
  ctx.lineTo(cx + 89 * rx + 5.5 * tx, cy + 89 * ry + 5.5 * ty);
  ctx.lineTo(cx + 89 * rx - 5.5 * tx, cy + 89 * ry - 5.5 * ty);
  ctx.closePath();
  ctx.fill();
}

const accentFor = (s: Segment) => (s.type === 'station' ? C.LEMON : C.CYAN);

/** TrackScreen's main face. segSec: time in this segment; totalSec: the race clock. */
export function raceFace(ctx: Ctx, idx: number, segSec: number, totalSec: number, hr: number) {
  const s = RACE.segs[idx];
  title(ctx, 'Race');
  label(ctx, 'italic_18', s.name, 0, 42, 240, 'center', accentFor(s));
  label(ctx, 'medium_18', `${s.work} · ${idx + 1} of ${RACE.segs.length}`, 0, 64, 240, 'center', C.GRAY);
  label(ctx, 'semibold_40', shortTime(segSec), 0, 80, 240);
  label(ctx, 'medium_18', hms(totalSec), 0, 124, 240);
  const next = RACE.segs[idx + 1];
  label(ctx, 'regular_14', next ? `Next: ${next.name}` : 'Last segment', 0, 148, 240, 'center', C.GRAY);
  label(ctx, 'medium_18', `${Math.round(hr)} bpm`, 0, 160, 240);
  hrZone(ctx, hr);
  buttons(ctx, { r1: 'white', r2: 'amber' });
}

/** TrackLapScreen: the segment that just ended, and its time. */
export function splitToast(ctx: Ctx, idx: number) {
  const s = RACE.segs[idx];
  label(ctx, 'italic_18', s.name, 0, 86, 240);
  label(ctx, 'semibold_40', shortTime(s.sec), 0, 116, 240);
  title(ctx, 'Split');
}

/** TrackResultScreen, Finished: R1 saves, L2 undoes the finishing split. */
export function finished(ctx: Ctx) {
  label(ctx, 'italic_18', 'Race time', 0, 62, 240);
  label(ctx, 'semibold_40', hms(RACE.total), 0, 92, 240);
  label(ctx, 'medium_18', 'R1 Save    L2 Undo', 0, 160, 240, 'center', C.GRAY);
  title(ctx, 'Finished');
  buttons(ctx, { l2: 'white', r1: 'green' });
}

/**
 * TrackResultScreen, Saved (Race's and X Run's): "Saved", the tick; then, with the Streak line,
 * the session's load in place of the tick and the line in amber in place of the confirmation.
 * p: the tick drawing on (0..1); line: [line1, line2] once it has arrived.
 */
export function saved(ctx: Ctx, appTitle: string, p: number, load: number, line: [string, string] | null) {
  label(ctx, 'semibold_30', 'Saved', 41, 47, 159);
  if (line) {
    label(ctx, 'semibold_25', `Load ${load}`, 0, 104, 240);
    label(ctx, 'medium_18', `${line[0]}\n${line[1]}`, 35, 156, 170, 'center', C.YELLOW_DARK);
  } else {
    circleTick(ctx, 120, 120, C.YELLOW_DARK, p);
    label(ctx, 'medium_18', appTitle === 'RUN' ? 'Activity has\nbeen saved' : 'Race has\nbeen saved', 48, 156, 144);
  }
  title(ctx, appTitle);
}
