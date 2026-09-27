// Canvas helpers: fonts, type with tracking, kinetic-type reveals, layers and
// glow. Everything draws into a Skia 2D context (@napi-rs/canvas).

import { createCanvas, GlobalFonts } from '@napi-rs/canvas';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
import { W, H, clamp, prog, E, hash } from './core.mjs';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const MODS = path.join(HERE, '..', 'node_modules', '@expo-google-fonts');

let fontsReady = false;
export function setupFonts() {
  if (fontsReady) return;
  const pop = ['100Thin', '200ExtraLight', '300Light', '400Regular', '500Medium', '600SemiBold', '700Bold',
    '800ExtraBold', '300Light_Italic', '400Regular_Italic', '500Medium_Italic', '600SemiBold_Italic'];
  for (const w of pop) GlobalFonts.registerFromPath(path.join(MODS, 'poppins', w, `Poppins_${w}.ttf`), 'Poppins');
  for (const w of ['300Light', '400Regular', '500Medium', '700Bold']) {
    GlobalFonts.registerFromPath(path.join(MODS, 'jetbrains-mono', w, `JetBrainsMono_${w}.ttf`), 'JetBrains Mono');
  }
  fontsReady = true;
}

export { createCanvas };

// --- Type -------------------------------------------------------------------

/**
 * A type style: { size, weight, italic, mono, color, tracking (em), align,
 * baseline }. `font()` turns it into a CSS font string.
 */
export function font(s) {
  const fam = s.mono ? '"JetBrains Mono"' : 'Poppins';
  return `${s.italic ? 'italic ' : ''}${s.weight || 400} ${s.size}px ${fam}`;
}

export function applyType(ctx, s) {
  ctx.font = font(s);
  ctx.letterSpacing = `${(s.tracking || 0) * s.size}px`;
  ctx.textAlign = 'left';
  ctx.textBaseline = s.baseline || 'alphabetic';
}

export function measure(ctx, str, s) {
  applyType(ctx, s);
  // Letter-spacing adds space after every glyph, the last included; drop it
  // so centred text is truly centred.
  return ctx.measureText(str).width - (s.tracking || 0) * s.size;
}

/** Plain text at (x, y) with the style's alignment. Returns its width. */
export function text(ctx, str, x, y, s) {
  const w = measure(ctx, str, s);
  const x0 = s.align === 'center' ? x - w / 2 : s.align === 'right' ? x - w : x;
  ctx.fillStyle = s.color || '#fff';
  const a = ctx.globalAlpha;
  if (s.alpha !== undefined) ctx.globalAlpha = a * s.alpha;
  ctx.fillText(str, x0, y);
  ctx.globalAlpha = a;
  return w;
}

/** Left edge of each character, from prefix widths (keeps kerning). */
export function charXs(ctx, str, s) {
  applyType(ctx, s);
  const xs = [];
  for (let i = 0; i < str.length; i++) {
    xs.push(i === 0 ? 0 : ctx.measureText(str.slice(0, i)).width);
  }
  return xs;
}

/**
 * Kinetic type. p is the reveal progress 0..1 and q the exit progress 0..1.
 * Modes:
 *  - 'rise':  the line rises into place behind a mask (the classic).
 *  - 'chars': characters rise and fade in one after another.
 *  - 'scramble': monospace characters flicker through glyphs, then lock.
 *  - 'type':  typed on, with a block cursor while typing.
 *  - 'wipe':  a hard-edged mask wipes left to right.
 *  - 'fade':  opacity with a small drift.
 */
export function kinetic(ctx, str, x, y, s, p, q = 0, mode = 'rise', opt = {}) {
  if (p <= 0 || q >= 1) return;
  const w = measure(ctx, str, s);
  const x0 = s.align === 'center' ? x - w / 2 : s.align === 'right' ? x - w : x;
  const size = s.size;
  ctx.save();
  ctx.fillStyle = s.color || '#fff';
  if (s.alpha !== undefined) ctx.globalAlpha *= s.alpha;

  if (mode === 'rise') {
    const ein = E.outExpo(p);
    const eout = E.inExpo(q);
    ctx.beginPath();
    ctx.rect(x0 - size, y - size * 1.12, w + size * 2, size * 1.45);
    ctx.clip();
    const dy = (1 - ein) * size * 1.25 - eout * size * 1.25;
    ctx.fillText(str, x0, y + dy);
  } else if (mode === 'chars') {
    const xs = charXs(ctx, str, s);
    const n = str.length;
    const spread = opt.spread ?? 0.6;
    for (let i = 0; i < n; i++) {
      const d = n > 1 ? (i / (n - 1)) * spread : 0;
      const pi = clamp((p - d) / (1 - spread));
      const qi = clamp((q - d) / (1 - spread));
      if (pi <= 0 || qi >= 1) continue;
      const a = E.outCubic(pi) * (1 - E.inCubic(qi));
      const dy = (1 - E.outExpo(pi)) * size * 0.45 - E.inExpo(qi) * size * 0.3;
      ctx.globalAlpha = (s.alpha ?? 1) * a;
      ctx.fillText(str[i], x0 + xs[i], y + dy);
    }
  } else if (mode === 'scramble') {
    const glyphs = opt.glyphs || 'ABCDEFGHJKLMNPQRSTUVWXYZ0123456789/:·-';
    const xs = charXs(ctx, str, s);
    const n = str.length;
    const seed = opt.seed ?? 7;
    const frame = Math.floor((opt.t ?? p * 10) * 30);
    for (let i = 0; i < n; i++) {
      const lockAt = 0.25 + (i / Math.max(1, n - 1)) * 0.7;
      const showAt = (i / Math.max(1, n - 1)) * 0.3;
      if (p < showAt) continue;
      const qi = clamp((q - (i / n) * 0.5) / 0.5);
      if (qi >= 1) continue;
      let ch = str[i];
      if (ch !== ' ' && (p < lockAt || qi > 0)) {
        ch = glyphs[Math.floor(hash(seed + i * 31, frame + i) * glyphs.length)];
      }
      ctx.globalAlpha = (s.alpha ?? 1) * (p < lockAt ? 0.55 : 1) * (1 - qi);
      ctx.fillText(ch, x0 + xs[i], y);
    }
  } else if (mode === 'type') {
    const n = Math.floor(clamp(p) * str.length + 0.0001);
    const vis = str.slice(0, n);
    const qa = 1 - E.inCubic(q);
    ctx.globalAlpha *= qa;
    ctx.fillText(vis, x0, y);
    if (p < 1 || opt.cursor) {
      const cw = ctx.measureText(vis).width;
      const blink = opt.t !== undefined ? (Math.floor(opt.t * 2.5) % 2 === 0 ? 1 : 0.15) : 1;
      ctx.globalAlpha *= blink;
      ctx.fillRect(x0 + cw + size * 0.08, y - size * 0.78, size * 0.5, size * 0.92);
    }
  } else if (mode === 'wipe') {
    const a = E.inOutQuart(p);
    const b = E.inOutQuart(q);
    ctx.beginPath();
    ctx.rect(x0 - 4 + (w + 8) * b, y - size * 1.1, (w + 8) * (a - b), size * 1.5);
    ctx.clip();
    ctx.fillText(str, x0, y);
  } else {
    const a = E.outCubic(p) * (1 - E.inCubic(q));
    ctx.globalAlpha *= a;
    ctx.fillText(str, x0, y + (1 - E.outCubic(p)) * size * 0.2 - E.inCubic(q) * size * 0.2);
  }
  ctx.restore();
}

// --- Layers -----------------------------------------------------------------

const pool = new Map();
/** A reusable offscreen canvas, cleared, at full frame size unless given. */
export function layer(key, w = W, h = H) {
  const k = `${key}:${w}x${h}`;
  let l = pool.get(k);
  if (!l) {
    const c = createCanvas(w, h);
    l = { canvas: c, ctx: c.getContext('2d'), w, h };
    pool.set(k, l);
  }
  l.ctx.setTransform(1, 0, 0, 1, 0, 0);
  l.ctx.globalAlpha = 1;
  l.ctx.globalCompositeOperation = 'source-over';
  l.ctx.filter = 'none';
  l.ctx.clearRect(0, 0, l.w, l.h);
  return l;
}

/**
 * Draw `fn` into a layer and composite it with opacity, blend mode and an
 * optional cheap bloom (blurred at quarter size and added on top).
 */
export function group(ctx, key, fn, { alpha = 1, blend = 'source-over', bloom = 0, bloomRadius = 18, clip = null } = {}) {
  if (alpha <= 0) return;
  const l = layer(key);
  fn(l.ctx);
  ctx.save();
  if (clip) {
    ctx.beginPath();
    clip(ctx);
    ctx.clip();
  }
  ctx.globalAlpha = alpha;
  ctx.globalCompositeOperation = blend;
  ctx.drawImage(l.canvas, 0, 0);
  if (bloom > 0) addBloom(ctx, l.canvas, bloom * alpha, bloomRadius);
  ctx.restore();
}

/** Add a soft glow of a canvas's bright content onto ctx. */
export function addBloom(ctx, src, amount, radius = 18) {
  const q = layer('bloomq', W / 4, H / 4);
  q.ctx.drawImage(src, 0, 0, W / 4, H / 4);
  const b = layer('bloomb', W / 4, H / 4);
  b.ctx.filter = `blur(${Math.max(1, radius / 4)}px)`;
  b.ctx.drawImage(q.canvas, 0, 0);
  b.ctx.filter = 'none';
  ctx.save();
  ctx.globalCompositeOperation = 'lighter';
  ctx.globalAlpha = clamp(amount);
  ctx.imageSmoothingEnabled = true;
  ctx.imageSmoothingQuality = 'high';
  ctx.drawImage(b.canvas, 0, 0, W, H);
  ctx.restore();
}

/** Run fn with a shadow glow. */
export function glow(ctx, color, blur, fn) {
  ctx.save();
  ctx.shadowColor = color;
  ctx.shadowBlur = blur;
  fn();
  ctx.restore();
}

// --- Shapes -----------------------------------------------------------------

export function roundRect(ctx, x, y, w, h, r) {
  r = Math.min(r, w / 2, h / 2);
  ctx.beginPath();
  ctx.moveTo(x + r, y);
  ctx.arcTo(x + w, y, x + w, y + h, r);
  ctx.arcTo(x + w, y + h, x, y + h, r);
  ctx.arcTo(x, y + h, x, y, r);
  ctx.arcTo(x, y, x + w, y, r);
  ctx.closePath();
}

export function poly(ctx, pts, close = true) {
  ctx.beginPath();
  pts.forEach(([x, y], i) => (i ? ctx.lineTo(x, y) : ctx.moveTo(x, y)));
  if (close) ctx.closePath();
}

/** Stroke a polyline, drawn from progress a to b of its length (0..1). */
export function strokePart(ctx, pts, a, b) {
  if (b <= a || pts.length < 2) return;
  const lens = [0];
  for (let i = 1; i < pts.length; i++) {
    lens.push(lens[i - 1] + Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]));
  }
  const total = lens[lens.length - 1];
  const s = a * total, e = b * total;
  ctx.beginPath();
  let started = false;
  for (let i = 1; i < pts.length; i++) {
    const l0 = lens[i - 1], l1 = lens[i];
    if (l1 < s || l0 > e) continue;
    const t0 = clamp((s - l0) / (l1 - l0 || 1));
    const t1 = clamp((e - l0) / (l1 - l0 || 1));
    const p0 = [pts[i - 1][0] + (pts[i][0] - pts[i - 1][0]) * t0, pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * t0];
    const p1 = [pts[i - 1][0] + (pts[i][0] - pts[i - 1][0]) * t1, pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * t1];
    if (!started) {
      ctx.moveTo(p0[0], p0[1]);
      started = true;
    }
    ctx.lineTo(p1[0], p1[1]);
  }
  ctx.stroke();
}

/** Point at fraction f of a polyline's length, with its direction. */
export function pointAt(pts, f) {
  const lens = [0];
  for (let i = 1; i < pts.length; i++) {
    lens.push(lens[i - 1] + Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]));
  }
  const target = clamp(f) * lens[lens.length - 1];
  for (let i = 1; i < pts.length; i++) {
    if (target <= lens[i] || i === pts.length - 1) {
      const t = clamp((target - lens[i - 1]) / (lens[i] - lens[i - 1] || 1));
      const dx = pts[i][0] - pts[i - 1][0], dy = pts[i][1] - pts[i - 1][1];
      return { x: pts[i - 1][0] + dx * t, y: pts[i - 1][1] + dy * t, a: Math.atan2(dy, dx), i };
    }
  }
  return { x: pts[0][0], y: pts[0][1], a: 0, i: 0 };
}

export function polyLength(pts) {
  let s = 0;
  for (let i = 1; i < pts.length; i++) s += Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]);
  return s;
}

/** A soft radial light: for glows behind things. */
export function light(ctx, x, y, r, color, alpha = 1) {
  if (alpha <= 0 || r <= 0) return;
  const g = ctx.createRadialGradient(x, y, 0, x, y, r);
  g.addColorStop(0, color);
  g.addColorStop(1, 'rgba(0,0,0,0)');
  ctx.save();
  ctx.globalAlpha = clamp(alpha);
  ctx.globalCompositeOperation = 'lighter';
  ctx.fillStyle = g;
  ctx.fillRect(x - r, y - r, r * 2, r * 2);
  ctx.restore();
}

/** '#RRGGBB' + alpha → rgba() */
export function rgba(hex, a = 1) {
  const n = parseInt(hex.slice(1), 16);
  return `rgba(${(n >> 16) & 255},${(n >> 8) & 255},${n & 255},${a})`;
}

export function vignette(ctx, strength = 0.55) {
  const g = ctx.createRadialGradient(W / 2, H / 2, H * 0.35, W / 2, H / 2, H * 1.05);
  g.addColorStop(0, 'rgba(0,0,0,0)');
  g.addColorStop(1, `rgba(0,0,0,${strength})`);
  ctx.save();
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, W, H);
  ctx.restore();
}

export { prog };
