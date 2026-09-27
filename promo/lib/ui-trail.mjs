// HybridX Trail's screens: CONCEPT designs for the films, drawn in the same
// system as Race and Streak (240 x 240, the SDK's 64 colours, Poppins, the
// SDK Title, Buttons and WheelMenu). The app is at its T0 probe; its real
// screens are designed in phase T3 of the brief. Features shown are the
// brief's v1 list (docs/HYBRIDX_TRAIL_BRIEF.md §2).

import { C } from './brand.mjs';
import { F, label, title, buttons, wheel, disc, ring, line, tri, arc, textWidth } from './lvgl.mjs';
import { clamp, lerp, TAU } from './core.mjs';
import { ROUTE, routeAt, routeSlice, routePoint } from './terrain.mjs';

export const ROUTE_LINE = C.pink; // ORCHID: the line to follow
export const ROUTE_DONE = '#AA00AA'; // PURPLE: the line behind you
export const START = C.chartreuse;

export const ROUTES = [
  { label: 'Ridge loop', tip: '14.2 km · 650 m' },
  { label: 'Coast path', tip: '21.1 km · 310 m' },
  { label: 'Sunday long', tip: '18.0 km · 240 m' },
  { label: 'Hill reps', tip: '6.4 km · 410 m' },
];

/** The route list: a WheelMenu of the GPX files in Routes/. */
export function routeList(ctx, s = {}) {
  title(ctx, 'Routes');
  wheel(ctx, ROUTES, s.sel || 0, s.shift || 0);
  // Scroll rail, as the SDK draws it.
  const n = ROUTES.length, a0 = 238, a1 = 302, span = (a1 - a0) / n;
  const top = a1 - ((s.sel || 0) + (s.shift || 0)) * span;
  arc(ctx, 120, 120, 112, 9, a0, a1, C.grayDark, true);
  arc(ctx, 120, 120, 112, 9, top - span, top, C.white, true);
  buttons(ctx, { r1: 'amber', r2: 'white' }, s.glow || {});
}

/** Fit the whole route into a box (like SDK::TrackMapBuilder's overview). */
export function overview(ctx, cx, cy, size, width = 3, p = 1, color = ROUTE_LINE) {
  let x0 = Infinity, x1 = -Infinity, y0 = Infinity, y1 = -Infinity;
  for (const [x, y] of ROUTE.pts) {
    x0 = Math.min(x0, x); x1 = Math.max(x1, x); y0 = Math.min(y0, y); y1 = Math.max(y1, y);
  }
  const k = size / Math.max(x1 - x0, y1 - y0);
  const mx = (x0 + x1) / 2, my = (y0 + y1) / 2;
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = width;
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';
  ctx.beginPath();
  const n = Math.max(2, Math.floor(ROUTE.pts.length * clamp(p)));
  for (let i = 0; i < n; i++) {
    const [x, y] = ROUTE.pts[i];
    const X = cx + (x - mx) * k, Y = cy - (y - my) * k;
    if (i) ctx.lineTo(X, Y);
    else ctx.moveTo(X, Y);
  }
  ctx.stroke();
  const [sx, sy] = ROUTE.pts[0];
  disc(ctx, cx + (sx - mx) * k, cy - (sy - my) * k, width + 1.5, START);
  ctx.restore();
}

/** The route's summary before starting. */
export function routeSummary(ctx, s = {}) {
  title(ctx, 'Ridge loop');
  overview(ctx, 120, 108, 108, 3, s.p ?? 1);
  label(ctx, F.Medium18, '14.2 km · 650 m', 0, 170, 240, 'center', C.white);
  label(ctx, F.Regular14, 'R1 to start', 0, 196, 240, 'center', C.gray);
  buttons(ctx, { r1: 'amber', r2: 'white' }, s.glow || {});
}

const ZOOMS = { '200 m': 200, '500 m': 500, '1 km': 1000, '2 km': 2000 };

/**
 * The map screen. s: { at (m along the route), radius (m shown across the
 * safe radius of 100 px), headingUp, heading (override, radians), you
 * ([x, y] world override when off the line), scale ('500 m'), banner
 * ('off'|'back'|null), bannerP (0..1), offBy (m), toGo (km) }
 */
export function mapScreen(ctx, s) {
  const here = routeAt(s.at);
  const you = s.you || [here.x, here.y];
  const heading = s.heading ?? here.heading;
  // Heading-up turns the world by the heading so the way you face is up.
  const th = s.headingUp ? heading : 0;
  const k = 100 / s.radius;
  const cth = Math.cos(th), sth = Math.sin(th);
  const toScr = (x, y) => {
    const dx = (x - you[0]) * k, dy = (y - you[1]) * k;
    return [120 + (dx * cth - dy * sth), 120 - (dx * sth + dy * cth)];
  };
  // The line: behind you in purple, ahead in orchid.
  ctx.save();
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';
  const done = routeSlice(0, s.at);
  const ahead = routeSlice(s.at, ROUTE.length);
  const stroke = (pts, col, w) => {
    ctx.strokeStyle = col;
    ctx.lineWidth = w;
    ctx.beginPath();
    pts.forEach(([x, y], i) => {
      const [X, Y] = toScr(x, y);
      if (i) ctx.lineTo(X, Y);
      else ctx.moveTo(X, Y);
    });
    ctx.stroke();
  };
  stroke(done, ROUTE_DONE, 4);
  stroke(ahead, ROUTE_LINE, 4);
  // Start / finish.
  const [sx, sy] = toScr(...ROUTE.pts[0]);
  ring(ctx, sx, sy, 7, 3, START);
  // A breadcrumb of where you've really been, when it differs from the line.
  if (s.crumbs && s.crumbs.length) {
    s.crumbs.forEach(([x, y]) => {
      const [X, Y] = toScr(x, y);
      disc(ctx, X, Y, 1.6, C.gray);
    });
  }
  ctx.restore();
  // You: a white arrow pointing the way you face.
  const youA = s.headingUp ? 0 : heading;
  ctx.save();
  ctx.translate(...toScr(...you));
  ctx.rotate(youA);
  ctx.fillStyle = C.white;
  ctx.strokeStyle = '#000';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(0, -11);
  ctx.lineTo(8, 8);
  ctx.lineTo(0, 4);
  ctx.lineTo(-8, 8);
  ctx.closePath();
  ctx.stroke();
  ctx.fill();
  ctx.restore();
  // North: an "N" on the edge of the disc.
  const nx = 120 - sth * 92, ny = 120 - cth * 92;
  ctx.save();
  ctx.fillStyle = '#000';
  ctx.beginPath();
  ctx.arc(nx, ny, 10, 0, TAU);
  ctx.fill();
  ctx.restore();
  label(ctx, F.SemiBold20, 'N', nx - 12, ny - 12, 24, 'center', C.red);
  // Distance to go at the top, the scale at the bottom.
  if (s.toGo != null) label(ctx, F.Medium18, `${s.toGo.toFixed(1)} km to go`, 0, 48, 240, 'center', C.white);
  const scaleM = s.scale || '500 m';
  const px = (ZOOMS[scaleM] || 500) * k;
  if (px < 170) {
    ctx.fillStyle = C.white;
    ctx.fillRect(120 - px / 2, 204, px, 3);
    ctx.fillRect(120 - px / 2, 199, 2, 8);
    ctx.fillRect(120 + px / 2 - 2, 199, 2, 8);
  }
  label(ctx, F.Regular14, scaleM, 0, 180, 240, 'center', C.gray);
  // The banner.
  if (s.banner) bannerBand(ctx, s.banner, s.bannerP ?? 1, s.offBy);
  buttons(ctx, { l1: 'white', l2: 'white', r1: 'white', r2: 'amber' }, s.glow || {});
}

/** The off-course / back-on-course banner: a band like the wheel's, clipped. */
export function bannerBand(ctx, kind, p = 1, offBy = 64) {
  const h = 70 * clamp(p);
  if (h <= 0) return;
  ctx.save();
  ctx.beginPath();
  ctx.arc(120, 120, 112, 0, TAU);
  ctx.clip();
  ctx.fillStyle = kind === 'off' ? C.amber : ROUTE_LINE;
  ctx.fillRect(0, 120 - h / 2, 240, h);
  ctx.beginPath();
  ctx.rect(0, 120 - h / 2, 240, h);
  ctx.clip();
  if (kind === 'off') {
    label(ctx, F.SemiBold25, 'Off course', 0, 90, 240, 'center', '#000');
    label(ctx, F.Medium18, `${Math.round(offBy)} m from the line`, 0, 120, 240, 'center', '#000');
  } else {
    label(ctx, F.SemiBold25, 'Back on course', 0, 104, 240, 'center', '#000');
  }
  ctx.restore();
}

/** A run data face, as RunLVGL's (time, distance, pace, heart rate). */
export function dataFace(ctx, s) {
  title(ctx, 'Run');
  label(ctx, F.SemiBold40, s.dist.toFixed(2), 0, 44, 240, 'center', C.white);
  label(ctx, F.Regular14, 'km', 0, 90, 240, 'center', C.gray);
  label(ctx, F.Medium25, s.pace, 30, 112, 90, 'center', C.white);
  label(ctx, F.Regular14, '/km', 30, 140, 90, 'center', C.gray);
  label(ctx, F.Medium25, s.time, 120, 112, 90, 'center', C.white);
  label(ctx, F.Regular14, 'time', 120, 140, 90, 'center', C.gray);
  label(ctx, F.Medium18, `${s.hr} bpm`, 0, 166, 240, 'center', C.white);
  label(ctx, F.Regular14, `Lap ${s.lap}`, 0, 190, 240, 'center', ROUTE_LINE);
  buttons(ctx, { l1: 'white', l2: 'white', r1: 'white', r2: 'amber' });
}

export { ROUTE, routeAt, routeSlice, routePoint };
