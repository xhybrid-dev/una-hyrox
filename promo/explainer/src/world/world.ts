// Everything round the watch, updated from film time.

import * as THREE from 'three';
import type { Ratio } from '../config';
import { LAYOUTS } from '../layout';
import type { Frame } from '../app';
import { Family } from './family';
import { WeekStrip } from './week';
import { RaceRing } from './raceRing';
import { StreakWorld } from './streak';
import { textsReady } from '../hud/type';
import { rng } from './util';

/** A few slow motes of dust in the light: seeded, drifting with time. */
class Dust {
  private pts: THREE.Points;
  private base: Float32Array;
  constructor(scene: THREE.Scene) {
    const r = rng(7), n = 70;
    this.base = new Float32Array(n * 3);
    for (let i = 0; i < n; i++) {
      this.base[i * 3] = (r() - 0.5) * 16;
      this.base[i * 3 + 1] = (r() - 0.5) * 9;
      this.base[i * 3 + 2] = -2 - r() * 6;
    }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(this.base.slice(), 3));
    this.pts = new THREE.Points(g, new THREE.PointsMaterial({ color: 0xffffff, size: 0.025, transparent: true, opacity: 0.35, depthWrite: false }));
    scene.add(this.pts);
  }
  update(t: number, a: number) {
    const p = this.pts.geometry.attributes.position as THREE.BufferAttribute;
    for (let i = 0; i < p.count; i++) {
      p.setXYZ(i, this.base[i * 3] + Math.sin(t * 0.07 + i) * 0.3, this.base[i * 3 + 1] + ((t * 0.04 + i * 0.37) % 1) * 0.6, this.base[i * 3 + 2]);
    }
    p.needsUpdate = true;
    (this.pts.material as THREE.PointsMaterial).opacity = 0.3 * a;
  }
}

export class World {
  readonly family: Family;
  readonly week: WeekStrip;
  readonly race: RaceRing;
  readonly streak: StreakWorld;
  private dust: Dust;

  constructor(scene: THREE.Scene, ratio: Ratio, icons: THREE.Texture[]) {
    const L = LAYOUTS[ratio];
    this.family = new Family(scene, icons, L.ringR, L.iconR);
    this.family.squash = L.ringSquash;
    this.family.lift = L.ringLift;
    this.week = new WeekStrip(scene, icons);
    this.race = new RaceRing(scene, L.segR);
    this.streak = new StreakWorld(scene);
    this.dust = new Dust(scene);
  }

  ready() { return textsReady(); }

  update(f: Frame) {
    this.family.update(f);
    this.week.update(f, this.family);
    this.race.update(f);
    this.streak.update(f, this.family);
    this.dust.update(f.t, f.S.light.grid);
  }
}
