// The master timeline: one paused GSAP timeline. Every frame is timeline.seek(t), then the scene
// reads the state object below. Each property moves through its own keyframes, one tween after
// another (never two at once), so seeking to any time in any order gives the same values.
//
// Scene modules also compute per-element motion from t directly with GSAP's eases (ease()).

import gsap from 'gsap';
import { RACE_BEATS, STREAK_BEATS, WEEK_BEATS, MONTAGE, type Ratio } from './config';
import { LAYOUTS, type Shot } from './layout';

export const ease = (name: string) => gsap.parseEase(name) as (x: number) => number;

/** A value through keyframes [time, value, ease into it], as a pure function of t. */
export function keys(t: number, k: [number, number, string?][]): number {
  if (t <= k[0][0]) return k[0][1];
  for (let i = 1; i < k.length; i++) {
    if (t <= k[i][0]) {
      const [t0, v0] = k[i - 1];
      const [t1, v1, e] = k[i];
      const x = (t - t0) / (t1 - t0);
      return v0 + (v1 - v0) * ease(e ?? 'power3.inOut')(x);
    }
  }
  return k[k.length - 1][1];
}

/** 0 before a, 1 after b, eased between. */
export const span = (t: number, a: number, b: number, e = 'power3.inOut') => (t <= a ? 0 : t >= b ? 1 : ease(e)((t - a) / (b - a)));

export type State = {
  cam: { px: number; py: number; pz: number; lx: number; ly: number; lz: number; fov: number };
  light: { env: number; key: number; rim: number; rimX: number; grid: number };
  watch: { turn: number; sway: number };
  family: { a: number; spin: number; tight: number };
  week: { a: number };
  race: { a: number };
  streak: { a: number; load: number };
  lockup: { a: number; cta: number };
};

export function buildTimeline(ratio: Ratio) {
  const L = LAYOUTS[ratio];
  const S: State = {
    cam: { px: 0, py: 0, pz: 0, lx: 0, ly: 0, lz: 0, fov: 30 },
    light: { env: 0, key: 0, rim: 0, rimX: -8, grid: 0 },
    watch: { turn: 0, sway: 0 },
    family: { a: 0, spin: 0, tight: 0 },
    week: { a: 0 },
    race: { a: 0 },
    streak: { a: 0, load: 0 },
    lockup: { a: 0, cta: 0 },
  };
  const tl = gsap.timeline({ paused: true, defaults: { immediateRender: false } });

  /** Keyframes for one property: [time, value, ease]. */
  const track = <T extends object>(obj: T, prop: keyof T & string, k: [number, number, string?][]) => {
    tl.set(obj, { [prop]: k[0][1] }, 0);
    for (let i = 1; i < k.length; i++) {
      const [t0, v0] = k[i - 1];
      const [t1, v1, e] = k[i];
      if (t1 > t0) tl.fromTo(obj, { [prop]: v0 }, { [prop]: v1, duration: t1 - t0, ease: e ?? 'power3.inOut' }, t0);
    }
  };

  // The camera: shots per ratio (layout.ts), confident eased moves between them.
  const shots: [number, Shot, string?][] = [
    [0, L.shots.introA],
    [4.8, L.shots.introB, 'power2.inOut'],
    [5.2, L.shots.introB],
    [6.8, L.shots.family, 'power3.inOut'],
    [14.4, L.shots.family],
    [15.8, L.shots.aux, 'power3.inOut'],
    [26.6, L.shots.aux],
    [27.8, L.shots.race, 'power3.inOut'],
    [RACE_BEATS.finish - 0.2, L.shots.race],
    [RACE_BEATS.savePress + 0.3, L.shots.raceWide, 'power3.inOut'],
    [STREAK_BEATS.turn + 0.9, L.shots.raceWide],
    [STREAK_BEATS.turn + 2.0, L.shots.streak, 'power3.inOut'],
    [STREAK_BEATS.homeAfter + 0.4, L.shots.streak],
    [56.4, L.shots.outro, 'power3.inOut'],
    [62, L.shots.outro],
  ];
  const camKeys = (f: (s: Shot) => number) => shots.map(([t, s, e]) => [t, f(s), e] as [number, number, string?]);
  track(S.cam, 'px', camKeys((s) => s.pos[0]));
  track(S.cam, 'py', camKeys((s) => s.pos[1]));
  track(S.cam, 'pz', camKeys((s) => s.pos[2]));
  track(S.cam, 'lx', camKeys((s) => s.look[0]));
  track(S.cam, 'ly', camKeys((s) => s.look[1]));
  track(S.cam, 'lz', camKeys((s) => s.look[2]));
  track(S.cam, 'fov', camKeys((s) => s.fov));

  // Intro: a rim light sweeps across in the dark, then the room and the key light come up.
  track(S.light, 'rim', [[0, 0], [0.4, 4.5, 'power2.out'], [3.2, 4.5], [4.6, 1.6, 'power2.inOut']]);
  track(S.light, 'rimX', [[0, -9], [3.0, 9, 'power2.inOut']]);
  track(S.light, 'env', [[0, 0], [1.6, 0], [3.8, 1, 'power2.inOut']]);
  track(S.light, 'key', [[0, 0], [2.0, 0], [4.0, 1.5, 'power2.inOut']]);
  track(S.light, 'grid', [[0, 0], [2.4, 0], [4.4, 1, 'power2.out']]);

  // The lockup: in the intro, then again at the end with the address.
  track(S.lockup, 'a', [[0, 0], [1.4, 0], [2.4, 1, 'power3.out'], [4.4, 1], [5.0, 0, 'power2.in'], [57.4, 0], [58.4, 1, 'power3.out']]);
  track(S.lockup, 'cta', [[0, 0], [58.2, 0], [59.0, 1, 'power3.out']]);

  // The watch turns to X Streak (one full turn; the screen changes facing away).
  track(S.watch, 'turn', [[0, 0], [STREAK_BEATS.turn, 0], [STREAK_BEATS.turn + STREAK_BEATS.turnDur, 1, 'power3.inOut']]);

  // Each layer's presence.
  track(S.family, 'a', [[0, 0], [MONTAGE.start, 1, 'none'], [26.6, 1], [27.4, 0.0, 'power2.inOut'], [49.6, 0], [50.4, 1, 'power2.out'], [62, 1]]);
  track(S.family, 'spin', [[0, 0], [62, 62 * 0.05, 'none']]);
  // A tighter, higher orbit once the panel is beside the watch; a little looser for the end.
  track(S.family, 'tight', [[0, 0], [14.6, 0], [16.0, 1, 'power3.inOut'], [55.6, 1], [57.2, 0.45, 'power3.inOut']]);
  // The week panel steps aside while the race runs, and is back for the race's tile.
  track(S.week, 'a', [[0, 0], [WEEK_BEATS.start - 0.6, 0], [WEEK_BEATS.start + 0.4, 1, 'power3.out'], [RACE_BEATS.marks, 1],
    [RACE_BEATS.gun + 0.4, 0, 'power2.inOut'], [RACE_BEATS.finish, 0], [RACE_BEATS.savePress - 0.1, 1, 'power2.out'],
    [STREAK_BEATS.turn - 0.2, 1], [STREAK_BEATS.turn + 0.6, 0, 'power2.in']]);
  track(S.race, 'a', [[0, 0], [RACE_BEATS.gun - 0.4, 0], [RACE_BEATS.gun + 0.4, 1, 'power3.out'], [RACE_BEATS.finish + 1.2, 1], [RACE_BEATS.finish + 2.0, 0, 'power2.in']]);
  track(S.streak, 'a', [[0, 0], [STREAK_BEATS.turn + 0.9, 0], [STREAK_BEATS.turn + 1.7, 1, 'power3.out'], [STREAK_BEATS.homeAfter + 0.4, 1], [56.0, 0, 'power2.in']]);
  track(S.streak, 'load', [[0, 0], [STREAK_BEATS.load, 0], [STREAK_BEATS.load + 0.6, 1, 'power3.out'], [STREAK_BEATS.homeAfter + 0.4, 1], [56.0, 0, 'power2.in']]);

  return { tl, S };
}
