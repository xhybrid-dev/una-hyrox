// The SDK's LVGL widgets as the apps use them, in the display's own 240 x 240 space
// (una-sdk/Libs/Source/GUI/LVGL: Draw, Title, Buttons, ScrollIndicator, WheelMenu, Toggle).
// Angles are TouchGFX degrees: 0 = 12 o'clock, clockwise.

import { WATCH } from '../config';
import { drawText, lineHeight, type Align, type FontKey } from './font';

export const C = WATCH;
export type Ctx = CanvasRenderingContext2D;

export const label = (ctx: Ctx, f: FontKey, s: string, x: number, y: number, w: number, align: Align = 'center', color: string = C.WHITE, wrap = false) =>
  drawText(ctx, f, s, x, y, w, align, color, wrap);

const rad = (deg: number) => ((deg - 90) * Math.PI) / 180;

/**
 * Draw::arc: lv_arc's box is sized from the centre-line radius plus half the stroke, rounded up,
 * and the stroke is drawn inward from the box's edge.
 */
export function arc(ctx: Ctx, cx: number, cy: number, radius: number, width: number, a0: number, a1: number, color: string, rounded = true) {
  const outer = radius + Math.floor((width + 1) / 2);
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = width;
  ctx.lineCap = rounded ? 'round' : 'butt';
  ctx.beginPath();
  ctx.arc(cx, cy, outer - width / 2, rad(a0), rad(a1));
  ctx.stroke();
  ctx.restore();
}

export function disc(ctx: Ctx, x: number, y: number, r: number, color: string) {
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.arc(x, y, r, 0, Math.PI * 2);
  ctx.fill();
}

export function ring(ctx: Ctx, x: number, y: number, r: number, w: number, color: string) {
  ctx.strokeStyle = color;
  ctx.lineWidth = w;
  ctx.beginPath();
  ctx.arc(x, y, r - w / 2, 0, Math.PI * 2);
  ctx.stroke();
}

export type Pt = [number, number];

export function tri(ctx: Ctx, a: Pt, b: Pt, c: Pt, color: string) {
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.moveTo(a[0], a[1]);
  ctx.lineTo(b[0], b[1]);
  ctx.lineTo(c[0], c[1]);
  ctx.closePath();
  ctx.fill();
}

export function line(ctx: Ctx, a: Pt, b: Pt, w: number, color: string, cap: CanvasLineCap = 'round') {
  ctx.strokeStyle = color;
  ctx.lineWidth = w;
  ctx.lineCap = cap;
  ctx.beginPath();
  ctx.moveTo(a[0], a[1]);
  ctx.lineTo(b[0], b[1]);
  ctx.stroke();
}

/** Draw::hline: 3 px tall, fully rounded. */
export function hline(ctx: Ctx, x: number, y: number, w: number, color: string) {
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.roundRect(x, y, w, 3, 1.5);
  ctx.fill();
}

/** SDK::LVGL::Title: the label at (60, 11, 120) over a rule at (50, 38, 140), in the app's Italic 18. */
export function title(ctx: Ctx, s: string, color: string = C.WHITE) {
  hline(ctx, 50, 38, 140, C.GRAY_DARK);
  label(ctx, 'italic_18', s, 60, 11, 120, 'center', color);
}

// SDK::LVGL::Buttons: kButtonRadius 113, kButtonWidth 6, kButtonArcs.
export type Btn = 'l1' | 'l2' | 'r1' | 'r2';
const BTN_ARCS: Record<Btn, [number, number]> = { l1: [291, 308], l2: [232, 249], r1: [52, 69], r2: [112, 128] };
const BTN_COL = { white: C.WHITE, amber: C.YELLOW_DARK, red: C.RED, green: C.CHARTREUSE } as const;
export type BtnHint = keyof typeof BTN_COL;
/** The centre of each button's hint, in TouchGFX degrees: where the button is. */
export const BTN_ANGLE: Record<Btn, number> = { l1: 299.5, l2: 240.5, r1: 60.5, r2: 120 };

export function buttons(ctx: Ctx, set: Partial<Record<Btn, BtnHint>>) {
  (Object.keys(set) as Btn[]).forEach((k) => {
    const v = set[k];
    if (v) arc(ctx, 120, 120, 113, 6, BTN_ARCS[k][0], BTN_ARCS[k][1], BTN_COL[v]);
  });
}

/** ScrollIndicator::kBig beside a wheel: rail 232-308, an 18-degree handle, item 0 at the top. */
export function scrollRail(ctx: Ctx, index: number, count: number) {
  arc(ctx, 120, 120, 112, 9, 232, 308, C.GRAY_DARK);
  if (count <= 1) return;
  const step = (308 - 232 - 18) / (count - 1);
  const start = Math.trunc(308 - 18 - step * index + 0.5);
  arc(ctx, 120, 120, 112, 9, start, Math.trunc(start + 18), C.WHITE);
}

export type WheelItem = { text: string; tip?: string; tipColor?: string; centerTipColor?: string; toggle?: boolean; toggleOn?: boolean };
export type WheelFonts = { center: FontKey; item: FontKey; tip: FontKey };
export const RACE_WHEEL: WheelFonts = { center: 'semibold_30', item: 'medium_18', tip: 'italic_18' };
export const STREAK_WHEEL: WheelFonts = { center: 'semibold_25', item: 'medium_18', tip: 'italic_18' };

/**
 * WheelMenu at rest: the selected item on the teal lens (a radius-110 disc clipped to the band at
 * y 87-153), the next one below it (y 153-219). renderSlot()'s layout: Simple centred in the 66 px
 * slot; Tip with the label centred in the top half (+3 selected, +4 below) and the tip in the
 * bottom half (-3, -4).
 */
export function wheel(ctx: Ctx, items: WheelItem[], sel: number, fonts: WheelFonts = RACE_WHEEL) {
  ctx.save();
  ctx.beginPath();
  ctx.rect(16, 87, 220, 66);
  ctx.clip();
  disc(ctx, 16 + 110, 87 + 33, 110, C.TEAL_DARK);
  ctx.restore();
  const slot = (it: WheelItem | undefined, y0: number, center: boolean) => {
    if (!it) return;
    const f = center ? fonts.center : fonts.item;
    const lines = it.text.split('\n').length;
    const h = lineHeight(f) * lines;
    if (it.tip) {
      const ty = y0 + Math.trunc((33 - h) / 2) + (center ? 3 : 4);
      label(ctx, f, it.text, 0, ty, 240);
      const th = lineHeight(fonts.tip);
      const tipY = y0 + 33 + Math.trunc((33 - th) / 2) + (center ? -3 : -4);
      label(ctx, fonts.tip, it.tip, 0, tipY, 240, 'center', center ? (it.centerTipColor ?? C.WHITE) : (it.tipColor ?? C.WHITE));
    } else {
      label(ctx, f, it.text, 0, y0 + Math.trunc((66 - h) / 2), 240);
    }
  };
  slot(items[sel], 87, true);
  slot(items[sel + 1], 153, false);
  scrollRail(ctx, sel, items.length);
}

/** img_tickgreen_22x17 beside R1, as a stroke. */
export function tick(ctx: Ctx, x: number, y: number, color: string) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = 4;
  ctx.lineCap = 'round';
  ctx.lineJoin = 'round';
  ctx.beginPath();
  ctx.moveTo(x + 2, y + 9);
  ctx.lineTo(x + 8, y + 15);
  ctx.lineTo(x + 20, y + 2);
  ctx.stroke();
  ctx.restore();
}

/** img_circletick_50x50 (tinted), drawn on by p (0..1). */
export function circleTick(ctx: Ctx, cx: number, cy: number, color: string, p = 1) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = 5;
  ctx.lineCap = 'round';
  ctx.lineJoin = 'round';
  ctx.beginPath();
  ctx.arc(cx, cy, 22, -Math.PI / 2, -Math.PI / 2 + Math.PI * 2 * Math.min(1, p * 1.4));
  ctx.stroke();
  if (p > 0.55) {
    const q = Math.min(1, (p - 0.55) / 0.35);
    ctx.beginPath();
    ctx.moveTo(cx - 11, cy + 1);
    const m: Pt = [cx - 3, cy + 9], e: Pt = [cx + 12, cy - 7];
    if (q < 0.4) ctx.lineTo(cx - 11 + (m[0] - cx + 11) * (q / 0.4), cy + 1 + (m[1] - cy - 1) * (q / 0.4));
    else {
      ctx.lineTo(m[0], m[1]);
      const r = (q - 0.4) / 0.6;
      ctx.lineTo(m[0] + (e[0] - m[0]) * r, m[1] + (e[1] - m[1]) * r);
    }
    ctx.stroke();
  }
  ctx.restore();
}
