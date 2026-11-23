// The strap, modelled from UNA's product photos (UNA's CAD has none): two-layer silicone round the
// wrist from lug to lug, with a buckle and a keeper at the back. Units: the display's radius is 1.

import * as THREE from 'three';
import { RoundedBoxGeometry } from 'three/examples/jsm/geometries/RoundedBoxGeometry.js';

/** The strap's centre line, in (y, z): from the top lug round the wrist to the bottom lug. */
const STRAP_PATH: [number, number][] = [
  [1.4, -0.46], [1.72, -0.5], [2.02, -0.64], [2.27, -0.95], [2.38, -1.42], [2.32, -1.92], [2.02, -2.36],
  [1.45, -2.68], [0.62, -2.84], [-0.3, -2.84], [-1.15, -2.7], [-1.8, -2.38], [-2.2, -1.92], [-2.36, -1.4],
  [-2.26, -0.95], [-2.02, -0.64], [-1.72, -0.5], [-1.4, -0.46],
];
const STRAP_W = 1.41;
const STRAP_T = 0.16;

/** A ribbon along the path, with outer and inner faces and two edges. Vertex colour r: 1 outer, 0 inner, 0.5 edge. */
function strapGeometry() {
  const curve = new THREE.CatmullRomCurve3(STRAP_PATH.map(([y, z]) => new THREE.Vector3(0, y, z)), false, 'centripetal');
  const N = 260;
  const L = curve.getLength();
  const P = curve.getSpacedPoints(N);
  const pos: number[] = [], nor: number[] = [], uv: number[] = [], col: number[] = [], idx: number[] = [];
  const frames = P.map((p, i) => {
    const t = curve.getTangentAt(i / N);
    // The outward normal in the y-z plane: the tangent turned a quarter, away from the wrist's centre.
    let n = new THREE.Vector3(0, -t.z, t.y);
    const c = new THREE.Vector3(0, 0, -1.65);
    if (n.dot(p.clone().sub(c)) < 0) n.negate();
    return { p, n: n.normalize() };
  });
  const ring = (side: number, kind: number) => {
    const base = pos.length / 3;
    frames.forEach(({ p, n }, i) => {
      for (const u of [-1, 1]) {
        pos.push((u * STRAP_W) / 2, p.y + (n.y * STRAP_T * side) / 2, p.z + (n.z * STRAP_T * side) / 2);
        nor.push(0, n.y * side, n.z * side);
        uv.push(u * 0.5, (i / N) * L);
        col.push(kind, 0, 0);
      }
    });
    for (let i = 0; i < N; i++) {
      const a = base + i * 2, b = a + 1, c = a + 2, d = a + 3;
      if (side > 0) idx.push(a, c, b, b, c, d); else idx.push(a, b, c, b, d, c);
    }
  };
  ring(1, 1);
  ring(-1, 0);
  for (const u of [-1, 1]) {
    const base = pos.length / 3;
    frames.forEach(({ p, n }, i) => {
      for (const side of [-1, 1]) {
        pos.push((u * STRAP_W) / 2, p.y + (n.y * STRAP_T * side) / 2, p.z + (n.z * STRAP_T * side) / 2);
        nor.push(u, 0, 0);
        uv.push(u * 0.5, (i / N) * L);
        col.push(0.5, 0, 0);
      }
    });
    for (let i = 0; i < N; i++) {
      const a = base + i * 2, b = a + 1, c = a + 2, d = a + 3;
      if (u > 0) idx.push(a, b, c, b, d, c); else idx.push(a, c, b, b, c, d);
    }
  }
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('normal', new THREE.Float32BufferAttribute(nor, 3));
  g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
  g.setAttribute('color', new THREE.Float32BufferAttribute(col, 3));
  g.setIndex(idx);
  return { geometry: g, curve, length: L };
}

/**
 * Silicone strap, two layers: the outside in the colourway, a darker insert inside. Rows of a slot (the
 * insert shows through) between two round holes (right through), as UNA's render and photos show.
 */
function strapMaterial(c: { strapOut: string; strapIn: string }, length: number) {
  const m = new THREE.MeshPhysicalMaterial({ color: 0xffffff, roughness: 0.9, specularIntensity: 0.12 });
  m.defines = { USE_UV: '' };
  m.onBeforeCompile = (s) => {
    s.uniforms.cOut = { value: new THREE.Color(c.strapOut) };
    s.uniforms.cIn = { value: new THREE.Color(c.strapIn) };
    s.uniforms.len = { value: length };
    s.vertexShader = s.vertexShader
      .replace('#include <common>', '#include <common>\nattribute vec3 color;\nvarying float vKind;')
      .replace('#include <begin_vertex>', '#include <begin_vertex>\nvKind = color.r;');
    s.fragmentShader = s.fragmentShader
      .replace('#include <common>', '#include <common>\nuniform vec3 cOut;\nuniform vec3 cIn;\nuniform float len;\nvarying float vKind;')
      .replace('#include <color_fragment>', `#include <color_fragment>
        {
          float x = vUv.x * ${STRAP_W.toFixed(2)};
          float v = vUv.y;
          // vents from 0.3 in from each lug, a row every 0.29
          float vv = min(v, len - v) - 0.3;
          float row = floor(vv / 0.29);
          float ly = vv - (row + 0.5) * 0.29;
          bool vents = vv > 0.0 && abs(v - len * 0.5) > 0.5;
          vec3 col = vKind > 0.75 ? cOut : (vKind < 0.25 ? mix(cOut * 0.82, cIn, step(abs(x), 0.4)) : cOut * 0.9);
          if (vents) {
            float hole = length(vec2(abs(x) - 0.46, ly)) - 0.062;
            if (hole < 0.0 && abs(vKind - 0.5) > 0.1) discard;
            float slot = length(vec2(max(abs(x) - 0.13, 0.0), ly)) - 0.046;
            if (slot < 0.0 && vKind > 0.75) col = cIn * 0.8;
          }
          diffuseColor.rgb = col;
        }`);
  };
  m.vertexColors = false;
  return m;
}


export function strap(strapOut: string, strapIn: string): THREE.Group {
  const g = new THREE.Group();
  const strap = strapGeometry();
  g.add(new THREE.Mesh(strap.geometry, strapMaterial({ strapOut, strapIn }, strap.length)));
  const tB = 0.73;
  const pB = strap.curve.getPointAt(tB);
  const tanB = strap.curve.getTangentAt(tB);
  const nB = new THREE.Vector3(0, -tanB.z, tanB.y);
  if (nB.dot(pB.clone().sub(new THREE.Vector3(0, 0, -1.65))) < 0) nB.negate();
  const buckle = new THREE.Group();
  const metal = new THREE.MeshPhysicalMaterial({ color: '#1c1d20', roughness: 0.3, metalness: 0.8, clearcoat: 0.4 });
  const bar = (w: number, h: number, x: number, y: number) => {
    const m = new THREE.Mesh(new RoundedBoxGeometry(w, h, 0.07, 2, 0.025), metal);
    m.position.set(x, y, 0);
    buckle.add(m);
  };
  bar(1.62, 0.08, 0, 0.24); bar(1.62, 0.08, 0, -0.24); bar(0.08, 0.56, 0.79, 0); bar(0.08, 0.56, -0.79, 0); bar(0.06, 0.5, 0, 0);
  const keeper = new THREE.Mesh(new RoundedBoxGeometry(STRAP_W + 0.08, 0.2, STRAP_T + 0.1, 3, 0.05), new THREE.MeshPhysicalMaterial({ color: strapOut, roughness: 0.7 }));
  keeper.position.set(0, 0.6, -0.03);
  buckle.add(keeper);
  buckle.quaternion.setFromRotationMatrix(new THREE.Matrix4().makeBasis(new THREE.Vector3(1, 0, 0), tanB.clone().normalize(), nB.clone().normalize()));
  buckle.position.copy(pB).addScaledVector(nB, STRAP_T / 2 + 0.03);
  g.add(buckle);
  return g;
}
