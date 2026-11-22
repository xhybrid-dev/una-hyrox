// Boots the explainer. Live: plays from ?t (or ?scene) with the dev overlay unless ?clean=1.
// Export (?export=1): exposes window.__explainer.renderAt(t) for scripts/export.ts and draws only
// when asked, so every frame depends on its time alone.

import { App } from './app';
import { DURATION, RATIOS } from './config';
import { readParams } from './params';
import { overlay, type Player } from './dev/overlay';

declare global {
  interface Window {
    __explainer?: { ready: boolean; duration: number; renderAt: (t: number) => Promise<void> };
  }
}

async function main() {
  const p = readParams();
  const canvas = document.getElementById('film') as HTMLCanvasElement;
  const R = RATIOS[p.ratio];
  document.documentElement.style.setProperty('--ar', `${R.w} / ${R.h}`);
  if (p.exporting) document.body.classList.add('export');
  const app = await App.create(canvas, p.ratio, p.exporting);

  if (p.exporting) {
    window.__explainer = {
      ready: true,
      duration: DURATION,
      async renderAt(t: number) {
        app.renderAt(t);
        // Let the GPU finish before the frame is read.
        await new Promise((r) => requestAnimationFrame(() => r(null)));
      },
    };
    (window as unknown as { __app: App }).__app = app; // for scripts/determinism.ts's state dump
    app.renderAt(p.t);
    return;
  }

  const player: Player = {
    t: p.t,
    playing: p.play,
    seek(t) { this.t = Math.max(0, Math.min(DURATION, t)); app.renderAt(this.t); },
    toggle() { this.playing = !this.playing; },
  };
  const dev = p.clean ? null : overlay(player, p.ratio);
  if (p.clean) document.body.classList.add('clean');
  let last = performance.now();
  const loop = (now: number) => {
    // Wall time only advances the playhead; everything drawn is a function of the playhead.
    const dt = Math.min(0.1, (now - last) / 1000);
    last = now;
    if (player.playing) {
      player.t += dt;
      if (player.t >= DURATION) player.t = 0;
    }
    app.renderAt(player.t);
    dev?.tick();
    requestAnimationFrame(loop);
  };
  app.renderAt(player.t);
  document.body.classList.add('ready');
  requestAnimationFrame(loop);
}

main().catch((e) => {
  console.error(e);
  document.body.insertAdjacentHTML('beforeend', `<pre class="err">${String(e?.stack ?? e)}</pre>`);
});
