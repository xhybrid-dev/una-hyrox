// The dev overlay (hidden with ?clean=1): play/pause, a scrub bar with the scenes marked, a button
// per scene, the time and frame, the ratio, and the frame rate. Plain DOM, so never in an export.

import { DURATION, FPS, RATIOS, SCENES, type Ratio } from '../config';

export type Player = { t: number; playing: boolean; seek: (t: number) => void; toggle: () => void };

export function overlay(player: Player, ratio: Ratio) {
  const el = document.createElement('div');
  el.id = 'dev';
  el.innerHTML = `
    <div class="row">
      <button id="pp" aria-label="Play or pause">Pause</button>
      <span id="time">0.00 s</span>
      <span id="fps"></span>
      <span class="sp"></span>
      ${Object.keys(RATIOS).map((r) => `<a href="?ratio=${r}" class="${r === ratio ? 'on' : ''}">${r.replace('x', ':')}</a>`).join('')}
    </div>
    <div id="bar"><div id="fill"></div>${SCENES.map((s) => `<i style="left:${(s.start / DURATION) * 100}%"></i>`).join('')}</div>
    <div class="row">${SCENES.map((s) => `<button data-t="${s.start}">${s.title}</button>`).join('')}</div>`;
  document.body.appendChild(el);
  const pp = el.querySelector<HTMLButtonElement>('#pp')!;
  const time = el.querySelector<HTMLSpanElement>('#time')!;
  const fpsEl = el.querySelector<HTMLSpanElement>('#fps')!;
  const bar = el.querySelector<HTMLDivElement>('#bar')!;
  const fill = el.querySelector<HTMLDivElement>('#fill')!;
  pp.onclick = () => player.toggle();
  el.querySelectorAll<HTMLButtonElement>('button[data-t]').forEach((b) => (b.onclick = () => player.seek(Number(b.dataset.t))));
  let drag = false;
  const scrub = (e: PointerEvent) => {
    const r = bar.getBoundingClientRect();
    player.seek(Math.max(0, Math.min(1, (e.clientX - r.left) / r.width)) * DURATION);
  };
  bar.onpointerdown = (e) => { drag = true; bar.setPointerCapture(e.pointerId); scrub(e); };
  bar.onpointermove = (e) => drag && scrub(e);
  bar.onpointerup = () => (drag = false);
  window.addEventListener('keydown', (e) => {
    if (e.code === 'Space') { e.preventDefault(); player.toggle(); }
    if (e.code === 'ArrowRight') player.seek(player.t + (e.shiftKey ? 1 : 1 / FPS));
    if (e.code === 'ArrowLeft') player.seek(player.t - (e.shiftKey ? 1 : 1 / FPS));
  });
  let frames = 0, last = performance.now();
  return {
    tick() {
      pp.textContent = player.playing ? 'Pause' : 'Play';
      time.textContent = `${player.t.toFixed(2)} s · frame ${Math.round(player.t * FPS)}`;
      fill.style.width = `${(player.t / DURATION) * 100}%`;
      frames++;
      const now = performance.now();
      if (now - last > 500) {
        fpsEl.textContent = `${Math.round((frames * 1000) / (now - last))} fps`;
        frames = 0;
        last = now;
      }
    },
  };
}
