// Where things sit in the world, per aspect ratio. 16:9 puts the explaining panel (the week, the
// mountain, the load curve) to the right of the watch; 9:16 and 1:1 put it below, the watch in the
// upper half, so the captions keep the bottom of the frame (inside Instagram's safe area on 9:16).

import type { Ratio } from './config';

export type V3 = [number, number, number];
export type Shot = { pos: V3; look: V3; fov: number };

export type Layout = {
  aux: V3; // the panel's centre
  auxScale: number;
  floorY: number;
  ringR: number; // the family's orbit
  ringSquash: number; // its height over its width
  ringLift: number; // how far it rises once the panel is beside the watch
  iconR: number;
  segR: number; // race day's ring of 16
  shots: Record<'introA' | 'introB' | 'family' | 'aux' | 'race' | 'raceWide' | 'streak' | 'outro', Shot>;
  /** Captions: left edge, bottom edge (px from the top), size, max width (px). */
  caption: { x: number; bottom: number; size: number; maxW: number };
  /** The lockup's centre (px from the top) and height. */
  lockup: { y: number; h: number };
};

export const LAYOUTS: Record<Ratio, Layout> = {
  '16x9': {
    aux: [4.55, 0.05, 0.2], auxScale: 1, floorY: -3.9, ringR: 3.05, ringSquash: 0.8, ringLift: 0.5, iconR: 0.36, segR: 1.98,
    shots: {
      introA: { pos: [3.4, 1.1, 6.6], look: [0, 0.05, 0], fov: 26 },
      introB: { pos: [0, -0.35, 11.6], look: [0, -0.6, 0], fov: 28 },
      family: { pos: [0, 1.5, 12.8], look: [0, 0.1, 0], fov: 30 },
      aux: { pos: [2.35, 0.9, 11.8], look: [2.35, 0.05, 0], fov: 30 },
      race: { pos: [-0.6, 0.8, 9.8], look: [0, -0.05, 0], fov: 30 },
      raceWide: { pos: [2.35, 0.9, 11.8], look: [2.35, 0.05, 0], fov: 30 },
      streak: { pos: [2.3, 1.1, 11.6], look: [2.3, 0.3, 0], fov: 30 },
      outro: { pos: [0, 0.2, 13.4], look: [0, -0.85, 0], fov: 30 },
    },
    caption: { x: 96, bottom: 1080 - 92, size: 54, maxW: 1100 },
    lockup: { y: 1080 * 0.86, h: 58 },
  },
  '9x16': {
    aux: [0, -3.0, 0.7], auxScale: 0.98, floorY: -5.8, ringR: 2.35, ringSquash: 0.72, ringLift: 0.15, iconR: 0.34, segR: 1.95,
    shots: {
      introA: { pos: [2.6, 0.6, 9.8], look: [0, -0.9, 0], fov: 38 },
      introB: { pos: [0, 0.0, 12.0], look: [0, -1.1, 0], fov: 40 },
      family: { pos: [0, 0.6, 14.6], look: [0, -1.3, 0], fov: 40 },
      aux: { pos: [0, -0.3, 14.2], look: [0, -1.95, 0], fov: 40 },
      race: { pos: [-0.4, 0.2, 11.4], look: [0, -1.05, 0], fov: 40 },
      raceWide: { pos: [0, -0.3, 14.2], look: [0, -1.95, 0], fov: 40 },
      streak: { pos: [0, -0.1, 14.4], look: [0, -1.85, 0], fov: 40 },
      outro: { pos: [0, 0.4, 15.4], look: [0, -1.5, 0], fov: 40 },
    },
    caption: { x: 84, bottom: 1520, size: 62, maxW: 820 },
    lockup: { y: 1300, h: 70 },
  },
  '1x1': {
    aux: [0, -3.4, 0.7], auxScale: 0.9, floorY: -5.4, ringR: 2.75, ringSquash: 0.8, ringLift: 0.1, iconR: 0.34, segR: 2.05,
    shots: {
      introA: { pos: [2.9, 0.8, 8.4], look: [0, -0.5, 0], fov: 32 },
      introB: { pos: [0, 0.1, 11.0], look: [0, -0.7, 0], fov: 34 },
      family: { pos: [0, 1.0, 14.0], look: [0, -0.6, 0], fov: 34 },
      aux: { pos: [0, -0.1, 13.8], look: [0, -1.55, 0], fov: 34 },
      race: { pos: [-0.4, 0.4, 10.6], look: [0, -0.5, 0], fov: 34 },
      raceWide: { pos: [0, -0.1, 13.8], look: [0, -1.55, 0], fov: 34 },
      streak: { pos: [0, 0.1, 14.0], look: [0, -1.5, 0], fov: 34 },
      outro: { pos: [0, 0.8, 14.6], look: [0, -0.8, 0], fov: 34 },
    },
    caption: { x: 76, bottom: 1080 - 70, size: 50, maxW: 930 },
    lockup: { y: 1080 * 0.8, h: 58 },
  },
};
