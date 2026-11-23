// URL parameters: ?t=12.5 ?scene=streak ?ratio=16x9|9x16|1x1 ?clean=1 ?export=1 ?play=0

import { RATIOS, SCENES, type Ratio, type SceneId } from './config';

export type Params = { t: number; ratio: Ratio; clean: boolean; exporting: boolean; play: boolean };

export function readParams(search = location.search): Params {
  const q = new URLSearchParams(search);
  const ratio = (q.get('ratio') ?? '16x9') as Ratio;
  let t = Number(q.get('t') ?? 0);
  const scene = q.get('scene') as SceneId | null;
  if (scene) {
    const s = SCENES.find((x) => x.id === scene);
    if (s) t = s.start;
  }
  return {
    t: Number.isFinite(t) ? t : 0,
    ratio: ratio in RATIOS ? ratio : '16x9',
    clean: q.get('clean') === '1',
    exporting: q.get('export') === '1',
    play: q.get('play') !== '0' && q.get('export') !== '1',
  };
}
