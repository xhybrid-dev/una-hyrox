// The apps' own LVGL fonts, drawn glyph for glyph as LVGL draws them (scripts/lvgl-fonts.mjs makes
// the JSON from the apps' generated .c files). Advance, placement, centring and wrapping follow
// una-sdk/ThirdParty/lvgl: lv_font_fmt_txt.c, lv_draw_label.c and lv_text.c.

export type FontKey =
  | 'italic_18' | 'italic_20' | 'light_60' | 'medium_18' | 'medium_25' | 'medium_40'
  | 'regular_14' | 'regular_16' | 'regular_18' | 'semibold_20' | 'semibold_25'
  | 'semibold_30' | 'semibold_35' | 'semibold_40' | 'semibold_60';

type Glyph = { adv: number; w: number; h: number; ox: number; oy: number; sx: number; sy: number };
type Font = { lh: number; bl: number; glyphs: Map<number, Glyph>; atlas: HTMLCanvasElement };

const fonts = new Map<FontKey, Font>();
const tinted = new Map<string, HTMLCanvasElement>();

function decode(b64: string, w: number, h: number): Uint8ClampedArray {
  const bin = atob(b64);
  const out = new Uint8ClampedArray(w * h);
  for (let i = 0; i < w * h; i++) {
    const bit = i * 2;
    const byte = bin.charCodeAt(bit >> 3);
    out[i] = ((byte >> (6 - (bit & 7))) & 3) * 85;
  }
  return out;
}

/** Loads every font in public/gen/fonts. */
export async function loadFonts(base: string): Promise<void> {
  const keys: FontKey[] = await (await fetch(`${base}fonts/index.json`)).json();
  await Promise.all(keys.map(async (key) => {
    const src: { lh: number; bl: number; glyphs: Record<string, [number, number, number, number, number, string]> } =
      await (await fetch(`${base}fonts/${key}.json`)).json();
    const entries = Object.entries(src.glyphs);
    // Pack the glyphs into rows of a white atlas whose alpha is the glyph's coverage.
    const W = 512;
    let x = 0, y = 0, rowH = 0;
    const placed: [number, Glyph, Uint8ClampedArray][] = [];
    for (const [cp, [adv, w, h, ox, oy, b64]] of entries) {
      if (x + w + 1 > W) { x = 0; y += rowH + 1; rowH = 0; }
      placed.push([Number(cp), { adv, w, h, ox, oy, sx: x, sy: y }, w && h ? decode(b64, w, h) : new Uint8ClampedArray(0)]);
      x += w + 1;
      rowH = Math.max(rowH, h);
    }
    const atlas = document.createElement('canvas');
    atlas.width = W;
    atlas.height = Math.max(1, y + rowH + 1);
    const actx = atlas.getContext('2d')!;
    const img = actx.createImageData(W, atlas.height);
    for (const [, g, a] of placed) {
      for (let j = 0; j < g.h; j++) {
        for (let i = 0; i < g.w; i++) {
          const o = ((g.sy + j) * W + g.sx + i) * 4;
          img.data[o] = img.data[o + 1] = img.data[o + 2] = 255;
          img.data[o + 3] = a[j * g.w + i];
        }
      }
    }
    actx.putImageData(img, 0, 0);
    fonts.set(key, { lh: src.lh, bl: src.bl, glyphs: new Map(placed.map(([cp, g]) => [cp, g])), atlas });
  }));
}

function font(key: FontKey): Font {
  const f = fonts.get(key);
  if (!f) throw new Error(`font ${key} not loaded`);
  return f;
}

function atlasIn(key: FontKey, color: string): HTMLCanvasElement {
  const id = `${key}|${color}`;
  let c = tinted.get(id);
  if (!c) {
    const f = font(key);
    c = document.createElement('canvas');
    c.width = f.atlas.width;
    c.height = f.atlas.height;
    const x = c.getContext('2d')!;
    x.fillStyle = color;
    x.fillRect(0, 0, c.width, c.height);
    x.globalCompositeOperation = 'destination-in';
    x.drawImage(f.atlas, 0, 0);
    tinted.set(id, c);
  }
  return c;
}

export const lineHeight = (key: FontKey) => font(key).lh;
/** The distance from a line's top to its baseline. */
export const ascent = (key: FontKey) => font(key).lh - font(key).bl;

/** lv_text_get_width: the sum of the advances (no letter spacing in these apps). */
export function textWidth(key: FontKey, str: string): number {
  const f = font(key);
  let w = 0;
  for (const ch of str) w += f.glyphs.get(ch.codePointAt(0)!)?.adv ?? 0;
  return w;
}

/** lv_text's word wrap at width w: breaks after a space (or a hyphen), never inside a word. */
export function wrap(key: FontKey, str: string, w: number): string[] {
  const out: string[] = [];
  for (const para of str.split('\n')) {
    const words = para.split(/(?<=[ -])/);
    let line = '';
    for (const word of words) {
      const next = line + word;
      if (line && textWidth(key, next.trimEnd()) > w) {
        out.push(line.trimEnd());
        line = word;
      } else line = next;
    }
    out.push(line.trimEnd());
  }
  return out;
}

export type Align = 'left' | 'center' | 'right';

/**
 * A label: its box's top-left at (x, y), width w, the text aligned per line inside it (the
 * apps' Theme::label). `wrapIt` wraps at w as LV_LABEL_LONG_WRAP; otherwise lines split on "\n"
 * only. Returns the box's height.
 */
export function drawText(
  ctx: CanvasRenderingContext2D, key: FontKey, str: string, x: number, y: number, w: number,
  align: Align = 'center', color = '#FFFFFF', wrapIt = false,
): number {
  const f = font(key);
  const lines = wrapIt ? wrap(key, str, w) : str.split('\n');
  const src = atlasIn(key, color);
  const top = f.lh - f.bl;
  lines.forEach((ln, i) => {
    const lw = textWidth(key, ln);
    let px = x;
    if (align === 'center') px = x + Math.trunc((w - lw) / 2);
    else if (align === 'right') px = x + w - lw;
    const ly = y + i * f.lh;
    for (const ch of ln) {
      const g = f.glyphs.get(ch.codePointAt(0)!);
      if (!g) continue;
      if (g.w && g.h) {
        ctx.drawImage(src, g.sx, g.sy, g.w, g.h, px + g.ox, ly + top - g.h - g.oy, g.w, g.h);
      }
      px += g.adv;
    }
  });
  return lines.length * f.lh;
}
