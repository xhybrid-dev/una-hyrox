// The rest of the family on the screen: X Ride's and X Workout's Saved screen (their ResultScreen),
// and the apps' real simulator previews (Resources/previews, the store's), drawn into the same
// 240 px canvas and quantised with everything else.

import type { AppId } from '../config';
import { C, label, arc, type Ctx } from './lvgl';

/**
 * ResultScreen, saved (X Ride, X Workout): a chartreuse ring, "Saved", the note ("It will sync to
 * the UNA app"), replaced by the Streak line in amber when it arrives, and the load under it.
 */
export function ringSaved(ctx: Ctx, load: number, line: [string, string] | null) {
  arc(ctx, 120, 120, 113, 6, 0, 360, C.CHARTREUSE);
  label(ctx, 'semibold_30', 'Saved', 0, 62, 240, 'center', C.CHARTREUSE);
  if (line) label(ctx, 'medium_18', `${line[0]}\n${line[1]}`, 0, 118, 240, 'center', C.YELLOW_DARK);
  else label(ctx, 'medium_18', 'It will sync to\nthe UNA app', 0, 118, 240);
  if (line) label(ctx, 'regular_14', `Load ${load}`, 0, 174, 240);
}

const previews = new Map<AppId, HTMLImageElement>();

export async function loadPreviews(base: string, ids: AppId[]): Promise<void> {
  await Promise.all(ids.map((id) => new Promise<void>((ok, fail) => {
    const im = new Image();
    im.onload = () => { previews.set(id, im); ok(); };
    im.onerror = () => fail(new Error(`preview ${id}`));
    im.src = `${base}previews/${id}.png`;
  })));
}

/** A real simulator screen (480 px), scaled to the display's 240. */
export function preview(ctx: Ctx, id: AppId) {
  const im = previews.get(id);
  if (!im) return;
  ctx.save();
  ctx.imageSmoothingEnabled = true;
  ctx.imageSmoothingQuality = 'high';
  ctx.drawImage(im, 0, 0, 240, 240);
  ctx.restore();
}
