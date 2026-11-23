// What the watch's screen shows at film time t, and when its buttons are pressed and it buzzes.
// Everything is a pure function of t, so any frame can be drawn on its own.

import { FAMILY, MONTAGE, RACE, RACE_BEATS, STREAK_BEATS, WEEK, WEEK_BEATS, WATCH as C, SCENES, type AppId } from '../config';
import type { Btn } from './lvgl';
import * as race from './race';
import * as streak from './streak';
import { preview, ringSaved } from './family';

export type Draw = (ctx: CanvasRenderingContext2D) => void;
const clamp = (x: number, a = 0, b = 1) => Math.min(b, Math.max(a, x));
const easeInOut = (x: number) => (x < 0.5 ? 2 * x * x : 1 - Math.pow(-2 * x + 2, 2) / 2);

/** The Streak line on each Saved screen through the week (hybridx-link StreakNow). */
export function streakLine(n: number): [string, string] {
  const met = n >= WEEK.target;
  return [`${n} of ${WEEK.target} this week`, `${WEEK.streakBefore + (met ? 1 : 0)}-week streak`];
}

/** The race at film time t: which segment is open and how far into it, by the race clock. */
export function raceAt(t: number) {
  const b = RACE_BEATS;
  for (let k = 0; k < RACE.segs.length; k++) {
    const t0 = b.open[k], t1 = t0 + b.segDur[k];
    const toastEnd = t1 + (k < b.toastSplits ? b.toast : 0);
    if (t < t1 || k === RACE.segs.length - 1) {
      const f = clamp((t - t0) / b.segDur[k]);
      const s = RACE.segs[k];
      const prevHr = k > 0 ? RACE.segs[k - 1].hr : 128;
      return { idx: k, f, toast: false, segSec: f * s.sec, total: s.start + f * s.sec, hr: prevHr + (s.hr - prevHr) * clamp(f * 3) + Math.sin(t * 7.3) };
    }
    if (t < toastEnd) return { idx: k, f: 1, toast: true, segSec: RACE.segs[k].sec, total: RACE.segs[k].end, hr: RACE.segs[k].hr };
  }
  return null;
}

/** Streak's Home through scene 4's moments (HomeScreen.cpp's stages). */
function streakHome(t: number): streak.HomeState {
  const b = STREAK_BEATS;
  const before: streak.HomeState = {
    weeks: WEEK.weeksBefore, glide: 0, target: WEEK.target, streak: WEEK.streakBefore, words: 'week streak', sessions: 3, pop: 0,
    coach: '1 more · 2 days left', coachColor: C.GRAY, burst: 0,
  };
  if (t < b.toast) return before;
  const popT = t - b.toast;
  const pop = popT < 0.22 ? (popT / 0.22) * 4 : popT < 0.44 ? 4 - ((popT - 0.22) / 0.22) * 4 : 0;
  const toast = { ...before, sessions: 4, pop: Math.round(pop), coach: `+1 Hybrid · ${RACE.minutes} min`, coachColor: C.LIME };
  if (t < b.stepUp) return toast;
  const g = clamp((t - b.stepUp) / b.glide);
  const done = b.stepUp + b.glide;
  return { ...toast, streak: null, words: 'Week complete!', wordsColor: C.LIME, glide: easeInOut(g), burst: t > done ? (t - done) / 0.5 : 0 };
}

const after: streak.HomeState = {
  weeks: WEEK.weeksBefore + 1, glide: 0, target: WEEK.target, streak: WEEK.streakBefore + 1, words: 'week streak', sessions: 4, pop: 0,
  coach: 'Week banked. Rest up.', coachColor: C.LIME, burst: 0,
};

/** Scene 1: the app on screen during its slot in the montage. */
export function montageApp(t: number): AppId | null {
  const k = Math.floor((t - MONTAGE.start) / MONTAGE.step);
  return k >= 0 && k < FAMILY.length ? FAMILY[k].id : null;
}

export function screenAt(t: number): Draw {
  const black: Draw = () => {};
  if (t < MONTAGE.start) return black;

  // Scene 1: each app in turn, then X Streak holds until the week begins.
  if (t < WEEK_BEATS.start) {
    const id = montageApp(t) ?? 'streak';
    if (id === 'race') {
      const s = RACE.segs[2];
      const f = (t - MONTAGE.start) / MONTAGE.step - 5;
      return (ctx) => race.raceFace(ctx, 2, 61 + f * 40, s.start + 61 + f * 40, s.hr - 2);
    }
    if (id === 'streak') return (ctx) => streak.home(ctx, { ...streakHome(0), sessions: 3 }, t);
    return (ctx) => preview(ctx, id);
  }

  // Scene 2: X Run, X Ride and X Workout save, each Saved screen counting the week up.
  if (t < RACE_BEATS.menu) {
    const k = Math.min(2, Math.floor((t - WEEK_BEATS.start) / WEEK_BEATS.step));
    const local = t - WEEK_BEATS.start - k * WEEK_BEATS.step;
    const s = WEEK.sessions[k];
    const line = local > 0.7 ? streakLine(k + 1) : null;
    if (s.app === 'run') return (ctx) => race.saved(ctx, 'RUN', clamp(local / 0.55), s.load, line);
    return (ctx) => ringSaved(ctx, s.load, line);
  }

  // Scene 3: race day.
  const b = RACE_BEATS;
  if (t < b.marks) return (ctx) => race.mainMenu(ctx);
  if (t < b.gun) return (ctx) => race.onYourMarks(ctx);
  if (t < b.finish) {
    const r = raceAt(t)!;
    if (r.toast) return (ctx) => race.splitToast(ctx, r.idx);
    return (ctx) => race.raceFace(ctx, r.idx, r.segSec, r.total, r.hr);
  }
  if (t < b.savePress) return (ctx) => race.finished(ctx);
  const swap = STREAK_BEATS.swap + STREAK_BEATS.swapDur / 2;
  if (t < swap) {
    const local = t - b.savePress;
    const line = t >= b.savedLine ? streakLine(4) : null;
    return (ctx) => race.saved(ctx, 'HybridX Race', clamp(local / 0.55), RACE.load, line);
  }

  // Scene 4: X Streak opens on the race.
  const sb = STREAK_BEATS;
  if (t < sb.summit) return (ctx) => streak.home(ctx, streakHome(t), t);
  if (t < sb.load) return (ctx) => streak.summit(ctx, 1, t - sb.summit);
  if (t < sb.homeAfter) return (ctx) => streak.trainingLoad(ctx);

  // Scene 5: Home on Ben Nevis.
  return (ctx) => streak.home(ctx, after, t);
}

// --- Presses and buzzes -------------------------------------------------------------------------

export type Press = { t: number; btn: Btn };
/** A buzz: how many pulses, each how long (s), and how strong (0..1). */
export type Buzz = { t: number; pulses: number; gap: number; len: number; amp: number };

export const PRESSES: Press[] = (() => {
  const b = RACE_BEATS;
  const out: Press[] = [{ t: b.marks, btn: 'r1' }, { t: b.gun, btn: 'r1' }];
  RACE.segs.forEach((_, k) => out.push({ t: b.open[k] + b.segDur[k], btn: 'r2' }));
  out.push({ t: b.savePress, btn: 'r1' });
  return out;
})();

/**
 * Where the watch really buzzes (Race's Service::notifySegment and finishRace; Streak's DESIGN.md
 * section 6 moments): a run one strong click, a station two, the finish a 1 s alert; Streak's
 * session found a tick, its step up a double click, its summit two strong pulses.
 */
export const BUZZES: Buzz[] = (() => {
  const b = RACE_BEATS;
  const out: Buzz[] = [{ t: b.gun, pulses: 1, gap: 0, len: 0.12, amp: 1 }];
  RACE.segs.forEach((_, k) => {
    const t = b.open[k] + b.segDur[k];
    const next = RACE.segs[k + 1];
    if (!next) out.push({ t, pulses: 1, gap: 0, len: 1.0, amp: 0.8 });
    else if (next.type === 'run') out.push({ t, pulses: 1, gap: 0, len: 0.12, amp: 1 });
    else out.push({ t, pulses: 2, gap: 0.16, len: 0.1, amp: 1 });
  });
  const s = STREAK_BEATS;
  out.push({ t: s.toast, pulses: 1, gap: 0, len: 0.08, amp: 0.6 });
  out.push({ t: s.stepUp, pulses: 2, gap: 0.14, len: 0.1, amp: 1 });
  out.push({ t: s.summit, pulses: 2, gap: 0.25, len: 0.2, amp: 1 });
  return out;
})();

/** The film's scene at t. */
export const sceneAt = (t: number) => SCENES.find((s) => t >= s.start && t < s.end) ?? SCENES[SCENES.length - 1];
