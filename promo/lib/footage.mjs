// Real footage for the reels: phone clips decoded by ffmpeg, graded on the
// way in, and handed to the canvas one frame at a time.
//
// A film built on footage keeps a plate (a canvas the size of the source
// frame). Before each frame is drawn, its prepare(t) asks for the source
// frame it needs: a moving shot reads forward through a decoder that stays
// open for that shot; a held frame is grabbed once, cached as a PNG in
// out/footage/, and reused. Nothing here is a pure function of time, but
// every frame still depends only on its time: a decoder that is asked to go
// backwards, or far ahead, is simply reopened at the new time.

import fs from 'node:fs';
import path from 'node:path';
import { spawn, spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { createCanvas, loadImage } from '@napi-rs/canvas';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const CACHE = path.join(HERE, '..', 'out', 'footage');

/** The same ffmpeg as render.mjs: $FFMPEG, else the one on the PATH. */
function ffmpegPath() {
  return process.env.FFMPEG || 'ffmpeg';
}

export const SRC_FPS = 30;

/**
 * The grade, applied to every frame as it's decoded: a little contrast and
 * clarity for a phone clip shot in daylight, nothing that shifts the
 * product's colours.
 */
export const GRADE = 'eq=contrast=1.07:saturation=1.05:gamma=0.97,unsharp=5:5:0.45:5:5:0';

/** Reads raw RGBA frames from one ffmpeg process, in order. */
class Decoder {
  constructor(file, t0, dur, w, h) {
    this.t0 = t0;
    this.size = w * h * 4;
    this.idx = -1;
    this.buf = null;
    this.chunks = [];
    this.have = 0;
    this.ended = false;
    this.waiter = null;
    const vf = `fps=${SRC_FPS},scale=${w}:${h}:flags=lanczos,${GRADE},format=rgba`;
    this.p = spawn(ffmpegPath(), ['-v', 'error', '-ss', t0.toFixed(3), '-i', file, '-t', dur.toFixed(3), '-an', '-sn', '-dn',
      '-vf', vf, '-f', 'rawvideo', '-pix_fmt', 'rgba', '-'], { stdio: ['ignore', 'pipe', 'pipe'] });
    // A decoder closed early complains about its broken pipe; only report
    // errors from one that ran to its end.
    this.err = '';
    this.closed = false;
    this.p.stderr.on('data', (c) => (this.err += c));
    // Closing a decoder mid-read can raise stream errors; they are expected.
    this.p.on('error', (e) => {
      if (!this.closed) throw e;
    });
    this.p.stdout.on('error', () => {});
    this.p.on('close', (code) => {
      if (code && !this.closed) process.stderr.write(`ffmpeg (decoding ${path.basename(file)} at ${t0}): ${this.err}\n`);
    });
    this.p.stdout.on('data', (c) => {
      this.chunks.push(c);
      this.have += c.length;
      // Keep no more than a few frames in memory.
      if (this.have > this.size * 3) this.p.stdout.pause();
      this.wake();
    });
    this.p.stdout.on('end', () => {
      this.ended = true;
      this.wake();
    });
  }

  wake() {
    const w = this.waiter;
    this.waiter = null;
    if (w) w();
  }

  async next() {
    while (this.have < this.size && !this.ended) {
      this.p.stdout.resume();
      await new Promise((r) => (this.waiter = r));
    }
    if (this.have < this.size) return null;
    const out = Buffer.allocUnsafe(this.size);
    let o = 0;
    while (o < this.size) {
      const c = this.chunks[0];
      const n = Math.min(c.length, this.size - o);
      c.copy(out, o, 0, n);
      o += n;
      if (n === c.length) this.chunks.shift();
      else this.chunks[0] = c.subarray(n);
    }
    this.have -= this.size;
    if (this.have < this.size * 2) this.p.stdout.resume();
    return out;
  }

  /** Frame k (0 is the frame at t0), reading forward; the last frame holds at the end. */
  async frame(k) {
    while (this.idx < k) {
      const b = await this.next();
      if (!b) break;
      this.buf = b;
      this.idx++;
    }
    return this.buf;
  }

  close() {
    this.closed = true;
    try {
      this.p.stdout.destroy();
      this.p.kill();
    } catch {
      // Already gone.
    }
  }
}

/**
 * A set of source clips, and a plate the film draws from. sources:
 * { key: absolute path }. w, h: the source frame after rotation.
 */
export class Footage {
  constructor(sources, w = 1080, h = 1920) {
    this.sources = sources;
    this.w = w;
    this.h = h;
    this.plate = createCanvas(w, h);
    this.pctx = this.plate.getContext('2d');
    this.image = this.pctx.createImageData(w, h);
    this.decoders = new Map();
    this.stills = new Map();
    this.shown = null;
  }

  /** Every source clip exists? (A film can fall back to a slate without them.) */
  available() {
    return Object.values(this.sources).every((f) => fs.existsSync(f));
  }

  /**
   * Put the moving frame at source time s of clip `src` on the plate. `key`
   * names the shot, so each shot keeps its own decoder; `until` is where the
   * shot ends in the source, so the decoder knows how far to read.
   */
  async moving(key, src, s, until) {
    let d = this.decoders.get(key);
    const k = d ? Math.round((s - d.t0) * SRC_FPS) : -1;
    if (!d || k < d.idx || k - d.idx > SRC_FPS * 2) {
      if (d) d.close();
      d = new Decoder(this.sources[src], Math.max(0, s), Math.max(0.2, until - s + 0.3), this.w, this.h);
      this.decoders.set(key, d);
    }
    const buf = await d.frame(Math.max(0, Math.round((s - d.t0) * SRC_FPS)));
    const id = `${key}:${d.t0}:${d.idx}`;
    if (buf && this.shown !== id) {
      this.image.data.set(buf);
      this.pctx.putImageData(this.image, 0, 0);
      this.shown = id;
    }
  }

  /** Close the decoders of every shot but `keep`. */
  retire(keep) {
    for (const [k, d] of this.decoders) {
      if (k !== keep) {
        d.close();
        this.decoders.delete(k);
      }
    }
  }

  stillPath(src, s) {
    return path.join(CACHE, `${src}_${s.toFixed(3)}.png`);
  }

  /** Grab (once) and cache the graded frame at source time s. */
  grab(src, s) {
    const f = this.stillPath(src, s);
    if (fs.existsSync(f)) return f;
    fs.mkdirSync(CACHE, { recursive: true });
    const vf = `scale=${this.w}:${this.h}:flags=lanczos,${GRADE}`;
    const r = spawnSync(ffmpegPath(), ['-v', 'error', '-y', '-ss', s.toFixed(3), '-i', this.sources[src], '-frames:v', '1',
      '-an', '-vf', vf, f + '.tmp.png']);
    if (r.status !== 0) throw new Error(`ffmpeg could not grab ${src} at ${s}: ${r.stderr}`);
    fs.renameSync(f + '.tmp.png', f);
    return f;
  }

  /** grab(), without blocking: for caching many frames at once. */
  grabAsync(src, s) {
    const f = this.stillPath(src, s);
    if (fs.existsSync(f)) return Promise.resolve(f);
    fs.mkdirSync(CACHE, { recursive: true });
    const vf = `scale=${this.w}:${this.h}:flags=lanczos,${GRADE}`;
    return new Promise((resolve, reject) => {
      const p = spawn(ffmpegPath(), ['-v', 'error', '-y', '-ss', s.toFixed(3), '-i', this.sources[src], '-frames:v', '1',
        '-an', '-vf', vf, f + '.tmp.png'], { stdio: ['ignore', 'ignore', 'inherit'] });
      p.on('close', (c) => {
        if (c !== 0) return reject(new Error(`ffmpeg could not grab ${src} at ${s}`));
        fs.renameSync(f + '.tmp.png', f);
        resolve(f);
      });
    });
  }

  /** Put the held frame at source time s of clip `src` on the plate. */
  async still(src, s) {
    const id = `still:${src}:${s.toFixed(3)}`;
    if (this.shown === id) return;
    let img = this.stills.get(id);
    if (!img) {
      img = await loadImage(fs.readFileSync(this.grab(src, s)));
      // Keep a handful: the rewind shows each only briefly.
      if (this.stills.size > 48) this.stills.delete(this.stills.keys().next().value);
      this.stills.set(id, img);
    }
    this.pctx.drawImage(img, 0, 0, this.w, this.h);
    this.shown = id;
  }

  close() {
    for (const d of this.decoders.values()) d.close();
    this.decoders.clear();
  }
}

/**
 * Where the plate goes on the frame: source point P lands on output point O
 * at scale z, then O is nudged so the plate always covers the frame.
 * Returns the transform as [z, dx, dy] (out = src * z + d).
 */
export function placement(W, H, w, h, { z = 1, px = w / 2, py = h / 2, ox = W / 2, oy = H / 2 }) {
  z = Math.max(z, W / w, H / h);
  let dx = ox - px * z;
  let dy = oy - py * z;
  dx = Math.min(0, Math.max(W - w * z, dx));
  dy = Math.min(0, Math.max(H - h * z, dy));
  return [z, dx, dy];
}
