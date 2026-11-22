// The overlay, drawn after post (no bloom, no grain) in the delivery size's pixels: captions,
// line-art callouts with leader lines, the time-lapse tag, and the UNA x HybridX lockup.

import * as THREE from 'three';
import { SVGLoader } from 'three/examples/jsm/loaders/SVGLoader.js';
import { BRAND, CAPTIONS, CALLOUTS, CTA, FAMILY, MONTAGE, RACE_BEATS, RATIOS, STREAK_BEATS, WEEK_BEATS, type Ratio } from '../config';
import { LAYOUTS, type Layout } from '../layout';
import type { Frame } from '../app';
import type { World } from '../world/world';
import { ease, span } from '../timeline';
import { makeText, textsReady, widthOf } from './type';
import type { Text } from 'troika-three-text';
import { GEN } from '../app';

type Callout = {
  t0: number; t1: number;
  anchor: (w: World, f: Frame) => THREE.Vector3 | null;
  side: 1 | -1 | 0; // label to the right (1), left (-1), or centred under the leader (0); fixed, as text lays out once
  fixedY?: number; // no leader: the label centred at this height (px from the top)
  up: 1 | -1; // the leader goes up (1) or down (-1)
  label: Text;
  line: THREE.Mesh[];
  dot: THREE.Mesh;
};

const flat = (color: THREE.ColorRepresentation, opacity = 1) =>
  new THREE.MeshBasicMaterial({ color, transparent: true, opacity, depthTest: false, depthWrite: false });

export class Hud {
  readonly scene = new THREE.Scene();
  readonly camera: THREE.OrthographicCamera;
  private L: Layout;
  private Rw: number;
  private Rh: number;
  private captions: { t0: number; t1: number; text: Text; rule: THREE.Mesh }[] = [];
  private callouts: Callout[] = [];
  private tag: Text;
  private tagDot: THREE.Mesh;
  private lockup = new THREE.Group();
  private una: THREE.Mesh;
  private times: Text;
  private xmark = new THREE.Group();
  private name: Text;
  private cta: Text;

  constructor(ratio: Ratio, _w: number, _h: number, wordmark: THREE.Texture) {
    this.L = LAYOUTS[ratio];
    this.Rw = RATIOS[ratio].w;
    this.Rh = RATIOS[ratio].h;
    this.camera = new THREE.OrthographicCamera(0, this.Rw, 0, -this.Rh, -10, 10);
    const k = this.Rw < this.Rh ? 1.18 : 1; // small type a little larger on a phone

    for (const c of CAPTIONS) {
      const text = makeText({ text: c.br?.[ratio] ?? c.text, size: this.L.caption.size, bold: true, maxWidth: this.L.caption.maxW, anchorY: 'bottom', lineHeight: 1.06, tracking: -0.01 });
      text.position.set(this.L.caption.x, -this.L.caption.bottom, 0);
      const rule = new THREE.Mesh(new THREE.PlaneGeometry(1, 1), flat(BRAND.yellow));
      this.scene.add(text, rule);
      this.captions.push({ t0: c.t0, t1: c.t1, text, rule });
    }

    const label = (s: string, side: 1 | -1 | 0, up: 1 | -1 = -1, size = 22 * k) => makeText({ text: s, size, tracking: 0.12, color: '#ffffff', anchorX: side > 0 ? 'left' : side < 0 ? 'right' : 'center', anchorY: side !== 0 ? 'middle' : up > 0 ? 'bottom' : 'top' });
    // On a narrow frame a label beside its anchor would run off the edge: hang it below instead.
    const narrow = this.Rw <= this.Rh;
    const add = (c: Omit<Callout, 'line' | 'dot'>) => {
      const line = [0, 1].map(() => new THREE.Mesh(new THREE.PlaneGeometry(1, 1), flat('#ffffff', 0.85)));
      const dot = new THREE.Mesh(new THREE.RingGeometry(4, 6.5, 32), flat(BRAND.yellow));
      this.scene.add(c.label, ...line, dot);
      this.callouts.push({ ...c, line, dot });
    };
    // Scene 1: what each app does for you, from its icon.
    FAMILY.forEach((f, i) => {
      // In step with the app's slot on the screen (screen/script.ts montageApp).
      const slot0 = MONTAGE.start + i * MONTAGE.step;
      const t0 = slot0 + (narrow ? 0.15 : 0.35);
      // Which side of the watch the icon is on during its callout (Family.slot's angle).
      const a = Math.PI / 2 - (i / FAMILY.length) * Math.PI * 2 - 0.05 * (t0 + 0.5);
      // On a narrow frame a label by its icon would land on the watch: the benefit takes one line
      // between the watch and the caption instead, and the icon's halo says whose it is.
      const side = narrow ? 0 : Math.cos(a) >= 0 ? 1 : -1, up = Math.sin(a) >= 0 ? 1 : -1;
      add({ t0, t1: slot0 + MONTAGE.step, anchor: (w) => w.family.edgeOf(i), side, up, fixedY: narrow ? this.L.benefitY : undefined, label: label(f.benefit, side, -1, narrow ? 28 * k : 22 * k) });
    });
    add({ t0: WEEK_BEATS.start + 0.6, t1: WEEK_BEATS.start + 3 * WEEK_BEATS.step - 0.4, anchor: (w) => w.week.corner(), side: 1, up: 1, label: label(CALLOUTS.shared, 1) });
    const sp = narrow ? 0 : 1;
    add({ t0: RACE_BEATS.gun + 1.0, t1: RACE_BEATS.gun + 4.6, anchor: (_, f) => f.watch.body.localToWorld(f.watch.buttonTip.r2.clone()), side: sp, up: -1, label: label(CALLOUTS.split, sp) });
    add({ t0: STREAK_BEATS.summit + 0.9, t1: STREAK_BEATS.load - 0.1, anchor: (w) => w.streak.shieldAnchor(), side: sp, up: 1, label: label(CALLOUTS.shield, sp, 1) });

    this.tag = makeText({ text: CALLOUTS.timelapse, size: 20 * k, tracking: 0.16, color: '#ffffff', anchorX: 'right', anchorY: 'middle' });
    this.tagDot = new THREE.Mesh(new THREE.CircleGeometry(6, 24), flat(BRAND.yellow));
    this.scene.add(this.tag, this.tagDot);

    // The lockup: UNA's wordmark, a times sign, HybridX's X and the name.
    const h = this.L.lockup.h;
    const aspect = (wordmark.image as HTMLImageElement).width / (wordmark.image as HTMLImageElement).height;
    this.una = new THREE.Mesh(new THREE.PlaneGeometry(h * 0.78 * aspect, h * 0.78), new THREE.MeshBasicMaterial({ map: wordmark, transparent: true, depthTest: false }));
    this.times = makeText({ text: '×', size: h * 0.95, color: '#8c8c8c', anchorX: 'center', anchorY: 'middle' });
    this.name = makeText({ text: 'HYBRIDX', size: h * 0.9, bold: true, tracking: 0.02, anchorY: 'middle' });
    this.cta = makeText({ text: CTA, size: h * 0.42, tracking: 0.24, color: '#ffffff', anchorX: 'center', anchorY: 'top' });
    this.lockup.add(this.una, this.times, this.xmark, this.name);
    this.scene.add(this.lockup, this.cta);
    this.scene.traverse((o) => { o.renderOrder = 100; const m = (o as THREE.Mesh).material as THREE.Material | undefined; if (m) m.depthTest = false; });
  }

  async ready() {
    // HybridX's X, traced from the brand's mark (scripts/trace-xmark.py).
    const svg = await new SVGLoader().loadAsync(`${GEN}x-mark.svg`);
    const vb = (svg.xml as unknown as SVGSVGElement).getAttribute('viewBox')!.split(' ').map(Number);
    const h = this.L.lockup.h * 0.92;
    const s = h / vb[3];
    const mat = flat(BRAND.yellow);
    for (const p of svg.paths) {
      for (const shape of SVGLoader.createShapes(p)) {
        const m = new THREE.Mesh(new THREE.ShapeGeometry(shape, 8), mat);
        m.scale.set(s, -s, 1);
        m.position.set(-(vb[2] * s) / 2, h / 2, 0);
        m.renderOrder = 100;
        this.xmark.add(m);
      }
    }
    this.xmark.userData.w = vb[2] * s;
    await textsReady();
    // Lay the lockup out now the text widths are known.
    const gap = this.L.lockup.h * 0.42;
    const unaW = (this.una.geometry as THREE.PlaneGeometry).parameters.width;
    const widths = [unaW, this.L.lockup.h * 0.36, this.xmark.userData.w as number, widthOf(this.name)];
    const total = widths.reduce((a, b) => a + b, 0) + gap * 2 + this.L.lockup.h * 0.22;
    let x = -total / 2;
    this.una.position.x = x + widths[0] / 2; x += widths[0] + gap;
    this.times.position.x = x + widths[1] / 2; x += widths[1] + gap;
    this.xmark.position.x = x + widths[2] / 2; x += widths[2] + this.L.lockup.h * 0.22;
    this.name.position.x = x;
    this.lockup.position.set(this.Rw / 2, -this.L.lockup.y, 0);
    this.cta.position.set(this.Rw / 2, -this.L.lockup.y - this.L.lockup.h * 1.0, 0);
    // Caption rules: a short yellow bar over each caption.
    for (const c of this.captions) {
      const b = c.text.textRenderInfo?.blockBounds;
      const top = b ? b[3] : this.L.caption.size;
      c.rule.scale.set(this.L.caption.size * 1.1, Math.max(4, this.L.caption.size * 0.09), 1);
      c.rule.position.set(this.L.caption.x + (this.L.caption.size * 1.1) / 2, -this.L.caption.bottom + top + this.L.caption.size * 0.42, 0);
    }
  }

  /** World to overlay pixels. */
  private toPx(p: THREE.Vector3, cam: THREE.Camera): THREE.Vector2 {
    const v = p.clone().project(cam);
    return new THREE.Vector2((v.x * 0.5 + 0.5) * this.Rw, -(1 - (v.y * 0.5 + 0.5)) * this.Rh);
  }

  private seg(m: THREE.Mesh, a: THREE.Vector2, b: THREE.Vector2, w = 2) {
    const d = b.clone().sub(a);
    m.position.set((a.x + b.x) / 2, (a.y + b.y) / 2, 0);
    m.scale.set(Math.max(0.001, d.length()), w, 1);
    m.rotation.z = Math.atan2(d.y, d.x);
  }

  update(f: Frame, w: World) {
    const { t, S, camera } = f;
    // Captions: rise in, fade out.
    for (const c of this.captions) {
      const a = span(t, c.t0, c.t0 + 0.55, 'power3.out') * (1 - span(t, c.t1 - 0.35, c.t1, 'power2.in'));
      c.text.visible = c.rule.visible = a > 0.001;
      c.text.fillOpacity = a;
      c.text.position.y = -this.L.caption.bottom - 26 * (1 - span(t, c.t0, c.t0 + 0.6, 'power3.out'));
      (c.rule.material as THREE.MeshBasicMaterial).opacity = a;
      c.rule.scale.x = this.L.caption.size * 1.1 * span(t, c.t0 + 0.05, c.t0 + 0.6, 'power3.out');
      c.rule.position.x = this.L.caption.x + c.rule.scale.x / 2;
    }

    // Callouts: the dot, the line out, the elbow, the label.
    const k = this.Rw < this.Rh ? 1.18 : 1;
    for (const c of this.callouts) {
      const on = t >= c.t0 && t <= c.t1;
      const p = on ? c.anchor(w, f) : null;
      const vis = !!p;
      c.label.visible = c.dot.visible = vis;
      c.line.forEach((m) => (m.visible = vis));
      if (!p) continue;
      const a0 = this.toPx(p, camera);
      const { side, up } = c;
      const g1 = span(t, c.t0, c.t0 + 0.3, 'power3.out');
      const g2 = span(t, c.t0 + 0.2, c.t0 + 0.4, 'power3.out');
      const fadeOut = 1 - span(t, c.t1 - 0.3, c.t1, 'power2.in');
      c.dot.position.set(a0.x, a0.y, 0);
      (c.dot.material as THREE.MeshBasicMaterial).opacity = fadeOut;
      c.line.forEach((m) => ((m.material as THREE.MeshBasicMaterial).opacity = 0.85 * fadeOut));
      c.label.fillOpacity = span(t, c.t0 + 0.3, c.t0 + 0.6, 'power2.out') * fadeOut;
      if (c.fixedY !== undefined) {
        c.line.forEach((m) => (m.visible = false));
        c.dot.visible = false;
        c.label.position.set(this.Rw / 2, -c.fixedY + 8 * (1 - span(t, c.t0 + 0.3, c.t0 + 0.6, 'power3.out')), 0);
        continue;
      }
      if (side === 0) {
        // Straight down (or up), the label centred on it and kept inside the frame.
        const end = a0.clone().add(new THREE.Vector2(0, up * 40 * k * g1));
        this.seg(c.line[0], a0, end);
        c.line[1].visible = false;
        const half = widthOf(c.label) / 2 + 40;
        c.label.position.set(Math.min(this.Rw - half, Math.max(half, end.x)), end.y + up * 10 * k, 0);
        continue;
      }
      const elbow = a0.clone().add(new THREE.Vector2(side * 46 * k, up * 46 * k).multiplyScalar(g1));
      const end = elbow.clone().add(new THREE.Vector2(side * 34 * k * g2, 0));
      this.seg(c.line[0], a0, elbow);
      this.seg(c.line[1], elbow, end);
      c.label.position.set(end.x + side * 12 * k, end.y, 0);
    }

    // The time-lapse tag, top right, while the race runs fast.
    const tl0 = RACE_BEATS.gun + 0.3, tl1 = RACE_BEATS.open[15];
    const ta = span(t, tl0, tl0 + 0.3) * (1 - span(t, tl1 - 0.3, tl1));
    this.tag.visible = this.tagDot.visible = ta > 0.001;
    const tagY = this.Rw < this.Rh ? -300 : -78;
    this.tag.position.set(this.Rw - 72, tagY, 0);
    this.tag.fillOpacity = ta;
    this.tagDot.position.set(this.Rw - 72 - widthOf(this.tag) - 22, tagY, 0);
    (this.tagDot.material as THREE.MeshBasicMaterial).opacity = ta * (0.6 + 0.4 * Math.sin(t * 9));

    // The lockup and the address.
    const la = S.lockup.a;
    this.lockup.visible = la > 0.001;
    this.lockup.traverse((o) => {
      const m = (o as THREE.Mesh).material as THREE.MeshBasicMaterial | undefined;
      if ('fillOpacity' in o) (o as unknown as Text).fillOpacity = la;
      else if (m) m.opacity = la;
    });
    this.lockup.position.y = -this.L.lockup.y - 18 * (1 - ease('power3.out')(Math.min(1, la)));
    this.cta.visible = S.lockup.cta > 0.001;
    this.cta.fillOpacity = S.lockup.cta;
  }

  render(renderer: THREE.WebGLRenderer) {
    renderer.render(this.scene, this.camera);
  }
}
