// Furniture for the portrait reels (1080 x 1920, for Instagram Reels and
// adverts): the safe area, a slim top bar, type that fits the width, and the
// closing card.
//
// Instagram lays its own interface over a reel: the header across the top,
// the caption and buttons across the bottom, and the like / comment / share
// column down the right of the lower half. So everything that must be read
// sits between SAFE.top and SAFE.bottom, and nothing important goes right of
// SAFE.right below SAFE.rail.

import { W, H, clamp, prog, ep, E } from './core.mjs';
import { text, kinetic, measure, rgba } from './gfx.mjs';
import { C, T, xMark, endCard } from './brand.mjs';
import { wrap } from './film.mjs';

export const RW = 1080;
export const RH = 1920;
export const SAFE = { top: 250, bottom: 1540, left: 72, right: 940, rail: 1100 };

/** A style whose size is reduced, if need be, so `str` fits in `maxW`. */
export function fit(ctx, str, style, maxW) {
  const w = measure(ctx, str, style);
  return w <= maxW ? style : { ...style, size: Math.floor((style.size * maxW) / w) };
}

/**
 * The top bar: the mark and the app on the left, context on the right, and a
 * segmented progress line above them (like Stories, with the film's own
 * chapters). opt: { app, accent, alpha, right, rightColor, progress, ticks,
 * tickColors }
 */
export function reelHud(ctx, t, opt) {
  const a = opt.alpha ?? 1;
  if (a <= 0) return;
  ctx.save();
  ctx.globalAlpha *= a;
  const x0 = SAFE.left, x1 = W - SAFE.left, y = SAFE.top - 34;
  // Progress, in segments.
  const bounds = [0, ...(opt.ticks || []), 1];
  const pr = clamp(opt.progress ?? 0);
  const gap = 8;
  for (let i = 0; i < bounds.length - 1; i++) {
    const sa = x0 + (x1 - x0) * bounds[i] + (i ? gap / 2 : 0);
    const sb = x0 + (x1 - x0) * bounds[i + 1] - (i < bounds.length - 2 ? gap / 2 : 0);
    ctx.fillStyle = rgba(C.white, 0.18);
    ctx.fillRect(sa, y, sb - sa, 3);
    const f = clamp((pr - bounds[i]) / (bounds[i + 1] - bounds[i]));
    if (f > 0) {
      ctx.fillStyle = opt.tickColors ? opt.tickColors[i] : opt.accent;
      ctx.fillRect(sa, y, (sb - sa) * f, 3);
    }
  }
  // The mark and the app.
  xMark(ctx, x0 + 16, y + 44, 26, C.white);
  const lab = T.label(20, C.soft);
  let x = x0 + 50;
  x += text(ctx, 'HYBRIDX', x, y + 52, lab) + 16;
  text(ctx, opt.app, x, y + 52, { ...lab, color: opt.accent });
  if (opt.right) text(ctx, opt.right, x1, y + 52, { ...T.label(20, opt.rightColor || C.soft), align: 'right' });
  ctx.restore();
}

/**
 * A reel headline: a title (one or more lines) and a wrapped supporting line,
 * sized for a phone. Returns the y below the block.
 */
export function reelHead(ctx, lt, a, b, x, y, { p0 = 0, q0 = 99, size = 112, width = SAFE.right - SAFE.left, color = C.white, align = 'left', subSize = 44 } = {}) {
  const q = prog(lt, q0, q0 + 0.35);
  const lines = Array.isArray(a) ? a : [a];
  const lh = size * 1.04;
  lines.forEach((ln, i) => {
    const st = fit(ctx, ln, { ...T.head(size, color), align }, width);
    kinetic(ctx, ln, x, y + i * lh, st, prog(lt, p0 + i * 0.08, p0 + 0.5 + i * 0.08), q, 'rise');
  });
  let yb = y + (lines.length - 1) * lh;
  if (b) {
    yb += size * 0.62 + subSize * 0.4;
    const s = { ...T.sub(subSize, C.soft), align };
    const ls = wrap(ctx, b, x, yb, width, s);
    ls.forEach((ln, i) => {
      kinetic(ctx, ln, x, yb + i * subSize * 1.3, s, prog(lt, p0 + 0.25 + i * 0.07, p0 + 0.85 + i * 0.07), q, 'fade');
    });
    yb += (ls.length - 1) * subSize * 1.3;
  }
  return yb;
}

/**
 * The closing card for a reel: a line to leave on, then the family end card
 * (lockup, "for UNA Watch", Race · Streak · Trail, the address), set larger
 * and higher so it clears Instagram's caption.
 */
export function reelEnd(ctx, t, opt) {
  const { tagline, accent } = opt;
  if (tagline) {
    const lines = Array.isArray(tagline) ? tagline : [tagline];
    lines.forEach((ln, i) => {
      const col = i === lines.length - 1 ? opt.tagColor || accent : C.white;
      const st = fit(ctx, ln, { ...T.hero(118, col), align: 'center' }, W - 2 * SAFE.left);
      kinetic(ctx, ln, W / 2, 560 + i * 128, st, ep(t, 0.1 + i * 0.12, 0.7 + i * 0.12, E.linear), 0, 'rise');
    });
  }
  endCard(ctx, t - 0.35, { ...opt, h: 128, cy: 950, ruleY: 1062, familyY: 1136, addrY: 1250, k: 1.55 });
}
