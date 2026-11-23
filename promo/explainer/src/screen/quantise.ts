// The display's colours: 2 bits a channel, shown as 0, 85, 170 and 255. Each channel goes to its
// nearest level, as the screen shader does (watch/screen.ts) for the film.

export const LEVELS = [0, 85, 170, 255];

export const q = (v: number) => Math.round(v / 85) * 85;

/** Quantises a canvas region in place (the screen check; the film does it in the shader). */
export function quantise(ctx: CanvasRenderingContext2D, w: number, h: number) {
  const im = ctx.getImageData(0, 0, w, h);
  const d = im.data;
  for (let i = 0; i < d.length; i += 4) {
    d[i] = q(d[i]);
    d[i + 1] = q(d[i + 1]);
    d[i + 2] = q(d[i + 2]);
  }
  ctx.putImageData(im, 0, 0);
}
