// Small pieces the world is built from: brand colours (HDR where they should bloom), line-art
// outlines, panels, and helpers for facing the camera.

import * as THREE from 'three';
import { BRAND } from '../config';

/** Yellow at an intensity: above about 1.1 it blooms. */
export const yellow = (k = 1) => new THREE.Color(BRAND.yellow).multiplyScalar(k);
export const white = (k = 1) => new THREE.Color('#ffffff').multiplyScalar(k);

export function lineMat(color: THREE.Color, opacity = 1, additive = true) {
  return new THREE.LineBasicMaterial({ color, transparent: true, opacity, depthWrite: false, blending: additive ? THREE.AdditiveBlending : THREE.NormalBlending });
}

export function basic(color: THREE.Color | string, opacity = 1, additive = false) {
  return new THREE.MeshBasicMaterial({ color, transparent: true, opacity, depthWrite: false, blending: additive ? THREE.AdditiveBlending : THREE.NormalBlending });
}

/** A rounded rectangle's outline, centred, as a line loop. */
export function roundRectLine(w: number, h: number, r: number, mat: THREE.LineBasicMaterial, seg = 6): THREE.LineLoop {
  const pts: THREE.Vector3[] = [];
  const c = [[w / 2 - r, h / 2 - r, 0], [-w / 2 + r, h / 2 - r, 1], [-w / 2 + r, -h / 2 + r, 2], [w / 2 - r, -h / 2 + r, 3]] as const;
  for (const [cx, cy, q] of c) {
    for (let i = 0; i <= seg; i++) {
      const a = (q * Math.PI) / 2 + (i / seg) * (Math.PI / 2);
      pts.push(new THREE.Vector3(cx + r * Math.cos(a), cy + r * Math.sin(a), 0));
    }
  }
  return new THREE.LineLoop(new THREE.BufferGeometry().setFromPoints(pts), mat);
}

/** A rounded-rectangle fill. */
export function roundRectMesh(w: number, h: number, r: number, mat: THREE.Material): THREE.Mesh {
  const s = new THREE.Shape();
  s.moveTo(-w / 2 + r, -h / 2);
  s.lineTo(w / 2 - r, -h / 2);
  s.quadraticCurveTo(w / 2, -h / 2, w / 2, -h / 2 + r);
  s.lineTo(w / 2, h / 2 - r);
  s.quadraticCurveTo(w / 2, h / 2, w / 2 - r, h / 2);
  s.lineTo(-w / 2 + r, h / 2);
  s.quadraticCurveTo(-w / 2, h / 2, -w / 2, h / 2 - r);
  s.lineTo(-w / 2, -h / 2 + r);
  s.quadraticCurveTo(-w / 2, -h / 2, -w / 2 + r, -h / 2);
  return new THREE.Mesh(new THREE.ShapeGeometry(s, 4), mat);
}

/** Sets the opacity of every material under an object (keeping each one's own base, userData.o). */
export function fade(obj: THREE.Object3D, a: number) {
  obj.visible = a > 0.002;
  if (!obj.visible) return;
  obj.traverse((o) => {
    const m = (o as THREE.Mesh).material as (THREE.Material & { opacity: number; fillOpacity?: number }) | undefined;
    if (!m) return;
    if (o.userData.o === undefined) o.userData.o = (o as unknown as { fillOpacity?: number }).fillOpacity ?? m.opacity;
    if ('fillOpacity' in o) (o as unknown as { fillOpacity: number }).fillOpacity = o.userData.o * a;
    else m.opacity = o.userData.o * a;
  });
}

/** A seeded random stream (mulberry32). */
export function rng(seed: number) {
  let a = seed >>> 0;
  return () => {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

/** Quadratic Bezier between a and b, lifted towards the camera by `lift` at the middle. */
export function arcPath(a: THREE.Vector3, b: THREE.Vector3, k: number, lift = 0.8): THREE.Vector3 {
  const mid = a.clone().add(b).multiplyScalar(0.5);
  mid.z += lift;
  mid.y += lift * 0.4;
  const p = a.clone().multiplyScalar((1 - k) * (1 - k));
  p.add(mid.multiplyScalar(2 * k * (1 - k)));
  p.add(b.clone().multiplyScalar(k * k));
  return p;
}
