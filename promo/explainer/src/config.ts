// Everything a person might want to change without touching scene code: the copy, the timings,
// the colours, the example race and the week's story. Times are seconds on the film's timeline.
//
// British English. Captions: 8 words at most, no em dashes (tests/config.test.ts checks).

// --- Colours ------------------------------------------------------------------------------------

/** Around the watch: HybridX yellow, black and white only (hybridx-link/Tools/Brand/brand.py). */
export const BRAND = {
  yellow: '#FADB5C',
  black: '#0A0A0A',
  white: '#FFFFFF',
  grey: '#8C8C8C',
  greyDark: '#2B2B2B',
} as const;

/**
 * On the watch: the SDK's palette (una-sdk/Libs/Header/SDK/GUI/Color.hpp) as the display shows it.
 * Each channel is 2 bits; the SDK's 0x40 steps show as 0, 85, 170, 255.
 */
export const WATCH = {
  BLACK: '#000000',
  WHITE: '#FFFFFF',
  GRAY: '#AAAAAA',
  GRAY_DARK: '#555555',
  CYAN: '#55FFFF',
  LEMON: '#FFFF55',
  ORCHID: '#FF55FF',
  YELLOW_DARK: '#FFAA00',
  YELLOW: '#FFFF00',
  CHARTREUSE: '#55FF00',
  GREEN: '#00FF00',
  RED: '#FF0000',
  TEAL: '#00AAAA',
  TEAL_DARK: '#005555',
  LIME: '#AAFF00',
} as const;

// --- The film -------------------------------------------------------------------------------------

export const FPS = 60;
export const SEED = 20261122;

export type SceneId = 'intro' | 'family' | 'week' | 'race' | 'streak' | 'outro';
export const SCENES: { id: SceneId; title: string; start: number; end: number }[] = [
  { id: 'intro', title: 'Intro', start: 0, end: 5 },
  { id: 'family', title: 'The family', start: 5, end: 15 },
  { id: 'week', title: 'One week, in sync', start: 15, end: 27 },
  { id: 'race', title: 'Race day', start: 27, end: 43 },
  { id: 'streak', title: 'X Streak', start: 43, end: 55 },
  { id: 'outro', title: 'Outro', start: 55, end: 62 },
];
export const DURATION = SCENES[SCENES.length - 1].end;

export type Ratio = '16x9' | '9x16' | '1x1';
export const RATIOS: Record<Ratio, { w: number; h: number }> = {
  '16x9': { w: 1920, h: 1080 },
  '9x16': { w: 1080, h: 1920 },
  '1x1': { w: 1080, h: 1080 },
};

/** Captions, bottom of frame. `br` gives a ratio its own line breaks ("\n"). */
export const CAPTIONS: { t0: number; t1: number; text: string; br?: Partial<Record<Ratio, string>> }[] = [
  { t0: 5.6, t1: 10.4, text: 'One watch. A whole family of training apps.', br: { '16x9': 'One watch. A whole family\nof training apps.', '9x16': 'One watch.\nA whole family\nof training apps.', '1x1': 'One watch. A whole family\nof training apps.' } },
  { t0: 15.4, t1: 20.6, text: 'Every session counts, whichever app.', br: { '9x16': 'Every session counts,\nwhichever app.' } },
  { t0: 21.2, t1: 26.4, text: 'Every app knows your week.', br: { '9x16': 'Every app\nknows your week.' } },
  { t0: 27.4, t1: 33.4, text: 'Race day. One button, every split.', br: { '9x16': 'Race day.\nOne button, every split.' } },
  { t0: 41.5, t1: 42.9, text: 'Your week, done the moment you save.', br: { '9x16': 'Your week, done\nthe moment you save.' } },
  { t0: 45.0, t1: 49.8, text: 'Hit your week. Climb a mountain.', br: { '9x16': 'Hit your week.\nClimb a mountain.' } },
  { t0: 50.4, t1: 54.6, text: 'One training load, from every app.', br: { '9x16': 'One training load,\nfrom every app.' } },
  { t0: 55.4, t1: 58.0, text: 'Seven apps. One watch. All in sync.', br: { '9x16': 'Seven apps.\nOne watch.\nAll in sync.' } },
];

/** Labels drawn by a leader line to a point on or round the watch. Uppercase, tracked. */
export const CALLOUTS = {
  shared: 'SHARED ON THE WATCH',
  split: 'R2 · SPLIT',
  shield: 'MISS A WEEK? A SHIELD SAVES IT',
  load: 'FITNESS · FATIGUE · FORM',
  timelapse: 'TIME-LAPSE',
} as const;

export const CTA = 'HYBRIDX.CLUB';

// --- The family -------------------------------------------------------------------------------------

export type AppId = 'run' | 'trail' | 'ride' | 'workout' | 'training' | 'race' | 'streak';

/**
 * The seven apps, in the order the film introduces them. Names as the watch shows them (CLAUDE.md,
 * 2 November 2026); each benefit is a claim from hybridx-link/docs/STORE_RELEASE.md's checked text.
 * `screen` is a real simulator preview (Resources/previews) or one of the screens redrawn live.
 */
export const FAMILY: { id: AppId; name: string; benefit: string; screen: 'preview' | 'live' }[] = [
  { id: 'run', name: 'X Run', benefit: 'VO2MAX FROM YOUR RUNS', screen: 'preview' },
  { id: 'trail', name: 'X Trail', benefit: 'FOLLOW A GPX ROUTE', screen: 'preview' },
  { id: 'ride', name: 'X Ride', benefit: 'OUTDOORS OR ON THE TURBO', screen: 'preview' },
  { id: 'workout', name: 'X Workout', benefit: 'GYM, HIIT AND CIRCUITS', screen: 'preview' },
  { id: 'training', name: 'X Training', benefit: 'YOUR PLAN, ON YOUR WRIST', screen: 'preview' },
  { id: 'race', name: 'HybridX Race', benefit: 'EVERY SPLIT, ONE BUTTON', screen: 'live' },
  { id: 'streak', name: 'X Streak', benefit: 'EVERY SESSION COUNTS', screen: 'live' },
];

/** Scene 1: when each app's screen comes up and its icon lifts out (one every `step` s). */
export const MONTAGE = { start: 5.6, step: 1.2 };

// --- Training load ----------------------------------------------------------------------------------

/**
 * Illustrative heart-rate zone thresholds (bpm) for an athlete with a maximum near 190: the watch's
 * own zones are set per person. Zone i + 1 is above threshold i (hybridx-link TrainingLoad.hpp).
 */
export const HR_ZONES = [95, 114, 133, 152, 171];

export function zoneOf(bpm: number, th: number[] = HR_ZONES): number {
  let z = 0;
  th.forEach((v, i) => {
    if (bpm > v) z = i + 1;
  });
  return Math.min(z, 5);
}

/** Edwards' TRIMP, as hybridx-link's edwardsLoad(): zone x minutes, rounded. zoneSec[0] scores 0. */
export function edwardsLoad(zoneSec: number[]): number {
  let pointSec = 0;
  for (let z = 1; z < 6; z++) pointSec += z * (zoneSec[z] ?? 0);
  return Math.floor((pointSec + 30) / 60);
}

// --- The example race (the promo films' race: an average finisher, for illustration) ---------------

/** RaceData.hpp kStations: the name on the race face, its work, the summary's short name. */
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

export type Segment = {
  type: 'run' | 'station';
  name: string; // as the race face shows it
  work: string;
  short: string;
  sec: number;
  hr: number;
  start: number; // race seconds
  end: number;
};

/** promo/lib/ui-race.mjs EXAMPLE: 1:15:28, invented for the films and kept (Jon, 22 November 2026). */
export const RACE = (() => {
  const runs = [280, 305, 312, 318, 322, 326, 330, 314];
  const stations = [272, 198, 245, 290, 281, 118, 252, 365];
  const segs: Segment[] = [];
  let acc = 0;
  for (let i = 0; i < 8; i++) {
    const r = { type: 'run' as const, name: `RUN ${i + 1}/8`, work: '1 km', short: `RUN ${i + 1}`, sec: runs[i], hr: 150 + i * 2 };
    const s = { type: 'station' as const, name: STATIONS[i].name, work: STATIONS[i].work, short: STATIONS[i].short, sec: stations[i], hr: 158 + i * 2 };
    for (const x of [r, s]) {
      segs.push({ ...x, start: acc, end: acc + x.sec });
      acc += x.sec;
    }
  }
  const zoneSec = [0, 0, 0, 0, 0, 0];
  segs.forEach((s) => (zoneSec[zoneOf(s.hr)] += s.sec));
  return { segs, total: acc, minutes: Math.floor(acc / 60), load: edwardsLoad(zoneSec) };
})();

// --- The week (one story across the family; the apps' own rules) ------------------------------------

/**
 * Target 4 a week. Before Monday: an 11-week streak, 11 weeks climbed (Snowdon, 1 week to go),
 * 2 shields. Each Saved screen's Streak line follows hybridx-link StreakNow: "n of 4 this week",
 * and the streak counts this week only once it is met.
 */
export const WEEK = {
  target: 4,
  streakBefore: 11,
  weeksBefore: 11,
  shields: 2,
  days: ['MON', 'TUE', 'WED', 'THU', 'FRI', 'SAT', 'SUN'],
  sessions: [
    { app: 'run' as AppId, day: 0, minutes: 40, kind: 'Run', zoneMin: [0, 0, 35, 5, 0, 0] },
    { app: 'ride' as AppId, day: 2, minutes: 45, kind: 'Ride', zoneMin: [0, 0, 20, 15, 10, 0] },
    { app: 'workout' as AppId, day: 3, minutes: 42, kind: 'Strength', zoneMin: [0, 30, 12, 0, 0, 0] },
    { app: 'race' as AppId, day: 5, minutes: RACE.minutes, kind: 'Hybrid', zoneMin: null },
  ].map((s) => ({ ...s, load: s.zoneMin ? edwardsLoad(s.zoneMin.map((m) => m * 60)) : RACE.load })),
} as const;

/** Streak's Training load screen after the week (LoadScreen; illustrative history). */
export const LOAD_VIEW = { form: '-9, Building', fitness: '48 · 6 weeks', fatigue: '57 · 7 days' };

// --- Timings that tie the screen to the world -------------------------------------------------------

/** Scene 2: one Saved screen every `step` s from `start`. */
export const WEEK_BEATS = { start: 15.4, step: 3.8 };

/** Scene 3, the race. Times on the film's timeline. */
export const RACE_BEATS = (() => {
  const menu = 27.0; // Race's main menu
  const marks = 27.9; // R1: "On your marks"
  const gun = 28.9; // R1: the clock starts
  // Each segment's time on screen: the first two and the last two slower, the middle a time-lapse.
  const segDur = [1.25, 1.1, 0.62, 0.56, 0.52, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.52, 0.56, 0.8, 1.1];
  const toast = 0.55; // the "Split" toast (2 s on the watch; shortened here)
  const toastSplits = 2; // how many splits show their toast
  // When each segment opens and closes on screen, toasts included.
  const open: number[] = [];
  let t = gun;
  segDur.forEach((d, i) => {
    open.push(t);
    t += d;
    if (i < toastSplits) t += toast;
  });
  const finish = t; // the last R2: "Finished"
  const savePress = finish + 0.85; // R1: Save
  const savedLine = savePress + 0.45; // the Streak line arrives
  return { menu, marks, gun, segDur, toast, toastSplits, open, finish, savePress, savedLine };
})();

/** Scene 4, X Streak's moments (HomeScreen.cpp: toast 2.2 s, beat 0.9 s, glide 1.1 s, to summit 0.9 s). */
export const STREAK_BEATS = {
  turn: 43.0, // the watch turns to X Streak
  turnDur: 1.0, // the screen changes halfway, facing away
  home: 43.8, // Home, before
  toast: 44.6, // "+1 Hybrid · 75 min", the fourth pip pops
  stepUp: 45.5, // "Week complete!", the glide begins
  glide: 1.1,
  summit: 47.5, // Summit!
  load: 50.3, // the Training load screen
  homeAfter: 54.2, // Home on Ben Nevis
};
