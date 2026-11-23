// The screen check (check.html, scripts/screen-check.ts): each screen drawn by the renderer with
// the same data as a simulator capture, quantised to the display's 64 colours, beside the capture
// and their difference. Not part of the film.

import { loadFonts } from './screen/font';
import { C, type Ctx } from './screen/lvgl';
import * as race from './screen/race';
import * as streak from './screen/streak';
import { ringSaved } from './screen/family';
import { quantise } from './screen/quantise';

const base = './gen/';

type Case = { id: string; ref: string; crop?: number; draw: (ctx: Ctx) => void };
const CASES: Case[] = [
  { id: 'Race: main menu', ref: 'race-menu', draw: (c) => race.mainMenu(c) },
  { id: 'Race: on your marks', ref: 'race-marks', draw: (c) => race.onYourMarks(c) },
  { id: 'Race: race face', ref: 'race-run', draw: (c) => race.raceFace(c, 0, 1, 1, 144) },
  { id: 'Race: split toast', ref: 'race-split', draw: (c) => race.splitToast(c, 0) },
  { id: 'Race: finished (example race)', ref: 'race-finished', draw: (c) => race.finished(c) },
  { id: 'X Ride: saved', ref: 'ride-saved', draw: (c) => ringSaved(c, 3, ['3 of 4 this week', '7-week streak']) },
  {
    id: 'X Streak: home', ref: 'streak-home', crop: 18.46, draw: (c) => streak.home(c, {
      weeks: 11, glide: 0, target: 3, streak: 11, words: 'week streak', sessions: 2, pop: 0,
      coach: '1 more · 7 days left', coachColor: C.GRAY, burst: 0,
    }, 0),
  },
  { id: 'X Streak: summit', ref: 'streak-summit', crop: 20, draw: (c) => streak.summit(c, 2, 0.4) },
  { id: 'X Streak: training load (our numbers)', ref: 'streak-load', crop: 20, draw: (c) => streak.trainingLoad(c) },
];

function img(src: string): Promise<HTMLImageElement> {
  return new Promise((ok, fail) => {
    const im = new Image();
    im.onload = () => ok(im);
    im.onerror = () => fail(new Error(src));
    im.src = src;
  });
}

function circle(ctx: Ctx) {
  ctx.globalCompositeOperation = 'destination-in';
  ctx.beginPath();
  ctx.arc(120, 120, 120, 0, Math.PI * 2);
  ctx.fill();
  ctx.globalCompositeOperation = 'source-over';
}

async function main() {
  await loadFonts(base);
  const grid = document.getElementById('grid')!;
  const scores: Record<string, number> = {};
  for (const k of CASES) {
    const ours = document.createElement('canvas');
    ours.width = ours.height = 240;
    const o = ours.getContext('2d')!;
    o.fillStyle = '#000';
    o.fillRect(0, 0, 240, 240);
    k.draw(o);
    quantise(o, 240, 240);
    circle(o);

    const theirs = document.createElement('canvas');
    theirs.width = theirs.height = 240;
    const r = theirs.getContext('2d')!;
    const im = await img(`${base}ref/${k.ref}.png`);
    const cr = k.crop ?? 0;
    r.imageSmoothingQuality = 'high';
    r.drawImage(im, cr, cr, im.width - 2 * cr, im.height - 2 * cr, 0, 0, 240, 240);
    circle(r);

    // Difference: grey where they agree, red where only ours draws, cyan where only theirs does.
    const diff = document.createElement('canvas');
    diff.width = diff.height = 240;
    const d = diff.getContext('2d')!;
    const a = o.getImageData(0, 0, 240, 240).data, b = r.getImageData(0, 0, 240, 240).data;
    const out = d.createImageData(240, 240);
    let bad = 0, lit = 0;
    for (let i = 0; i < a.length; i += 4) {
      const la = (a[i] + a[i + 1] + a[i + 2]) / 3, lb = (b[i] + b[i + 1] + b[i + 2]) / 3;
      const on = la > 40, tn = lb > 40;
      if (on || tn) lit++;
      if (on !== tn) bad++;
      out.data[i] = on && !tn ? 255 : on && tn ? 90 : 0;
      out.data[i + 1] = !on && tn ? 220 : on && tn ? 90 : 0;
      out.data[i + 2] = !on && tn ? 220 : on && tn ? 90 : 0;
      out.data[i + 3] = 255;
    }
    d.putImageData(out, 0, 0);
    const score = 1 - bad / Math.max(1, lit);
    scores[k.id] = Math.round(score * 1000) / 1000;

    const fig = document.createElement('figure');
    const row = document.createElement('div');
    row.style.display = 'flex';
    row.style.gap = '4px';
    for (const cv of [ours, theirs, diff]) {
      cv.style.width = '160px';
      cv.style.height = '160px';
      row.appendChild(cv);
    }
    fig.appendChild(row);
    const cap = document.createElement('figcaption');
    cap.textContent = `${k.id}: ours | simulator | difference (agreement ${(score * 100).toFixed(1)}%)`;
    fig.appendChild(cap);
    grid.appendChild(fig);
  }
  (window as unknown as { __check: unknown }).__check = scores;
}

main();
