// Button presses and haptics, as pure functions of film time: the pressed button sinks and a ring
// pulses out from it; where the watch really buzzes (screen/script.ts BUZZES) the body shakes by
// 2-3 px, once per pulse of the buzz.

import * as THREE from 'three';
import { BRAND } from '../config';
import { PRESSES, BUZZES } from '../screen/script';
import type { WatchParts } from './model';
import type { Btn } from '../screen/lvgl';

const hash = (n: number) => {
  const x = Math.sin(n * 127.1 + 311.7) * 43758.5453;
  return x - Math.floor(x);
};

export class Feedback {
  private pool: THREE.Mesh[] = [];
  private mat: THREE.MeshBasicMaterial[] = [];

  constructor(scene: THREE.Scene) {
    const geo = new THREE.RingGeometry(0.9, 1.0, 64);
    for (let i = 0; i < 4; i++) {
      const m = new THREE.MeshBasicMaterial({ color: new THREE.Color(BRAND.yellow).multiplyScalar(2.4), transparent: true, depthWrite: false, depthTest: false, blending: THREE.AdditiveBlending });
      const mesh = new THREE.Mesh(geo, m);
      mesh.renderOrder = 10;
      mesh.visible = false;
      scene.add(mesh);
      this.pool.push(mesh);
      this.mat.push(m);
    }
  }

  /**
   * @param pxToWorld world units per output pixel at the watch's distance (for the shake).
   */
  update(t: number, w: WatchParts, camera: THREE.Camera, pxToWorld: number) {
    // Shake: jitter of up to 2.5 px for each pulse of a buzz.
    const frame = Math.round(t * 60);
    let amp = 0;
    for (const b of BUZZES) {
      for (let i = 0; i < b.pulses; i++) {
        const t0 = b.t + i * (b.len + b.gap);
        if (t >= t0 && t <= t0 + b.len) amp = Math.max(amp, b.amp * (b.len > 0.5 ? 0.6 : 1));
      }
    }
    const px = 2.5 * amp;
    w.body.position.set((hash(frame) - 0.5) * 2 * px * pxToWorld, (hash(frame + 7.3) - 0.5) * 2 * px * pxToWorld, 0);
    // Everything below (and the overlay's callouts) reads the watch's world matrices: bring them
    // up to this frame first, or they would carry the last frame's turn and shake.
    w.root.updateMatrixWorld(true);

    // Buttons: in 0.05 and back over 0.18 s.
    const depth: Record<Btn, number> = { l1: 0, l2: 0, r1: 0, r2: 0 };
    for (const p of PRESSES) {
      const k = (t - p.t) / 0.18;
      if (k >= 0 && k <= 1) depth[p.btn] = Math.max(depth[p.btn], 0.05 * (1 - Math.abs(k * 2 - 1)));
    }
    (Object.keys(depth) as Btn[]).forEach((b) => {
      const m = w.buttons[b];
      const d = m.userData.dir as THREE.Vector2;
      m.position.set(d.x * (1.5 - depth[b]), d.y * (1.5 - depth[b]), -0.3);
    });

    // Ring pulses, facing the camera, from the button's outer end.
    let n = 0;
    for (const p of PRESSES) {
      const k = (t - p.t) / 0.7;
      if (k < 0 || k > 1 || n >= this.pool.length) continue;
      const mesh = this.pool[n];
      const mat = this.mat[n];
      n++;
      const pos = w.buttonTip[p.btn].clone();
      w.body.localToWorld(pos);
      mesh.position.copy(pos);
      mesh.quaternion.copy(camera.quaternion);
      const e = 1 - Math.pow(1 - k, 3);
      mesh.scale.setScalar(0.08 + 0.5 * e);
      mat.opacity = (1 - k) * (1 - k);
      mesh.visible = true;
    }
    for (; n < this.pool.length; n++) this.pool[n].visible = false;

  }
}
