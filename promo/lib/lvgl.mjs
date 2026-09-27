// Watch-screen primitives in the display's own 240 x 240 space, matching the
// SDK's LVGL helpers (una-sdk Libs/Source/GUI/LVGL): label boxes placed by
// their top edge with LVGL's font metrics, the Title rule, the button-hint
// arcs, and arcs in TouchGFX's angle convention (0 = 12 o'clock, clockwise).

import { measure, roundRect } from './gfx.mjs';
import { C } from './brand.mjs';

/**
 * The Poppins faces the apps use. asc = line_height - base_line from the
 * generated LVGL font files: the distance from a label's top to its baseline.
 */
export const F = {
  Italic18: { weight: 400, italic: true, size: 18, asc: 18 },
  Italic20: { weight: 400, italic: true, size: 20, asc: 19 },
  Light60: { weight: 300, size: 60, asc: 59 },
  Medium18: { weight: 500, size: 18, asc: 17 },
  Medium25: { weight: 500, size: 25, asc: 24 },
  Medium40: { weight: 500, size: 40, asc: 39 },
  Regular14: { weight: 400, size: 14, asc: 14 },
  Regular16: { weight: 400, size: 16, asc: 16 },
  Regular18: { weight: 400, size: 18, asc: 18 },
  SemiBold20: { weight: 600, size: 20, asc: 18 },
  SemiBold25: { weight: 600, size: 25, asc: 24 },
  SemiBold30: { weight: 600, size: 30, asc: 29 },
  SemiBold35: { weight: 600, size: 35, asc: 34 },
  SemiBold40: { weight: 600, size: 40, asc: 39 },
  SemiBold60: { weight: 600, size: 60, asc: 57 },
};

export const deg = (d) => ((d - 90) * Math.PI) / 180; // TouchGFX degrees → canvas radians

// The apps' Poppins build has tabular figures (every digit as wide as "0"),
// which keeps a running clock from jittering; the Google build's are
// proportional, so digits are set in fixed cells here.
const HAS_DIGIT = /[0-9]/;

function runs(ctx, str, s) {
  applyFont(ctx, s);
  const dw = ctx.measureText('0').width;
  let w = 0;
  const out = [];
  for (const ch of str) {
    const isD = ch >= '0' && ch <= '9';
    const cw = isD ? dw : ctx.measureText(ch).width;
    out.push({ ch, x: w, w: cw, isD });
    w += cw;
  }
  return { out, w };
}

function applyFont(ctx, s) {
  ctx.font = `${s.italic ? 'italic ' : ''}${s.weight} ${s.size}px Poppins`;
  ctx.letterSpacing = '0px';
  ctx.textAlign = 'left';
  ctx.textBaseline = 'alphabetic';
}

/** Draw::label(): text in a box of width w whose top is y. Returns text width. */
export function label(ctx, f, str, x, y, w, align = 'center', color = C.white, alpha = 1) {
  const s = { size: f.size, weight: f.weight, italic: f.italic };
  const tab = HAS_DIGIT.test(str);
  const lay = tab ? runs(ctx, str, s) : null;
  const tw = tab ? lay.w : measure(ctx, str, s);
  const tx = align === 'center' ? x + (w - tw) / 2 : align === 'right' ? x + w - tw : x;
  ctx.save();
  ctx.fillStyle = color;
  ctx.globalAlpha *= alpha;
  if (tab) {
    applyFont(ctx, s);
    for (const r of lay.out) {
      const gx = r.isD ? tx + r.x + (r.w - ctx.measureText(r.ch).width) / 2 : tx + r.x;
      ctx.fillText(r.ch, gx, y + f.asc);
    }
  } else {
    ctx.fillText(str, tx, y + f.asc);
  }
  ctx.restore();
  return tw;
}

export function textWidth(ctx, f, str) {
  const s = { size: f.size, weight: f.weight, italic: f.italic };
  return HAS_DIGIT.test(str) ? runs(ctx, str, s).w : measure(ctx, str, s);
}

/** Draw::hline(): 3 px tall, fully rounded. */
export function hline(ctx, x, y, w, color) {
  ctx.fillStyle = color;
  roundRect(ctx, x, y, w, 3, 1.5);
  ctx.fill();
}

/** An arc in TouchGFX degrees, centre-line radius r, stroke width w. */
export function arc(ctx, cx, cy, r, w, a0, a1, color, rounded = true) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = w;
  ctx.lineCap = rounded ? 'round' : 'butt';
  ctx.beginPath();
  ctx.arc(cx, cy, r, deg(a0), deg(a1));
  ctx.stroke();
  ctx.restore();
}

export function disc(ctx, x, y, r, color) {
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.arc(x, y, r, 0, Math.PI * 2);
  ctx.fill();
}

export function ring(ctx, x, y, r, w, color) {
  ctx.strokeStyle = color;
  ctx.lineWidth = w;
  ctx.beginPath();
  ctx.arc(x, y, r - w / 2, 0, Math.PI * 2);
  ctx.stroke();
}

export function tri(ctx, a, b, c, color) {
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.moveTo(a[0], a[1]);
  ctx.lineTo(b[0], b[1]);
  ctx.lineTo(c[0], c[1]);
  ctx.closePath();
  ctx.fill();
}

export function line(ctx, a, b, w, color, cap = 'round') {
  ctx.strokeStyle = color;
  ctx.lineWidth = w;
  ctx.lineCap = cap;
  ctx.beginPath();
  ctx.moveTo(a[0], a[1]);
  ctx.lineTo(b[0], b[1]);
  ctx.stroke();
}

/** SDK::LVGL::Title: the label at (60, 11, 120) over a rule at (50, 38, 140). */
export function title(ctx, str, color = C.white, lineColor = C.grayDark, f = F.Italic18) {
  hline(ctx, 50, 38, 140, lineColor);
  label(ctx, f, str, 60, 11, 120, 'center', color);
}

// SDK::LVGL::Buttons: radius 113, width 6, in TouchGFX degrees.
const BTN_ARCS = { l1: [291, 308], l2: [232, 249], r1: [52, 69], r2: [112, 128] };
const BTN_COL = { white: C.white, amber: C.amber, red: C.red, green: C.chartreuse };

/** Button hints: { l1: 'white', r2: 'amber', ... }; a value may also be a colour. */
export function buttons(ctx, set, glow = {}) {
  for (const k of ['l1', 'l2', 'r1', 'r2']) {
    const v = set[k];
    if (!v) continue;
    const col = BTN_COL[v] || v;
    const [a0, a1] = BTN_ARCS[k];
    if (glow[k]) {
      ctx.save();
      ctx.shadowColor = col;
      ctx.shadowBlur = 10 * glow[k];
      arc(ctx, 120, 120, 113, 6 + 2 * glow[k], a0 - 3 * glow[k], a1 + 3 * glow[k], col);
      ctx.restore();
    } else {
      arc(ctx, 120, 120, 113, 6, a0, a1, col);
    }
  }
}

/** Where each physical button sits, in TouchGFX degrees (centre of its hint). */
export const BTN_ANGLE = { l1: 299.5, l2: 240.5, r1: 60.5, r2: 120 };

/** The green tick the SDK draws beside R1 (img_tickgreen_22x17), as a stroke. */
export function tick(ctx, x, y, color = C.chartreuse, s = 1) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = 4 * s;
  ctx.lineCap = 'round';
  ctx.lineJoin = 'round';
  ctx.beginPath();
  ctx.moveTo(x + 2 * s, y + 9 * s);
  ctx.lineTo(x + 8 * s, y + 15 * s);
  ctx.lineTo(x + 20 * s, y + 2 * s);
  ctx.stroke();
  ctx.restore();
}

export function cross(ctx, x, y, color = C.white, s = 1) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = 4 * s;
  ctx.lineCap = 'round';
  ctx.beginPath();
  ctx.moveTo(x + 2 * s, y + 2 * s);
  ctx.lineTo(x + 16 * s, y + 16 * s);
  ctx.moveTo(x + 16 * s, y + 2 * s);
  ctx.lineTo(x + 2 * s, y + 16 * s);
  ctx.stroke();
  ctx.restore();
}

/**
 * The SDK WheelMenu, reduced to what the films show: the selected item on a
 * teal band (y 87-153, clipped to a radius-110 disc) in SemiBold 30 with an
 * optional italic tip, and the next item below it in Medium 18.
 * shift: 0..1 scroll towards the next item.
 */
export function wheel(ctx, items, sel, shift = 0, band = C.tealDark) {
  const pitch = 66;
  ctx.save();
  // Band.
  ctx.save();
  ctx.beginPath();
  ctx.arc(120, 120, 110, 0, Math.PI * 2);
  ctx.clip();
  ctx.fillStyle = band;
  ctx.fillRect(0, 87, 240, 66);
  ctx.restore();
  const draw = (it, cy, big) => {
    if (!it) return;
    if (big) {
      if (it.tip) {
        label(ctx, F.SemiBold30, it.label, 0, cy - 30, 240, 'center', C.white);
        label(ctx, F.Italic18, it.tip, 0, cy + 3, 240, 'center', C.white);
      } else {
        label(ctx, F.SemiBold30, it.label, 0, cy - 19, 240, 'center', C.white);
      }
    } else if (it.tip) {
      label(ctx, F.Medium18, it.label, 0, cy - 22, 240, 'center', C.white);
      label(ctx, F.Italic18, it.tip, 0, cy - 2, 240, 'center', C.white);
    } else {
      label(ctx, F.Medium18, it.label, 0, cy - 11, 240, 'center', C.white);
    }
  };
  const n = items.length;
  const off = -shift * pitch;
  // Items above and below, clipped outside the band; the selected inside it.
  for (let k = -1; k <= 2; k++) {
    const i = sel + k;
    if (i < 0 || i >= n) continue;
    const cy = 120 + k * pitch + off;
    const inBand = Math.abs(cy - 120) < 33;
    ctx.save();
    ctx.beginPath();
    if (inBand) ctx.rect(0, 87, 240, 66);
    else {
      ctx.rect(0, 0, 240, 87);
      ctx.rect(0, 153, 240, 87);
    }
    ctx.clip();
    draw(items[i], cy, inBand);
    ctx.restore();
  }
  ctx.restore();
}
