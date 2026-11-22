// UNA's watch, built procedurally and matched to UNA's front render (the SDK's Figma pack; see
// scripts/extract-una.py and PLAN.md section 2). Units: the display's radius is 1.
//
// Measured from the render: printed bezel 1.00-1.33, metal lip to about 1.48, case 1.47 round with
// lugs to 1.45 above and below (screws at +-0.86, +-1.33), strap 1.41 wide, buttons at the SDK's
// hint angles (L1 299.5, L2 240.5, R1 60.5, R2 120; TouchGFX degrees), USB-C at 3 o'clock.

import * as THREE from 'three';
import { RoundedBoxGeometry } from 'three/examples/jsm/geometries/RoundedBoxGeometry.js';
import { BTN_ANGLE, type Btn } from '../screen/lvgl';

const GRAPHITE = new THREE.Color('#1d1f23');
const METAL = new THREE.Color('#2e3136');

/** TouchGFX degrees (0 = 12 o'clock, clockwise) to a unit direction in the face's plane. */
export const dirOf = (deg: number) => {
  const a = ((90 - deg) * Math.PI) / 180;
  return new THREE.Vector2(Math.cos(a), Math.sin(a));
};

/** The case's front outline: a 1.47 circle joined with the lug block (|x| 1.03, |y| 1.45, r 0.22). */
function caseOutline(n = 256): THREE.Shape {
  const a = 1.03, b = 1.45, rc = 0.22, rCircle = 1.47;
  const sdf = (x: number, y: number) => {
    const qx = Math.abs(x) - (a - rc), qy = Math.abs(y) - (b - rc);
    return Math.hypot(Math.max(qx, 0), Math.max(qy, 0)) + Math.min(Math.max(qx, qy), 0) - rc;
  };
  const pts: THREE.Vector2[] = [];
  for (let i = 0; i < n; i++) {
    const th = (i / n) * Math.PI * 2;
    const dx = Math.cos(th), dy = Math.sin(th);
    let lo = 0, hi = 3;
    for (let k = 0; k < 40; k++) {
      const m = (lo + hi) / 2;
      if (sdf(dx * m, dy * m) < 0) lo = m; else hi = m;
    }
    const r = Math.max(rCircle, lo);
    pts.push(new THREE.Vector2(dx * r, dy * r));
  }
  return new THREE.Shape(pts);
}

/** The strap as a ribbon along a curve in the y-z plane: up from the lug, then looping back. */
function strapGeometry(sign: 1 | -1): THREE.BufferGeometry {
  const W = 1.41, T = 0.11, y0 = 1.3, y1 = 1.85, R = 1.25, sweep = (150 * Math.PI) / 180;
  const straight = y1 - y0, arcLen = R * sweep, L = straight + arcLen;
  const at = (s: number): [number, number, number, number] => {
    // position (y, z) and the outward normal (ny, nz) at arc length s
    if (s <= straight) return [y0 + s, -0.3, 0, 1];
    const th = (s - straight) / R;
    return [y1 + R * Math.sin(th), -0.3 - R + R * Math.cos(th), Math.sin(th), Math.cos(th)];
  };
  const N = 90;
  const pos: number[] = [], nor: number[] = [], uv: number[] = [], col: number[] = [], idx: number[] = [];
  const faces: [number, number][] = [[1, 1], [-1, -1]]; // outer (+n), inner (-n)
  const fade = (s: number) => 1 - THREE.MathUtils.smoothstep(s, L * 0.12, L * 0.6);
  // Outer and inner surfaces, with the strap's UV in strap units for the vents.
  for (const [side] of faces) {
    const base = pos.length / 3;
    for (let i = 0; i <= N; i++) {
      const s = (i / N) * L;
      const [y, z, ny, nz] = at(s);
      for (const u of [-1, 1]) {
        pos.push((u * W) / 2, sign * (y + (ny * T * side) / 2), z + (nz * T * side) / 2);
        nor.push(0, sign * ny * side, nz * side);
        uv.push(u * 0.5 + 0.5, s / W);
        col.push(1, 1, 1, fade(s));
      }
    }
    for (let i = 0; i < N; i++) {
      const a = base + i * 2, b = a + 1, c = a + 2, d = a + 3;
      // Counter-clockwise seen from outside (checked: (b - a) x (c - a) is the outward normal).
      if ((side > 0) === (sign > 0)) idx.push(a, b, c, b, d, c); else idx.push(a, c, b, b, c, d);
    }
  }
  // The two edges.
  for (const u of [-1, 1]) {
    const base = pos.length / 3;
    for (let i = 0; i <= N; i++) {
      const s = (i / N) * L;
      const [y, z, ny, nz] = at(s);
      for (const side of [-1, 1]) {
        pos.push((u * W) / 2, sign * (y + (ny * T * side) / 2), z + (nz * T * side) / 2);
        nor.push(u, 0, 0);
        uv.push(-1, -1);
        col.push(1, 1, 1, fade(s));
      }
    }
    for (let i = 0; i < N; i++) {
      const a = base + i * 2, b = a + 1, c = a + 2, d = a + 3;
      if ((u > 0) === (sign > 0)) idx.push(a, c, b, b, c, d); else idx.push(a, b, c, b, d, c);
    }
  }
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('normal', new THREE.Float32BufferAttribute(nor, 3));
  g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
  g.setAttribute('color', new THREE.Float32BufferAttribute(col, 4));
  g.setIndex(idx);
  return g;
}

/** Silicone strap material with the vents cut in the shader: rows of a slot between two holes. */
function strapMaterial(): THREE.MeshPhysicalMaterial {
  // The strap fades out towards its cut end (vertex alpha), as UNA's render does.
  const m = new THREE.MeshPhysicalMaterial({ color: '#0f1012', roughness: 0.8, metalness: 0, vertexColors: true, specularIntensity: 0.3, transparent: true });
  m.defines = { USE_UV: '' };
  m.onBeforeCompile = (s) => {
    s.fragmentShader = s.fragmentShader.replace('#include <clipping_planes_fragment>', `#include <clipping_planes_fragment>
      #ifdef USE_UV
      if (vUv.x >= 0.0) {
        float v = vUv.y;  // strap widths along
        float x = (vUv.x - 0.5) * 1.41;
        float row = floor((v - 0.42) / 0.27);
        float ly = (v - 0.42) - (row + 0.5) * 0.27;
        if (v > 0.42 && row < 6.0) {
          float slot = length(vec2(max(abs(x) - 0.13, 0.0), ly)) - 0.045;
          float holes = length(vec2(abs(x) - 0.43, ly)) - 0.05;
          if (min(slot, holes) < 0.0) discard;
        }
      }
      #endif`);
  };
  return m;
}

export type WatchParts = {
  root: THREE.Group; // moved by the film
  print: THREE.MeshBasicMaterial; // the bezel's printed ring
  body: THREE.Group; // shaken by the haptics
  buttons: Record<Btn, THREE.Mesh>;
  buttonTip: Record<Btn, THREE.Vector3>; // the outer end of each button, in body space
};

export function buildWatch(ring: THREE.Texture, screen: THREE.Mesh): WatchParts {
  const root = new THREE.Group();
  const body = new THREE.Group();
  root.add(body);

  // Case.
  const caseGeo = new THREE.ExtrudeGeometry(caseOutline(), { depth: 0.6, bevelEnabled: true, bevelThickness: 0.045, bevelSize: 0.04, bevelSegments: 4, curveSegments: 1 });
  caseGeo.translate(0, 0, -0.645);
  const caseMat = new THREE.MeshPhysicalMaterial({ color: GRAPHITE, roughness: 0.5, metalness: 0.35, clearcoat: 0.3, clearcoatRoughness: 0.35 });
  body.add(new THREE.Mesh(caseGeo, caseMat));

  // Caseback with a darker sensor window.
  const back = new THREE.Mesh(new THREE.CylinderGeometry(1.05, 1.12, 0.08, 96).rotateX(Math.PI / 2), caseMat);
  back.position.z = -0.72;
  body.add(back);
  const sensor = new THREE.Mesh(new THREE.CircleGeometry(0.45, 64), new THREE.MeshPhysicalMaterial({ color: '#0b0c0e', roughness: 0.1, clearcoat: 1 }));
  sensor.rotation.y = Math.PI;
  sensor.position.z = -0.765;
  body.add(sensor);

  // Bezel lip: a lathe from the print's edge to the case.
  const lip = new THREE.LatheGeometry([
    new THREE.Vector2(1.325, 0.06), new THREE.Vector2(1.35, 0.098), new THREE.Vector2(1.4, 0.108),
    new THREE.Vector2(1.45, 0.095), new THREE.Vector2(1.478, 0.055), new THREE.Vector2(1.482, 0.0),
  ], 160).rotateX(Math.PI / 2);
  body.add(new THREE.Mesh(lip, new THREE.MeshPhysicalMaterial({ color: METAL, roughness: 0.3, metalness: 0.9, clearcoat: 0.2 })));

  // The display and UNA's printed ring (its hole is the display's edge).
  screen.position.z = 0.03;
  body.add(screen);
  const ringInner = 1.0;
  const ringOuter = 1.331;
  // Printed ink under the glass: unlit, kept under the bloom threshold, dimmed with the light.
  const printMat = new THREE.MeshBasicMaterial({ map: ring, transparent: true, color: new THREE.Color(0.8, 0.8, 0.8) });
  const printed = new THREE.Mesh(new THREE.RingGeometry(ringInner * 0.999, ringOuter, 160, 1), printMat);
  printed.position.z = 0.036;
  body.add(printed);
  // A dark well under the print, between the print and the lip.
  const well = new THREE.Mesh(new THREE.RingGeometry(1.32, 1.36, 160, 1), new THREE.MeshStandardMaterial({ color: '#050505', roughness: 0.5 }));
  well.position.z = 0.034;
  body.add(well);

  // Glass: reflections only, added over the screen.
  const glass = new THREE.Mesh(new THREE.CircleGeometry(1.34, 128), new THREE.MeshPhysicalMaterial({
    color: 0x000000, roughness: 0.04, metalness: 0, clearcoat: 1, clearcoatRoughness: 0.03,
    transparent: true, blending: THREE.AdditiveBlending, depthWrite: false, envMapIntensity: 0.2,
  }));
  glass.position.z = 0.085;
  glass.renderOrder = 2;
  body.add(glass);

  // Screws on the lugs.
  const screwGeo = new THREE.CylinderGeometry(0.075, 0.075, 0.03, 32).rotateX(Math.PI / 2);
  const screwMat = new THREE.MeshPhysicalMaterial({ color: '#5d6168', roughness: 0.35, metalness: 0.95 });
  const slotGeo = new THREE.BoxGeometry(0.07, 0.014, 0.01);
  const slotMat = new THREE.MeshBasicMaterial({ color: '#111111' });
  for (const sx of [-1, 1]) for (const sy of [-1, 1]) {
    const s = new THREE.Mesh(screwGeo, screwMat);
    s.position.set(sx * 0.86, sy * 1.33, 0.012);
    body.add(s);
    for (const r of [0, Math.PI / 2]) {
      const sl = new THREE.Mesh(slotGeo, slotMat);
      sl.position.set(sx * 0.86, sy * 1.33, 0.028);
      sl.rotation.z = r + Math.PI / 4;
      body.add(sl);
    }
  }

  // Buttons: rounded pills on the case edge, long side along the edge.
  const btnGeo = new RoundedBoxGeometry(0.2, 0.36, 0.24, 4, 0.07);
  const btnMat = new THREE.MeshPhysicalMaterial({ color: '#33363c', roughness: 0.4, metalness: 0.6 });
  const buttons = {} as Record<Btn, THREE.Mesh>;
  const buttonTip = {} as Record<Btn, THREE.Vector3>;
  (Object.keys(BTN_ANGLE) as Btn[]).forEach((k) => {
    const d = dirOf(BTN_ANGLE[k]);
    const m = new THREE.Mesh(btnGeo, btnMat);
    m.position.set(d.x * 1.5, d.y * 1.5, -0.3);
    m.rotation.z = Math.atan2(d.y, d.x);
    m.userData.dir = d;
    buttons[k] = m;
    buttonTip[k] = new THREE.Vector3(d.x * 1.62, d.y * 1.62, -0.3);
    body.add(m);
  });

  // USB-C at 3 o'clock.
  const port = new THREE.Mesh(new RoundedBoxGeometry(0.05, 0.26, 0.1, 2, 0.02), new THREE.MeshPhysicalMaterial({ color: '#050506', roughness: 0.2, clearcoat: 1 }));
  port.position.set(1.49, 0, -0.3);
  body.add(port);

  // Straps.
  const sm = strapMaterial();
  body.add(new THREE.Mesh(strapGeometry(1), sm), new THREE.Mesh(strapGeometry(-1), sm));

  return { root, body, buttons, buttonTip, print: printMat };
}
