// The family: the seven app icons (their real store icons) lift out of the watch's screen one by
// one and settle into an orbit round it, each named beneath.

import * as THREE from 'three';
import { FAMILY, MONTAGE } from '../config';
import type { Frame } from '../app';
import { ease } from '../timeline';
import { makeText } from '../hud/type';
import type { Text } from 'troika-three-text';
import { basic, yellow } from './util';
import { montageApp } from '../screen/script';

export class Family {
  readonly group = new THREE.Group();
  readonly icons: THREE.Group[] = [];
  private names: Text[] = [];
  private halos: THREE.Mesh[] = [];
  private R: number;
  private r: number;
  /** The orbit's height over its width, and its rise beside the panel (layout.ts). */
  squash = 0.8;
  lift = 0.5;

  constructor(scene: THREE.Scene, textures: THREE.Texture[], ringR: number, iconR: number) {
    this.R = ringR;
    this.r = iconR;
    scene.add(this.group);
    const disc = new THREE.CircleGeometry(1, 96);
    const halo = new THREE.RingGeometry(1.12, 1.2, 96);
    FAMILY.forEach((f, i) => {
      const g = new THREE.Group();
      const icon = new THREE.Mesh(disc, new THREE.MeshBasicMaterial({ map: textures[i], transparent: true, depthWrite: false }));
      icon.renderOrder = 3;
      g.add(icon);
      const h = new THREE.Mesh(halo, basic(yellow(1.25), 0, true));
      h.renderOrder = 3;
      g.add(h);
      this.halos.push(h);
      const name = makeText({ text: f.name.toUpperCase(), size: 0.36, tracking: 0.12, anchorX: 'center', anchorY: 'top', color: '#ffffff' });
      name.position.y = -1.32;
      name.renderOrder = 3;
      (name.material as THREE.Material).depthWrite = false;
      g.add(name);
      this.names.push(name);
      g.scale.setScalar(iconR);
      this.group.add(g);
      this.icons.push(g);
    });
  }

  /** Where icon i sits on the orbit at time t (world space). */
  slot(i: number, t: number, spin: number, tight = 0): THREE.Vector3 {
    const n = FAMILY.length;
    const a = Math.PI / 2 - (i / n) * Math.PI * 2 - spin;
    const R = this.R * (1 - 0.22 * tight);
    const p = new THREE.Vector3(R * Math.cos(a), R * Math.sin(a) * this.squash + this.lift * tight, 0.45 + 0.25 * Math.sin(a + 0.6));
    p.applyAxisAngle(new THREE.Vector3(0, 1, 0), 0.18 * Math.sin(t * 0.21));
    return p;
  }

  update(f: Frame) {
    const { t, S, camera } = f;
    const active = montageApp(t);
    FAMILY.forEach((app, i) => {
      const g = this.icons[i];
      const t0 = MONTAGE.start + i * MONTAGE.step + 0.1;
      const k = t < t0 ? 0 : Math.min(1, (t - t0) / 0.85);
      const e = ease('power3.out')(k);
      const from = new THREE.Vector3(0, 0, 0.4);
      const to = this.slot(i, t, S.family.spin, S.family.tight);
      g.position.copy(from.lerp(to, e));
      const on = active === app.id ? 1 : 0;
      const pop = 1 + 0.22 * on * ease('power2.out')(Math.min(1, (t - t0) / 0.5));
      g.scale.setScalar(this.r * (0.2 + 0.8 * e) * pop * (1 - 0.2 * S.family.tight));
      g.quaternion.copy(camera.quaternion);
      g.visible = t >= t0 && S.family.a > 0.01;
      const a = S.family.a * Math.min(1, k * 3);
      (g.children[0] as THREE.Mesh<THREE.BufferGeometry, THREE.MeshBasicMaterial>).material.opacity = a;
      (this.halos[i].material as THREE.MeshBasicMaterial).opacity = a * on;
      this.names[i].fillOpacity = a * (0.55 + 0.45 * on) * Math.min(1, k * 1.5);
    });
  }

  /** A point just outside icon i, away from the watch: where its callout starts. */
  edgeOf(i: number): THREE.Vector3 {
    const p = this.icons[i].position.clone();
    const out = new THREE.Vector3(p.x, p.y, 0).normalize();
    return p.add(out.multiplyScalar(this.icons[i].scale.x * 1.25));
  }

  /** World position of an app's icon now. */
  positionOf(id: string): THREE.Vector3 {
    const i = FAMILY.findIndex((f) => f.id === id);
    return this.icons[i].position.clone();
  }
}
