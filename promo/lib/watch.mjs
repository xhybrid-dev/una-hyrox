// The watch around a live 240 x 240 screen.
//
// By default it is UNA's own watch: the renders from the SDK's Figma UI
// Resource Pack (una-sdk/Docs/Templates/Figma-UI-Kit), in graphite, teal or
// white, with the screen showing through the cut-out in the render. Those
// renders are UNA's artwork, so they are read from the SDK at render time
// (tools/una_mockups.py caches them in out/una/) and never committed here.
// Without the SDK, a generic round watch is drawn instead; its four buttons
// sit where the SDK's button hints point (L1 10 o'clock, L2 8, R1 2, R2 4).

import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { loadImage } from '@napi-rs/canvas';
import { clamp, lerp, TAU } from './core.mjs';
import { layer, addBloom, rgba, roundRect } from './gfx.mjs';
import { BTN_ANGLE } from './lvgl.mjs';

const HERE = path.dirname(fileURLToPath(import.meta.url));

/** UNA's renders by strap colour: { img, cx, cy, r, w, h } in render pixels. */
const UNA = await (async () => {
  const cache = path.join(HERE, '..', 'out', 'una');
  const metaFile = path.join(cache, 'meta.json');
  if (!fs.existsSync(metaFile)) {
    const sdk = process.env.UNA_SDK || path.join(HERE, '..', '..', 'una-sdk');
    const fig = path.join(sdk, 'Docs', 'Templates', 'Figma-UI-Kit', 'UNA-Watch-UI-Resource-Pack.fig');
    if (!fs.existsSync(fig)) return {};
    execFileSync('python3', [path.join(HERE, '..', 'tools', 'una_mockups.py'), sdk, cache], { stdio: 'ignore' });
  }
  const meta = JSON.parse(fs.readFileSync(metaFile, 'utf8'));
  const out = {};
  for (const [name, m] of Object.entries(meta)) {
    out[name] = { ...m, img: await loadImage(fs.readFileSync(path.join(cache, `${name}.png`))) };
  }
  return out;
})();

let MODEL = 'graphite';
/** Choose the strap for every watch drawn after this: graphite, teal or white. */
export function setWatchModel(name) {
  MODEL = name;
}
export const hasUnaWatch = () => Object.keys(UNA).length > 0;

// UNA's case is larger around its screen than the generic one, so its screen
// is drawn a little smaller to keep each film's layout.
const UNA_SCREEN = 0.86;
const unaOf = (opt) => (opt.generic ? null : UNA[opt.model || MODEL] || null);
const screenD = (opt) => (unaOf(opt) ? opt.d * UNA_SCREEN : opt.d);

const rad = (touchgfxDeg) => ((touchgfxDeg - 90) * Math.PI) / 180;

/**
 * Draw the watch. opt:
 *  cx, cy        centre of the display, in frame pixels
 *  d             display diameter in pixels (the 240 px screen maps to this)
 *  rot           rotation in radians
 *  screen(c, s)  draws the screen in 240-space; s is the pixel scale (d/240)
 *  press         { l1, l2, r1, r2 } 0..1, how far each button is pushed in
 *  bloom         0..1 glow of the screen's bright content
 *  strap         draw the strap (default true)
 *  shadow        drop shadow under the watch (default true)
 *  glass         glass sheen (default true)
 *  alpha         overall opacity
 *  on            0..1 screen brightness (0 = off)
 */
function genericWatch(ctx, opt) {
  const { cx, cy, d } = opt;
  const R = d / 2;
  const rot = opt.rot || 0;
  const alpha = opt.alpha ?? 1;
  if (alpha <= 0) return;
  ctx.save();
  ctx.globalAlpha *= alpha;
  ctx.translate(cx, cy);
  ctx.rotate(rot);

  // Strap: two bands leaving the case top and bottom, fading into the dark.
  if (opt.strap !== false) {
    const sw = R * 1.3;
    for (const dir of [-1, 1]) {
      const y0 = dir * R * 0.9;
      const y1 = dir * R * 3.6;
      const g = ctx.createLinearGradient(0, y0, 0, y1);
      g.addColorStop(0, '#141619');
      g.addColorStop(0.35, '#0c0d0f');
      g.addColorStop(1, 'rgba(8,9,10,0)');
      ctx.fillStyle = g;
      ctx.beginPath();
      ctx.moveTo(-sw / 2, y0);
      ctx.lineTo(sw / 2, y0);
      ctx.lineTo(sw / 2 * 0.98, y1);
      ctx.lineTo(-sw / 2 * 0.98, y1);
      ctx.closePath();
      ctx.fill();
      // Two faint grooves along the strap.
      const gg = ctx.createLinearGradient(0, y0, 0, y1);
      gg.addColorStop(0, 'rgba(255,255,255,0.05)');
      gg.addColorStop(0.6, 'rgba(255,255,255,0)');
      ctx.fillStyle = gg;
      ctx.fillRect(-sw / 2 + sw * 0.12, Math.min(y0, y1), 2, Math.abs(y1 - y0));
      ctx.fillRect(sw / 2 - sw * 0.12 - 2, Math.min(y0, y1), 2, Math.abs(y1 - y0));
    }
  }

  // Shadow under the case.
  if (opt.shadow !== false) {
    ctx.save();
    ctx.shadowColor = 'rgba(0,0,0,0.9)';
    ctx.shadowBlur = R * 0.35;
    ctx.shadowOffsetY = R * 0.06;
    ctx.fillStyle = '#0a0b0c';
    ctx.beginPath();
    ctx.arc(0, 0, R * 1.22, 0, TAU);
    ctx.fill();
    ctx.restore();
  }

  // Buttons, behind the case edge.
  const press = opt.press || {};
  for (const k of ['l1', 'l2', 'r1', 'r2']) {
    const a = rad(BTN_ANGLE[k]);
    const p = clamp(press[k] || 0);
    ctx.save();
    ctx.rotate(a);
    const inset = p * R * 0.045;
    const x0 = R * 1.16 - inset;
    const bw = R * 0.13, bh = R * 0.17;
    const g = ctx.createLinearGradient(0, -bh / 2, 0, bh / 2);
    g.addColorStop(0, '#4a4f56');
    g.addColorStop(0.5, '#2a2e33');
    g.addColorStop(1, '#16181b');
    ctx.fillStyle = g;
    roundRect(ctx, x0, -bh / 2, bw, bh, R * 0.03);
    ctx.fill();
    // Edge highlight.
    ctx.fillStyle = `rgba(255,255,255,${0.10 + 0.25 * p})`;
    ctx.fillRect(x0 + bw - R * 0.018, -bh / 2 + R * 0.02, R * 0.012, bh - R * 0.04);
    ctx.restore();
  }

  // Case: a graphite ring lit from the top left.
  const lightA = -Math.PI * 0.75 - rot;
  const lx = Math.cos(lightA), ly = Math.sin(lightA);
  const g = ctx.createLinearGradient(lx * R * 1.25, ly * R * 1.25, -lx * R * 1.25, -ly * R * 1.25);
  g.addColorStop(0, '#5b6068');
  g.addColorStop(0.28, '#2c3035');
  g.addColorStop(0.62, '#15171a');
  g.addColorStop(1, '#2a2d31');
  ctx.fillStyle = g;
  ctx.beginPath();
  ctx.arc(0, 0, R * 1.22, 0, TAU);
  ctx.fill();
  // Bevel: a thin bright rim and a darker inner step.
  ctx.lineWidth = Math.max(1, R * 0.012);
  const rg = ctx.createLinearGradient(lx * R, ly * R, -lx * R, -ly * R);
  rg.addColorStop(0, 'rgba(255,255,255,0.55)');
  rg.addColorStop(0.5, 'rgba(255,255,255,0.04)');
  rg.addColorStop(1, 'rgba(255,255,255,0.18)');
  ctx.strokeStyle = rg;
  ctx.beginPath();
  ctx.arc(0, 0, R * 1.215, 0, TAU);
  ctx.stroke();
  // Bezel ring.
  const bg = ctx.createLinearGradient(lx * R, ly * R, -lx * R, -ly * R);
  bg.addColorStop(0, '#1d2024');
  bg.addColorStop(1, '#0b0c0d');
  ctx.fillStyle = bg;
  ctx.beginPath();
  ctx.arc(0, 0, R * 1.13, 0, TAU);
  ctx.fill();
  ctx.strokeStyle = 'rgba(255,255,255,0.08)';
  ctx.lineWidth = Math.max(1, R * 0.006);
  ctx.beginPath();
  ctx.arc(0, 0, R * 1.13, 0, TAU);
  ctx.stroke();
  // Glass border (black mask around the active area).
  ctx.fillStyle = '#000';
  ctx.beginPath();
  ctx.arc(0, 0, R * 1.075, 0, TAU);
  ctx.fill();

  ctx.restore();

  // Screen: drawn in 240-space, clipped to the disc.
  const s = d / 240;
  const on = opt.on ?? 1;
  const drawScreen = (c) => {
    c.save();
    c.translate(cx, cy);
    c.rotate(rot);
    c.translate(-R, -R);
    c.scale(s, s);
    c.beginPath();
    c.arc(120, 120, 120, 0, TAU);
    c.clip();
    c.fillStyle = '#000';
    c.fillRect(0, 0, 240, 240);
    if (opt.screen && on > 0) {
      c.globalAlpha *= on;
      opt.screen(c, s);
    }
    c.restore();
  };
  ctx.save();
  ctx.globalAlpha *= alpha;
  if (opt.bloom && opt.bloom > 0) {
    const L = layer('watch-screen');
    drawScreen(L.ctx);
    ctx.drawImage(L.canvas, 0, 0);
    addBloom(ctx, L.canvas, opt.bloom * alpha, opt.bloomRadius || 22);
  } else {
    drawScreen(ctx);
  }
  ctx.restore();

  // Glass sheen: a faint diagonal band.
  if (opt.glass !== false) {
    ctx.save();
    ctx.globalAlpha *= alpha;
    ctx.translate(cx, cy);
    ctx.rotate(rot);
    ctx.beginPath();
    ctx.arc(0, 0, R * 1.075, 0, TAU);
    ctx.clip();
    const sg = ctx.createLinearGradient(-R, -R, R, R);
    sg.addColorStop(0, 'rgba(255,255,255,0.075)');
    sg.addColorStop(0.32, 'rgba(255,255,255,0.02)');
    sg.addColorStop(0.33, 'rgba(255,255,255,0)');
    sg.addColorStop(1, 'rgba(255,255,255,0)');
    ctx.fillStyle = sg;
    ctx.fillRect(-R * 1.1, -R * 1.1, R * 2.2, R * 2.2);
    ctx.restore();
  }
}

/**
 * Draw the watch. opt:
 *  cx, cy        centre of the display, in frame pixels
 *  d             display size in pixels (the 240 px screen maps to about this)
 *  rot           rotation in radians
 *  screen(c, s)  draws the screen in 240-space; s is the pixel scale
 *  press         { l1, l2, r1, r2 } 0..1, button presses (a glint on UNA's)
 *  bloom         0..1 glow of the screen's bright content
 *  strap, shadow, glass, alpha, on   as before; model overrides the strap
 */
export function watch(ctx, opt) {
  const u = unaOf(opt);
  if (!u) return genericWatch(ctx, opt);
  const alpha = opt.alpha ?? 1;
  if (alpha <= 0) return;
  const d = screenD(opt);
  const R = d / 2;
  const rot = opt.rot || 0;
  const k = R / u.r;
  // Shadow.
  if (opt.shadow !== false) {
    ctx.save();
    ctx.globalAlpha *= alpha;
    ctx.shadowColor = 'rgba(0,0,0,0.85)';
    ctx.shadowBlur = R * 0.5;
    ctx.shadowOffsetY = R * 0.08;
    ctx.fillStyle = '#050505';
    ctx.beginPath();
    ctx.arc(opt.cx, opt.cy, R * 1.45, 0, TAU);
    ctx.fill();
    ctx.restore();
  }
  // The screen, under the render's cut-out.
  drawScreen(ctx, { ...opt, d }, alpha);
  // The watch itself.
  ctx.save();
  ctx.globalAlpha *= alpha;
  ctx.translate(opt.cx, opt.cy);
  ctx.rotate(rot);
  ctx.imageSmoothingEnabled = true;
  ctx.imageSmoothingQuality = 'high';
  if (opt.strap === false) {
    // Crop to the case: a disc a little wider than the lugs.
    ctx.beginPath();
    ctx.arc(0, 0, R * 1.62, 0, TAU);
    ctx.clip();
  }
  ctx.drawImage(u.img, -u.cx * k, -u.cy * k, u.w * k, u.h * k);
  // A glint on a pressed button.
  const press = opt.press || {};
  for (const key of ['l1', 'l2', 'r1', 'r2']) {
    const p = clamp(press[key] || 0);
    if (p <= 0) continue;
    const a = rad(BTN_ANGLE[key]);
    const bx = Math.cos(a) * R * 1.5, by = Math.sin(a) * R * 1.5;
    const g = ctx.createRadialGradient(bx, by, 0, bx, by, R * 0.28);
    g.addColorStop(0, `rgba(255,255,255,${0.45 * p})`);
    g.addColorStop(1, 'rgba(255,255,255,0)');
    ctx.fillStyle = g;
    ctx.fillRect(bx - R * 0.3, by - R * 0.3, R * 0.6, R * 0.6);
  }
  ctx.restore();
}

/** The screen in 240-space, clipped to its disc, with bloom and glass. */
function drawScreen(ctx, opt, alpha) {
  const { cx, cy, d } = opt;
  const R = d / 2;
  const s = d / 240;
  const rot = opt.rot || 0;
  const on = opt.on ?? 1;
  const paint = (c) => {
    c.save();
    c.translate(cx, cy);
    c.rotate(rot);
    c.translate(-R, -R);
    c.scale(s, s);
    c.beginPath();
    c.arc(120, 120, 121, 0, TAU);
    c.clip();
    c.fillStyle = '#000';
    c.fillRect(0, 0, 240, 240);
    if (opt.screen && on > 0) {
      c.globalAlpha *= on;
      opt.screen(c, s);
    }
    c.restore();
  };
  ctx.save();
  ctx.globalAlpha *= alpha;
  if (opt.bloom && opt.bloom > 0) {
    const L = layer('watch-screen');
    paint(L.ctx);
    ctx.drawImage(L.canvas, 0, 0);
    addBloom(ctx, L.canvas, opt.bloom * alpha, opt.bloomRadius || 22);
  } else {
    paint(ctx);
  }
  if (opt.glass !== false) {
    ctx.translate(cx, cy);
    ctx.rotate(rot);
    ctx.beginPath();
    ctx.arc(0, 0, R, 0, TAU);
    ctx.clip();
    const sg = ctx.createLinearGradient(-R, -R, R, R);
    sg.addColorStop(0, 'rgba(255,255,255,0.07)');
    sg.addColorStop(0.32, 'rgba(255,255,255,0.02)');
    sg.addColorStop(0.33, 'rgba(255,255,255,0)');
    sg.addColorStop(1, 'rgba(255,255,255,0)');
    ctx.fillStyle = sg;
    ctx.fillRect(-R, -R, R * 2, R * 2);
  }
  ctx.restore();
}

/** Where a button is on screen for a watch drawn with these options. */
export function buttonPos(opt, k, out = 1.28) {
  const R = screenD(opt) / 2;
  if (unaOf(opt)) out *= 1.2; // UNA's buttons stand further out
  const a = rad(BTN_ANGLE[k]) + (opt.rot || 0);
  return [opt.cx + Math.cos(a) * R * out, opt.cy + Math.sin(a) * R * out];
}

/** Map a point in 240-space to frame pixels for a watch drawn with opt. */
export function screenToFrame(opt, x, y) {
  const s = screenD(opt) / 240;
  const rot = opt.rot || 0;
  const dx = (x - 120) * s, dy = (y - 120) * s;
  return [opt.cx + dx * Math.cos(rot) - dy * Math.sin(rot), opt.cy + dx * Math.sin(rot) + dy * Math.cos(rot)];
}

/** Rings that ripple out from a point: a button press, a buzz. */
export function ripple(ctx, x, y, t, color, { rings = 3, spread = 0.12, speed = 420, life = 0.7, width = 3 } = {}) {
  for (let i = 0; i < rings; i++) {
    const tt = t - i * spread;
    if (tt < 0 || tt > life) continue;
    const r = 10 + tt * speed;
    const a = 1 - tt / life;
    ctx.save();
    ctx.strokeStyle = rgba(color, a * a);
    ctx.lineWidth = width * (1 - tt / life) + 0.5;
    ctx.beginPath();
    ctx.arc(x, y, r, 0, TAU);
    ctx.stroke();
    ctx.restore();
  }
}

/** A haptic buzz: a tiny, fast, decaying shake offset in pixels. */
export function buzz(t, amp = 6, dur = 0.35, freq = 38) {
  if (t < 0 || t > dur) return [0, 0];
  const env = Math.pow(1 - t / dur, 2);
  return [Math.sin(t * TAU * freq) * amp * env, Math.cos(t * TAU * freq * 1.3) * amp * 0.4 * env];
}

export { lerp };
