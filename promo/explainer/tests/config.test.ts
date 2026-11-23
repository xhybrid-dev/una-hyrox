// The film's copy, timings and story, checked: the brief's caption rules, the scenes' order, and
// the numbers the watch's screens show agreeing with each other and with the apps' rules.

import { describe, expect, it } from 'vitest';
import fs from 'node:fs';
import path from 'node:path';
import {
  CAPTIONS, CALLOUTS, CTA, DURATION, FAMILY, RACE, RACE_BEATS, SCENES, STREAK_BEATS, WEEK, WEEK_BEATS, MONTAGE,
  edwardsLoad, zoneOf, BRAND,
} from '../src/config';
import { BUZZES, PRESSES, raceAt, streakLine } from '../src/screen/script';
import { CLIMBS, climbFor } from '../src/screen/streak';

const words = (s: string) => s.trim().split(/\s+/).length;

describe('captions', () => {
  const all = CAPTIONS.flatMap((c) => [c.text, ...Object.values(c.br ?? {})]);
  it('are 8 words at most', () => {
    for (const c of CAPTIONS) expect(words(c.text), c.text).toBeLessThanOrEqual(8);
  });
  it('have no em or en dashes, anywhere on screen', () => {
    for (const s of [...all, ...Object.values(CALLOUTS), CTA, ...FAMILY.map((f) => f.benefit)]) expect(s).not.toMatch(/[—–]/);
  });
  it('keep the same words in every ratio', () => {
    for (const c of CAPTIONS) for (const b of Object.values(c.br ?? {})) expect(b.replace(/\n/g, ' ')).toBe(c.text);
  });
  it('never overlap, and sit inside the film', () => {
    const s = [...CAPTIONS].sort((a, b) => a.t0 - b.t0);
    s.forEach((c, i) => {
      expect(c.t1).toBeGreaterThan(c.t0);
      expect(c.t1).toBeLessThanOrEqual(DURATION);
      if (i) expect(c.t0).toBeGreaterThanOrEqual(s[i - 1].t1);
    });
  });
  it('never name HYROX', () => {
    for (const s of [...all, ...Object.values(CALLOUTS), ...FAMILY.map((f) => f.benefit)]) expect(s.toLowerCase()).not.toContain('hyrox');
  });
});

describe('scenes', () => {
  it('run back to back from 0 to the end', () => {
    expect(SCENES[0].start).toBe(0);
    SCENES.forEach((s, i) => {
      expect(s.end).toBeGreaterThan(s.start);
      if (i) expect(s.start).toBe(SCENES[i - 1].end);
    });
    expect(SCENES[SCENES.length - 1].end).toBe(DURATION);
  });
  it('fit their beats', () => {
    const at = (id: string) => SCENES.find((s) => s.id === id)!;
    expect(MONTAGE.start + FAMILY.length * MONTAGE.step).toBeLessThanOrEqual(at('family').end);
    expect(WEEK_BEATS.start + 3 * WEEK_BEATS.step).toBeLessThanOrEqual(at('race').start + 0.1);
    expect(RACE_BEATS.savedLine).toBeLessThan(STREAK_BEATS.swap);
    expect(STREAK_BEATS.homeAfter).toBeLessThan(at('streak').end);
  });
});

describe('the example race', () => {
  it('is 16 segments, 8 runs and 8 stations, 1:15:28', () => {
    expect(RACE.segs).toHaveLength(16);
    expect(RACE.segs.filter((s) => s.type === 'run')).toHaveLength(8);
    expect(RACE.total).toBe(75 * 60 + 28);
    expect(RACE.minutes).toBe(75);
  });
  it('splits on screen in order, one R2 press per segment', () => {
    const r2 = PRESSES.filter((p) => p.btn === 'r2');
    expect(r2).toHaveLength(16);
    r2.forEach((p, i) => i && expect(p.t).toBeGreaterThan(r2[i - 1].t));
    expect(r2[15].t).toBeCloseTo(RACE_BEATS.finish);
    // Mid-segment, the race clock is inside that segment.
    for (let k = 0; k < 16; k++) {
      const r = raceAt(RACE_BEATS.open[k] + RACE_BEATS.segDur[k] / 2)!;
      expect(r.idx).toBe(k);
      expect(r.total).toBeGreaterThanOrEqual(RACE.segs[k].start);
      expect(r.total).toBeLessThanOrEqual(RACE.segs[k].end);
    }
  });
  it('buzzes as the code does: a run once, a station twice, the finish long', () => {
    const splits = BUZZES.filter((b) => b.t > RACE_BEATS.gun && b.t <= RACE_BEATS.finish);
    expect(splits).toHaveLength(16);
    splits.slice(0, 15).forEach((b, k) => expect(b.pulses).toBe(RACE.segs[k + 1].type === 'run' ? 1 : 2));
    expect(splits[15].len).toBeGreaterThanOrEqual(1);
  });
});

describe('training load (hybridx-link TrainingLoad.hpp)', () => {
  it("is Edwards': zone x minutes", () => {
    expect(edwardsLoad([0, 0, 35 * 60, 5 * 60, 0, 0])).toBe(85);
    expect(edwardsLoad([600, 0, 0, 0, 0, 0])).toBe(0);
    expect(zoneOf(0)).toBe(0);
    expect(zoneOf(152)).toBe(3);
    expect(zoneOf(153)).toBe(4);
    expect(zoneOf(200)).toBe(5);
  });
  it('gives the race the load its Saved screen shows', () => {
    expect(WEEK.sessions[3].load).toBe(RACE.load);
    expect(RACE.load).toBeGreaterThan(200);
  });
});

describe('the week and X Streak', () => {
  it('counts the week up on each Saved screen, the streak only once it is met', () => {
    expect(streakLine(1)).toEqual(['1 of 4 this week', '11-week streak']);
    expect(streakLine(3)).toEqual(['3 of 4 this week', '11-week streak']);
    expect(streakLine(4)).toEqual(['4 of 4 this week', '12-week streak']);
  });
  it('has the race complete the week that summits Snowdon', () => {
    expect(WEEK.sessions).toHaveLength(WEEK.target);
    expect(WEEK.sessions[WEEK.target - 1].app).toBe('race');
    const before = climbFor(WEEK.weeksBefore);
    expect(CLIMBS[before.climb].name).toBe('Snowdon');
    expect(before.steps - before.stepsClimbed).toBe(1);
    expect(CLIMBS[before.climb].summitAt).toBe(WEEK.weeksBefore + 1);
    expect(CLIMBS[climbFor(WEEK.weeksBefore + 1).climb].name).toBe('Ben Nevis');
  });
  it('holds the shields the rules allow (one per 4 weeks, at most 2)', () => {
    expect(WEEK.shields).toBe(Math.min(2, Math.floor(WEEK.weeksBefore / 4)));
  });
  it('names every app of the family once', () => {
    expect(new Set(FAMILY.map((f) => f.id)).size).toBe(7);
  });
});

describe('the brand', () => {
  it("uses the brand files' yellow", () => expect(BRAND.yellow).toBe('#FADB5C'));
});

describe('determinism', () => {
  it('draws nothing from wall time or unseeded chance', () => {
    const src = path.resolve(import.meta.dirname, '..', 'src');
    const files = fs.readdirSync(src, { recursive: true }).map(String).filter((f) => f.endsWith('.ts'));
    for (const f of files) {
      const text = fs.readFileSync(path.join(src, f), 'utf8');
      expect(text, f).not.toMatch(/Math\.random\(|Date\.now\(|getDelta\(/);
      // Wall time only moves the playhead (main.ts) and counts frames (the dev overlay).
      if (!/^(main|dev[\\/]overlay)\.ts$/.test(f)) expect(text, f).not.toMatch(/performance\.now\(/);
    }
  });
});
