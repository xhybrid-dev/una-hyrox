// Race day: the race's sixteen segments as a ring round the watch, runs white and stations yellow.
// The open segment fills as its clock runs; each R2 press lifts the split off the screen as a panel
// that flies to its segment and lights it. At the finish the ring folds away into the race's tile.

import * as THREE from 'three';
import { RACE, RACE_BEATS, BRAND } from '../config';
import type { Frame } from '../app';
import { ease, span } from '../timeline';
import { makeText } from '../hud/type';
import type { Text } from 'troika-three-text';
import { basic, roundRectMesh, yellow, fade } from './util';
import { raceAt } from '../screen/script';
import { shortTime } from '../screen/race';

const N = 16;

export class RaceRing {
  readonly group = new THREE.Group();
  private ring: THREE.Mesh;
  private mat: THREE.ShaderMaterial;
  private panels: THREE.Group[] = [];
  private R: number;

  constructor(scene: THREE.Scene, segR: number) {
    this.R = segR;
    scene.add(this.group);
    this.mat = new THREE.ShaderMaterial({
      transparent: true, depthWrite: false, blending: THREE.AdditiveBlending,
      uniforms: {
        fills: { value: new Array(N).fill(0) },
        types: { value: RACE.segs.map((s) => (s.type === 'station' ? 1 : 0)) },
        cur: { value: -1 }, curF: { value: 0 }, opacity: { value: 0 },
        yellow: { value: new THREE.Color(BRAND.yellow) },
      },
      vertexShader: /* glsl */ `
        varying vec2 vP;
        void main() { vP = position.xy; gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0); }`,
      fragmentShader: /* glsl */ `
        uniform float fills[${N}]; uniform float types[${N}]; uniform float cur; uniform float curF;
        uniform float opacity; uniform vec3 yellow; varying vec2 vP;
        void main() {
          float a = degrees(atan(vP.y, vP.x));
          float c = mod(90.0 - a, 360.0);           // clockwise from 12 o'clock
          float k = floor(c / 22.5);
          float p = fract(c / 22.5);
          if (p < 0.05 || p > 0.95) discard;
          int i = int(k);
          float fill = 0.0; float ty = 0.0;
          for (int j = 0; j < ${N}; j++) { if (j == i) { fill = fills[j]; ty = types[j]; } }
          vec3 col = mix(vec3(1.25), yellow * 2.1, ty);
          vec3 base = vec3(0.16);
          vec3 c3 = base;
          if (fill > 0.0) c3 = mix(base, col, fill);
          else if (abs(k - cur) < 0.5 && (p - 0.05) / 0.9 < curF) c3 = mix(base, col * 0.42, 1.0);
          gl_FragColor = vec4(c3 * opacity, 1.0);
        }`,
    });
    this.ring = new THREE.Mesh(new THREE.RingGeometry(segR - 0.075, segR + 0.075, 256, 1), this.mat);
    this.ring.renderOrder = 4;
    this.group.add(this.ring);

    // A panel for each split: the segment's short name and its time.
    RACE.segs.forEach((s) => {
      const g = new THREE.Group();
      const bg = roundRectMesh(1.18, 0.3, 0.04, basic('#101010', 0.92));
      const bar = new THREE.Mesh(new THREE.PlaneGeometry(0.035, 0.3), basic(s.type === 'station' ? yellow(2.0) : new THREE.Color(1.4, 1.4, 1.4), 1, true));
      bar.position.set(-0.57, 0, 0.001);
      const name: Text = makeText({ text: s.short, size: 0.1, tracking: 0.12, color: '#ffffff', anchorY: 'middle' });
      name.position.set(-0.5, 0, 0.002);
      const time: Text = makeText({ text: shortTime(s.sec), size: 0.14, bold: true, color: s.type === 'station' ? BRAND.yellow : '#ffffff', anchorX: 'right', anchorY: 'middle' });
      time.position.set(0.53, 0, 0.002);
      g.add(bg, bar, name, time);
      bg.renderOrder = 30; bar.renderOrder = 31; name.renderOrder = time.renderOrder = 32;
      this.group.add(g);
      this.panels.push(g);
    });
  }

  /** World position of segment k's middle on the ring. */
  private segPoint(k: number): THREE.Vector3 {
    const a = ((90 - (k + 0.5) * 22.5) * Math.PI) / 180;
    return this.ring.localToWorld(new THREE.Vector3(this.R * Math.cos(a), this.R * Math.sin(a), 0));
  }

  /** The R2 press for segment k ends it. */
  private splitT(k: number) { return RACE_BEATS.open[k] + RACE_BEATS.segDur[k]; }

  update(f: Frame) {
    const { t, S, camera } = f;
    const a = S.race.a;
    this.group.visible = a > 0.01;
    if (!this.group.visible) return;
    // A slight tilt for depth; the ring folds in after the finish.
    const fold = span(t, RACE_BEATS.finish + 0.6, RACE_BEATS.finish + 1.6, 'power3.in');
    this.ring.rotation.set(-0.32, 0.22, 0);
    this.ring.scale.setScalar(1 - 0.75 * fold);
    this.ring.position.z = -0.2;
    this.ring.updateMatrixWorld(true);
    const r = raceAt(Math.min(t, RACE_BEATS.finish - 1e-3));
    const fills = this.mat.uniforms.fills.value as number[];
    for (let k = 0; k < N; k++) {
      const st = this.splitT(k);
      fills[k] = t < st ? 0 : 0.7 + 0.3 * Math.exp(-(t - st) * 4) + 0.6 * Math.exp(-(t - st) * 9);
    }
    this.mat.uniforms.cur.value = t >= RACE_BEATS.gun && r ? r.idx : -1;
    this.mat.uniforms.curF.value = r ? r.f : 0;
    this.mat.uniforms.opacity.value = a * (1 - fold);

    // Split panels: up off the screen, out to their segment, then gone.
    RACE.segs.forEach((_, k) => {
      const g = this.panels[k];
      const st = this.splitT(k);
      const life = k < 2 || k >= 13 ? 0.95 : 0.55;
      const x = (t - st) / life;
      if (x < 0 || x > 1 || k === N - 1) { g.visible = false; return; }
      g.visible = true;
      const from = new THREE.Vector3(0, 1.05, 0.45); // from the screen's top edge, clear of its text
      const to = this.segPoint(k).multiplyScalar(1.12);
      to.z += 0.35;
      const e = ease('power3.out')(Math.min(1, x / 0.6));
      g.position.copy(from.lerp(to, e));
      g.quaternion.copy(camera.quaternion);
      g.scale.setScalar(0.55 + 0.45 * ease('back.out(2)')(Math.min(1, x / 0.35)));
      fade(g, a * Math.min(1, x * 8) * (1 - span(t, st + life * 0.7, st + life, 'power2.in')));
    });
  }
}
