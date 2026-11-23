// The apps' own LVGL fonts (Poppins, 2 bits per pixel, no kerning), converted from their
// generated .c files into JSON the screen renderer blits glyph for glyph. Same-named fonts from
// several apps are merged: their glyphs are the same Poppins at the same size, and each app's
// file adds the ranges it asked for (the middle dot, the ellipsis, a few letters at 40 and 60).
//
//   node scripts/lvgl-fonts.mjs      → public/gen/fonts/<face>_<size>.json
//
// Each glyph is [advance px, box w, box h, ofs x, ofs y, base64 of the packed 2 bpp bitmap].
// Advance rounding and placement follow una-sdk/ThirdParty/lvgl (lv_font_fmt_txt.c,
// lv_draw_label.c): adv = (adv_w + 8) >> 4; top = line top + (line_height - base_line) - box_h - ofs_y.

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, '..', '..', '..');
const OUT = path.join(HERE, '..', 'public', 'gen', 'fonts');

// The apps whose screens the film draws.
const APPS = ['hybridx-race', 'hybridx-streak', 'hybridx-ride', 'hybridx-workout', 'hybridx-run'];

function parse(file) {
  const src = fs.readFileSync(file, 'utf8');
  const bmpSrc = src.slice(src.indexOf('glyph_bitmap[] = {'), src.indexOf('};', src.indexOf('glyph_bitmap[] = {')));
  const bytes = Uint8Array.from([...bmpSrc.matchAll(/0x([0-9a-fA-F]{1,2})\b/g)].map((m) => parseInt(m[1], 16)));
  const dsc = [...src.matchAll(
    /\{\.bitmap_index = (\d+), \.adv_w = (\d+), \.box_w = (\d+), \.box_h = (\d+), \.ofs_x = (-?\d+), \.ofs_y = (-?\d+)\}/g,
  )].map((m) => m.slice(1).map(Number));
  const lists = {};
  for (const m of src.matchAll(/static const uint16_t (unicode_list_\d+)\[\] = \{([^}]*)\}/g)) {
    lists[m[1]] = [...m[2].matchAll(/0x([0-9a-fA-F]+)/g)].map((x) => parseInt(x[1], 16));
  }
  for (const m of src.matchAll(/static const uint8_t (glyph_id_ofs_list_\d+)\[\] = \{([^}]*)\}/g)) {
    lists[m[1]] = m[2].split(',').map((x) => x.trim()).filter(Boolean).map(Number);
  }
  const map = new Map(); // codepoint -> glyph id
  for (const m of src.matchAll(
    /\.range_start = (\d+), \.range_length = (\d+), \.glyph_id_start = (\d+),\s*\.unicode_list = (\w+), \.glyph_id_ofs_list = (\w+), \.list_length = (\d+), \.type = (\w+)/g,
  )) {
    const [start, len, gid] = [Number(m[1]), Number(m[2]), Number(m[3])];
    const type = m[7];
    if (type.endsWith('FORMAT0_TINY')) {
      for (let i = 0; i < len; i++) map.set(start + i, gid + i);
    } else if (type.endsWith('FORMAT0_FULL')) {
      // An offset per codepoint; lv_font_conv writes 0 for the ones it left out, which would
      // draw as the first glyph, so only the first and the non-zero ones are real.
      lists[m[5]].forEach((ofs, i) => {
        if (i === 0 || ofs !== 0) map.set(start + i, gid + ofs);
      });
    } else if (type.endsWith('SPARSE_TINY')) {
      lists[m[4]].forEach((ofs, i) => map.set(start + ofs, gid + i));
    } else {
      throw new Error(`${file}: cmap type ${type} not handled`);
    }
  }
  const lh = Number(src.match(/\.line_height = (\d+)/)[1]);
  const bl = Number(src.match(/\.base_line = (\d+)/)[1]);
  const bpp = Number(src.match(/\.bpp = (\d+)/)[1]);
  if (bpp !== 2) throw new Error(`${file}: bpp ${bpp}`);
  const glyphs = {};
  for (const [cp, gid] of map) {
    const [bi, advW, bw, bh, ox, oy] = dsc[gid];
    const n = Math.ceil((bw * bh * 2) / 8);
    glyphs[cp] = [(advW + 8) >> 4, bw, bh, ox, oy, Buffer.from(bytes.slice(bi, bi + n)).toString('base64')];
  }
  return { lh, bl, glyphs };
}

fs.mkdirSync(OUT, { recursive: true });
const fonts = {};
for (const app of APPS) {
  const dir = path.join(ROOT, app, 'Software', 'Apps', 'LVGL-GUI', 'assets', 'fonts');
  if (!fs.existsSync(dir)) continue;
  for (const f of fs.readdirSync(dir).filter((x) => /^poppins_\w+_\d+\.c$/.test(x))) {
    const key = f.replace(/^poppins_/, '').replace(/\.c$/, '');
    const p = parse(path.join(dir, f));
    if (!fonts[key]) fonts[key] = p;
    else {
      if (fonts[key].lh !== p.lh || fonts[key].bl !== p.bl) {
        throw new Error(`${key}: metrics differ between apps`);
      }
      for (const [cp, g] of Object.entries(p.glyphs)) if (!fonts[key].glyphs[cp]) fonts[key].glyphs[cp] = g;
    }
  }
}
const index = {};
for (const [key, f] of Object.entries(fonts)) {
  fs.writeFileSync(path.join(OUT, `${key}.json`), JSON.stringify(f));
  index[key] = Object.keys(f.glyphs).length;
}
fs.writeFileSync(path.join(OUT, 'index.json'), JSON.stringify(Object.keys(fonts).sort()));
console.log('lvgl-fonts:', JSON.stringify(index));
