// UNA's watch: UNA's own render, the graphite watch from the front (the SDK's Figma pack, read by
// scripts/extract-una.py), on a card that always faces the camera, with the film's live screen
// behind its transparent opening. The pack has the front view only, so the watch never turns:
// the camera and the world move round it. Units: the display's radius is 1.
//
// The card is lit in its shader: the room's light scales UNA's own shading (at full light the
// render's colours are shown as they are), an edge light picks out the outline on the side the
// rim light is on, and in the dark intro a soft band of light sweeps across the face.
//
// A plain case shape (never drawn) stands behind the card for the floor's contact shadow.

import * as THREE from 'three';
import type { Btn } from '../screen/lvgl';

export type WatchMeta = {
  watch: { cx: number; cy: number; r: number; w: number; h: number };
  buttons: Record<Btn, { deg: number; r: number }>;
};

/** TouchGFX degrees (0 = 12 o'clock, clockwise) to a unit direction in the face's plane. */
export const dirOf = (deg: number) => {
  const a = ((90 - deg) * Math.PI) / 180;
  return new THREE.Vector2(Math.cos(a), Math.sin(a));
};

/** The case's front outline: a 1.47 circle joined with the lug block (|x| 1.03, |y| 1.45, r 0.22). */
function caseOutline(n = 128): THREE.Shape {
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

const VERT = /* glsl */ `
  varying vec2 vUv;
  varying vec2 vP;
  void main() {
    vUv = uv;
    vP = position.xy;
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
  }`;

/** UNA's render, lit. Its texture is sRGB, so it samples as linear; the output stays linear (post converts). */
const FRAG = /* glsl */ `
  uniform sampler2D map;
  uniform vec2 texel;      // one texture pixel in uv
  uniform float level;     // the room's light: 1 shows the render's own colours
  uniform float rim;       // the edge light
  uniform vec2 rimDir;     // towards the rim light, in the face's plane
  uniform float sweep;     // the intro's band of light
  uniform float sweepPos;
  uniform vec2 fade;       // the strap fades out between these heights (display radii)
  uniform float cut;       // alpha under which nothing is drawn (the depth pass uses 0.5)
  varying vec2 vUv;
  varying vec2 vP;
  float A(vec2 o) { return texture2D(map, vUv + o * texel).a; }
  void main() {
    vec4 c = texture2D(map, vUv);
    float a = c.a * (1.0 - smoothstep(fade.x, fade.y, abs(vP.y)));
    if (a < cut) discard;
    vec3 col = c.rgb * level;
    // Edge light: the outline's outward normal from the alpha's slope, lit where it faces the light.
    vec2 g = vec2(A(vec2(3.0, 0.0)) - A(vec2(-3.0, 0.0)), A(vec2(0.0, 3.0)) - A(vec2(0.0, -3.0)));
    float e = length(g);
    float facing = e > 0.001 ? max(0.0, dot(-g / e, rimDir)) : 0.0;
    float lum = dot(c.rgb, vec3(0.2126, 0.7152, 0.0722));
    col += vec3(rim * min(e, 1.0) * facing * facing * (0.3 + 2.0 * lum));
    // The intro's sweep: a soft diagonal band that lifts UNA's shading where it passes.
    float s = dot(vP, vec2(0.94, 0.34));
    col += c.rgb * sweep * exp(-pow((s - sweepPos) / 0.55, 2.0)) * 2.2;
    gl_FragColor = vec4(col, a);
  }`;

export type WatchParts = {
  root: THREE.Group; // placed and turned to the camera by the film
  body: THREE.Group; // shaken by the haptics
  card: THREE.Group; // UNA's render (depth and colour passes)
  look: THREE.ShaderMaterial; // the render's light
  shadowCaster: THREE.Object3D; // the case shape, shown only for the contact shadow
  buttonTip: Record<Btn, THREE.Vector3>; // the outer edge of each button, in body space
};

export function buildWatch(render: THREE.Texture, meta: WatchMeta, screen: THREE.Mesh): WatchParts {
  const root = new THREE.Group();
  const body = new THREE.Group();
  root.add(body);
  const { cx, cy, r, w, h } = meta.watch;

  // The card: the image in display radii, its screen opening's centre at the origin.
  render.generateMipmaps = true;
  render.minFilter = THREE.LinearMipmapLinearFilter;
  render.magFilter = THREE.LinearFilter;
  const geo = new THREE.PlaneGeometry(w / r, h / r);
  geo.translate((w / 2 - cx) / r, -(h / 2 - cy) / r, 0);
  const uniforms = {
    map: { value: render },
    texel: { value: new THREE.Vector2(1 / w, 1 / h) },
    level: { value: 1 },
    rim: { value: 0 },
    rimDir: { value: new THREE.Vector2(1, 0) },
    sweep: { value: 0 },
    sweepPos: { value: 0 },
    // The strap's cut ends: gone by the nearer of the two (the image has a 24 px clear margin).
    fade: { value: new THREE.Vector2(1.55, Math.min(cy - 24, h - 24 - cy) / r) },
    cut: { value: 0.004 },
  };
  // Two passes: depth where the watch is solid (so the race ring passes behind it), then colour.
  const depthMat = new THREE.ShaderMaterial({ uniforms: { ...uniforms, cut: { value: 0.5 } }, vertexShader: VERT, fragmentShader: FRAG, colorWrite: false });
  const look = new THREE.ShaderMaterial({ uniforms, vertexShader: VERT, fragmentShader: FRAG, transparent: true, depthWrite: false });
  const card = new THREE.Group();
  const depth = new THREE.Mesh(geo, depthMat);
  depth.renderOrder = 1;
  const colour = new THREE.Mesh(geo, look);
  colour.renderOrder = 2;
  card.add(depth, colour);
  body.add(card);

  // The live display just behind the opening, on black (the opening's soft edge shows black, as
  // the glass over a display's border does).
  screen.position.z = -0.004;
  body.add(screen);
  const surround = new THREE.Mesh(new THREE.CircleGeometry(1.06, 128), new THREE.MeshBasicMaterial({ color: 0x000000 }));
  surround.position.z = -0.008;
  body.add(surround);

  // The case shape behind the card, for the contact shadow only.
  const caseGeo = new THREE.ExtrudeGeometry(caseOutline(), { depth: 0.55, bevelEnabled: false });
  caseGeo.translate(0, 0, -0.6);
  const strap = new THREE.BoxGeometry(1.41, 0.7, 0.12);
  const shadowCaster = new THREE.Group();
  shadowCaster.add(new THREE.Mesh(caseGeo));
  for (const sy of [-1, 1]) {
    const s = new THREE.Mesh(strap);
    s.position.set(0, sy * 1.75, -0.2);
    shadowCaster.add(s);
  }
  shadowCaster.visible = false;
  body.add(shadowCaster);

  const buttonTip = {} as Record<Btn, THREE.Vector3>;
  (Object.keys(meta.buttons) as Btn[]).forEach((k) => {
    const d = dirOf(meta.buttons[k].deg);
    buttonTip[k] = new THREE.Vector3(d.x * meta.buttons[k].r, d.y * meta.buttons[k].r, 0.02);
  });

  return { root, body, card, look, shadowCaster, buttonTip };
}
