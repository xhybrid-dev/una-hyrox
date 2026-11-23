// UNA's watch, from UNA's own CAD and UNA's own render.
//
// - The housing: the bezel, the housing, the four buttons, the rear cover and the sensor window are
//   UNA's CAD (github.com/UNAWatch/una-hardware, CC BY 4.0), converted by scripts/una-cad.mjs.
// - The face: every surface that faces the front shows UNA's front render (the SDK's Figma pack),
//   projected straight on, so from the front the watch is UNA's art; the rest is lit.
// - Not in the CAD: the lens (a disc carrying the render's printed ring, the live screen under it),
//   the USB-C cover (a rubber block in the housing's port) and the strap (strap.ts, from UNA's photos).
//
// Units: the display's radius is 1 (15.35 mm); the bezel's top at z = 0, the back towards -z.

import * as THREE from 'three';
import { RoundedBoxGeometry } from 'three/examples/jsm/geometries/RoundedBoxGeometry.js';
import type { Btn } from '../screen/lvgl';
import { strap } from './strap';

export const MM = 15.35; // mm per display radius: UNA's CAD fitted to UNA's render
const mm = (v: number) => v / MM;

export type Colourway = 'graphite' | 'white';
export type WatchMeta = {
  watch: { cx: number; cy: number; r: number; w: number; h: number }; // the screen opening in the render (px)
  buttons: Record<Btn, { deg: number; r: number }>; // TouchGFX degrees, display radii
};

const COLOURS: Record<Colourway, { housing: string; bezel: string; strapOut: string; strapIn: string; back: string; rubber: string }> = {
  graphite: { housing: '#232528', bezel: '#121315', strapOut: '#1a1b1e', strapIn: '#101113', back: '#18191b', rubber: '#141517' },
  white: { housing: '#dcdcda', bezel: '#141517', strapOut: '#dfe0de', strapIn: '#333437', back: '#202124', rubber: '#1b1c1f' },
};

/** TouchGFX degrees (0 = 12 o'clock, clockwise) to a unit direction in the face's plane. */
export const dirOf = (deg: number) => {
  const a = ((90 - deg) * Math.PI) / 180;
  return new THREE.Vector2(Math.cos(a), Math.sin(a));
};

/**
 * A lit material whose front-facing surfaces show UNA's render, projected along z, where the render
 * is opaque. `level` scales the render with the room's light (1: its own colours).
 */
function projected(base: THREE.MeshPhysicalMaterial, render: THREE.Texture, m: WatchMeta['watch'], level: { value: number }, frontLo = 0.6) {
  base.onBeforeCompile = (s) => {
    s.uniforms.projMap = { value: render };
    s.uniforms.projXf = { value: new THREE.Vector4(m.r / m.w, m.r / m.h, m.cx / m.w, (m.h - m.cy) / m.h) };
    s.uniforms.projLevel = level;
    s.vertexShader = s.vertexShader
      .replace('#include <common>', '#include <common>\nvarying vec3 vOP;\nvarying vec3 vON;')
      .replace('#include <begin_vertex>', '#include <begin_vertex>\nvOP = position;\nvON = normal;');
    s.fragmentShader = s.fragmentShader
      .replace('#include <common>', '#include <common>\nuniform sampler2D projMap;\nuniform vec4 projXf;\nuniform float projLevel;\nvarying vec3 vOP;\nvarying vec3 vON;')
      .replace('#include <opaque_fragment>', `#include <opaque_fragment>
        {
          vec2 puv = vec2(vOP.x * projXf.x + projXf.z, vOP.y * projXf.y + projXf.w);
          vec4 pc = texture2D(projMap, puv);
          float front = smoothstep(${frontLo.toFixed(2)}, ${(frontLo + 0.3).toFixed(2)}, normalize(vON).z) * smoothstep(0.35, 0.8, pc.a);
          gl_FragColor.rgb = mix(gl_FragColor.rgb, pc.rgb * projLevel, front);
        }`);
  };
  base.customProgramCacheKey = () => `una-projected-${frontLo}`;
  return base;
}

export type WatchParts = {
  root: THREE.Group; // moved by the film
  body: THREE.Group; // shaken by the haptics
  level: { value: number }; // the render's light
  buttons: Record<Btn, THREE.Object3D[]>; // each button's cap and stem, pressed in by the film
  buttonDir: Record<Btn, THREE.Vector3>; // outward, in body space
  buttonTip: Record<Btn, THREE.Vector3>; // the outer face of each button, in body space
};

export function buildWatch(cad: THREE.Object3D, render: THREE.Texture, meta: WatchMeta, screen: THREE.Mesh, colourway: Colourway, wordmark: THREE.Texture): WatchParts {
  const c = COLOURS[colourway];
  const root = new THREE.Group();
  const body = new THREE.Group();
  root.add(body);
  const level = { value: 1 };
  render.anisotropy = 8;
  const m = meta.watch;

  const mats = {
    bezel: projected(new THREE.MeshPhysicalMaterial({ color: c.bezel, roughness: 0.42, metalness: 0.1, clearcoat: 0.3, clearcoatRoughness: 0.35 }), render, m, level),
    housing: projected(new THREE.MeshPhysicalMaterial({ color: c.housing, roughness: 0.55, specularIntensity: 0.5 }), render, m, level),
    button: projected(new THREE.MeshPhysicalMaterial({ color: '#1a1b1d', roughness: 0.35, metalness: 0.35, clearcoat: 0.3 }), render, m, level),
    back: new THREE.MeshPhysicalMaterial({ color: c.back, roughness: 0.45, clearcoat: 0.4 }),
    window: new THREE.MeshPhysicalMaterial({ color: '#0b0c0e', roughness: 0.08, clearcoat: 1 }),
  };

  // UNA's parts, in display radii (the GLB is in metres).
  const buttons = { l1: [], l2: [], r1: [], r2: [] } as Record<Btn, THREE.Object3D[]>;
  const k = 1000 / MM;
  cad.updateMatrixWorld(true);
  cad.traverse((o) => {
    const mesh = o as THREE.Mesh;
    if (!mesh.isMesh) return;
    const g = mesh.geometry.clone();
    g.applyMatrix4(mesh.matrixWorld);
    g.scale(k, k, k);
    const n = mesh.name || mesh.parent?.name || '';
    const mat = n.startsWith('bezel') ? mats.bezel : n.startsWith('housing') ? mats.housing : n.startsWith('button') ? mats.button
      : n === 'sensor-window' ? mats.window : mats.back;
    const part = new THREE.Mesh(g, mat);
    body.add(part);
    const b = n.match(/^button-(l1|l2|r1|r2)/)?.[1] as Btn | undefined;
    if (b) buttons[b].push(part);
  });

  // The lens under the bezel's rim: UNA's printed ring round the live display, glass over both.
  const lensZ = mm(-0.6);
  const ring = new THREE.Mesh(new THREE.RingGeometry(1.0, mm(20.5), 192, 1),
    projected(new THREE.MeshPhysicalMaterial({ color: '#0d0d0e', roughness: 0.5 }), render, m, level, 0.2));
  ring.position.z = lensZ - 0.004;
  body.add(ring);
  screen.position.z = lensZ - 0.01;
  body.add(screen);
  const well = new THREE.Mesh(new THREE.CircleGeometry(mm(20.6), 128), new THREE.MeshBasicMaterial({ color: 0x000000 }));
  well.position.z = lensZ - 0.03;
  body.add(well);
  const glass = new THREE.Mesh(new THREE.CircleGeometry(mm(20.4), 160), new THREE.MeshPhysicalMaterial({
    color: 0x000000, roughness: 0.05, clearcoat: 1, clearcoatRoughness: 0.04, transparent: true, opacity: 0.3,
    blending: THREE.AdditiveBlending, depthWrite: false,
  }));
  glass.position.z = lensZ;
  glass.renderOrder = 2;
  body.add(glass);

  // The USB-C cover in the housing's port at 3 o'clock.
  const usb = new RoundedBoxGeometry(mm(1.6), mm(7.6), mm(5.2), 3, mm(0.5));
  usb.translate(mm(23.25), 0, mm(-6.5));
  body.add(new THREE.Mesh(usb, projected(new THREE.MeshPhysicalMaterial({ color: c.rubber, roughness: 0.7 }), render, m, level)));

  // UNA pressed into the housing's left side, as on the watch.
  const una = new THREE.Mesh(new THREE.PlaneGeometry(mm(7.5), mm(7.5) / 2.985), new THREE.MeshStandardMaterial({
    map: wordmark, transparent: true, opacity: colourway === 'white' ? 0.3 : 0.2, color: colourway === 'white' ? '#6d6e70' : '#9a9ca0',
    depthWrite: false, polygonOffset: true, polygonOffsetFactor: -2,
  }));
  una.rotation.set(0, -Math.PI / 2, -Math.PI / 2);
  una.position.set(mm(-23.72), 0, mm(-7.4));
  body.add(una);

  body.add(strap(c.strapOut, c.strapIn));

  const buttonDir = {} as Record<Btn, THREE.Vector3>;
  const buttonTip = {} as Record<Btn, THREE.Vector3>;
  (Object.keys(buttons) as Btn[]).forEach((b) => {
    const d = dirOf(meta.buttons[b].deg);
    buttonDir[b] = new THREE.Vector3(d.x, d.y, 0);
    buttonTip[b] = new THREE.Vector3(d.x * meta.buttons[b].r, d.y * meta.buttons[b].r, mm(-6.5));
  });
  return { root, body, level, buttons, buttonDir, buttonTip };
}
