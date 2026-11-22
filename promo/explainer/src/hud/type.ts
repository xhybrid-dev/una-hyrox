// The overlay's type: Space Grotesk 500 and 700 (the brand's headline face, SIL OFL), set with
// troika-three-text so every word is drawn in the canvas and so in the export. Every text is made
// once, then only moved and faded, so nothing re-lays out during a render.

import { Text, preloadFont } from 'troika-three-text';
import * as THREE from 'three';

export const FACE = { medium: '', bold: '' };
const pending: Promise<void>[] = [];

export async function loadType(base: string) {
  FACE.medium = `${base}type/space-grotesk-500.woff`;
  FACE.bold = `${base}type/space-grotesk-700.woff`;
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,:;!?·\'’-+×/';
  await Promise.all([FACE.medium, FACE.bold].map((font) => new Promise<void>((ok) => preloadFont({ font, characters: chars }, ok))));
}

export type TextOpts = {
  text: string;
  size: number;
  bold?: boolean;
  tracking?: number; // em; the brand's labels use 0.12
  color?: THREE.ColorRepresentation;
  anchorX?: Text['anchorX'];
  anchorY?: Text['anchorY'];
  align?: Text['textAlign'];
  maxWidth?: number;
  lineHeight?: number;
};

/** Makes a text and queues its layout; await textsReady() before the first frame. */
export function makeText(o: TextOpts): Text {
  const t = new Text();
  // Text never writes depth: a faded-out label must not cut holes in what is behind it.
  t.material = new THREE.MeshBasicMaterial({ transparent: true, depthWrite: false });
  t.text = o.text;
  t.font = o.bold ? FACE.bold : FACE.medium;
  t.fontSize = o.size;
  t.letterSpacing = o.tracking ?? 0;
  t.color = new THREE.Color(o.color ?? '#ffffff');
  t.anchorX = o.anchorX ?? 'left';
  t.anchorY = o.anchorY ?? 'bottom-baseline';
  t.textAlign = o.align ?? 'left';
  if (o.maxWidth) t.maxWidth = o.maxWidth;
  t.lineHeight = o.lineHeight ?? 1.1;
  pending.push(new Promise<void>((ok) => t.sync(ok)));
  return t;
}

export async function textsReady() {
  while (pending.length) await pending.shift();
}

/** The laid-out width of a text (after textsReady). */
export function widthOf(t: Text): number {
  const b = t.textRenderInfo?.blockBounds;
  return b ? b[2] - b[0] : 0;
}
