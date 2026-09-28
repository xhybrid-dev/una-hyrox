// Typographic and interaction helpers shared by the three films.

import { clamp, lerp, prog, E, TAU } from './core.mjs';
import { kinetic, measure, rgba } from './gfx.mjs';
import { C, T } from './brand.mjs';

/** A headline pair: title (large) and a supporting line. */
export function headline(ctx, lt, a, b, x, y, { p0 = 0, q0 = 99, size = 104, width = 760, color = C.white } = {}) {
  const q = prog(lt, q0, q0 + 0.4);
  const lines = Array.isArray(a) ? a : [a];
  lines.forEach((ln, i) => {
    kinetic(ctx, ln, x, y + i * size * 1.05, T.head(size, color), prog(lt, p0 + i * 0.08, p0 + 0.6 + i * 0.08), q, 'rise');
  });
  if (b) {
    const yb = y + (lines.length - 1) * size * 1.05 + size * 0.75;
    wrap(ctx, b, x, yb, width, T.sub(34, C.soft)).forEach((ln, i) => {
      kinetic(ctx, ln, x, yb + i * 48, T.sub(34, C.soft), prog(lt, p0 + 0.3 + i * 0.07, p0 + 1.0 + i * 0.07), q, 'fade');
    });
  }
}

export function wrap(ctx, str, x, y, width, s) {
  const words = str.split(' ');
  const lines = [];
  let cur = '';
  for (const w of words) {
    const tryL = cur ? `${cur} ${w}` : w;
    if (measure(ctx, tryL, s) > width && cur) {
      lines.push(cur);
      cur = w;
    } else cur = tryL;
  }
  if (cur) lines.push(cur);
  return lines;
}

/** A labelled callout line from a point to a label. */
export function callout(ctx, from, to, label, p, color = C.amber) {
  if (p <= 0) return;
  const e = E.outCubic(p);
  ctx.save();
  ctx.strokeStyle = rgba(color, 0.9);
  ctx.lineWidth = 2;
  const mx = lerp(from[0], to[0], e), my = lerp(from[1], to[1], e);
  ctx.beginPath();
  ctx.moveTo(from[0], from[1]);
  ctx.lineTo(mx, my);
  ctx.stroke();
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.arc(from[0], from[1], 5, 0, TAU);
  ctx.fill();
  ctx.restore();
  kinetic(ctx, label, to[0] + 14, to[1] + 7, T.label(20, color), prog(p, 0.5, 1), 0, 'scramble', { t: p * 3 });
}

/** A button press: travel in and out, given seconds since the press. */
export function pressAt(dt) {
  if (dt < -0.12 || dt > 0.5) return 0;
  return dt < 0 ? E.outCubic(1 + dt / 0.12) : 1 - E.inOutCubic(clamp(dt / 0.25));
}
