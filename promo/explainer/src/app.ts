// The explainer: builds the stage, the watch, the world and the overlay, and draws any frame from
// film time alone (renderAt). Live playback and the exporter both call renderAt.

import * as THREE from 'three';
import type gsap from 'gsap';
import { DURATION, RATIOS, FAMILY, STREAK_BEATS, type Ratio } from './config';
import { LAYOUTS, type Layout } from './layout';
import { buildTimeline, type State } from './timeline';
import { createPost, type Post } from './render/post';
import { environment, lights, grid, ContactShadow } from './render/stage';
import { buildWatch, type WatchParts, type WatchMeta } from './watch/model';
import { Screen } from './watch/screen';
import { Feedback } from './watch/feedback';
import { loadFonts } from './screen/font';
import { loadPreviews } from './screen/family';
import { screenAt } from './screen/script';
import { World } from './world/world';
import { Hud } from './hud/hud';
import { loadType } from './hud/type';

export const GEN = './gen/';

export type Frame = { t: number; S: State; camera: THREE.PerspectiveCamera; ratio: Ratio; L: Layout; watch: WatchParts; W: number; H: number };

function loadTexture(url: string, srgb = true): Promise<THREE.Texture> {
  return new Promise((ok, fail) => new THREE.TextureLoader().load(url, (tx) => {
    if (srgb) tx.colorSpace = THREE.SRGBColorSpace;
    tx.anisotropy = 8;
    ok(tx);
  }, undefined, fail));
}

export class App {
  readonly renderer: THREE.WebGLRenderer;
  readonly scene = new THREE.Scene();
  readonly camera = new THREE.PerspectiveCamera(30, 16 / 9, 0.1, 200);
  readonly ratio: Ratio;
  readonly W: number;
  readonly H: number;
  private post!: Post;
  private S!: State;
  private tl!: ReturnType<typeof gsap.timeline>;
  private lights!: ReturnType<typeof lights>;
  private gridMat!: THREE.ShaderMaterial;
  private shadow!: ContactShadow;
  private screen!: Screen;
  private feedback!: Feedback;
  private world!: World;
  private hud!: Hud;
  watch!: WatchParts;
  private lite: boolean;

  private constructor(canvas: HTMLCanvasElement, ratio: Ratio, exporting: boolean) {
    this.ratio = ratio;
    const R = RATIOS[ratio];
    // Exports render at the delivery size; live playback at the same size, scaled by CSS, unless
    // the device is a phone or weak: then at most 1.5 x its CSS pixels and about a million pixels a
    // frame, without grain. (Quality only: the timeline is the same.)
    const coarse = typeof matchMedia === 'function' && matchMedia('(pointer: coarse)').matches;
    this.lite = !exporting && ((navigator.hardwareConcurrency ?? 8) <= 4 || coarse);
    let scale = exporting ? 1 : Math.min(1, (Math.min(window.devicePixelRatio, this.lite ? 1.5 : 2) * Math.max(window.innerWidth / R.w, window.innerHeight / R.h)));
    if (this.lite) scale = Math.min(scale, Math.sqrt(1.0e6 / (R.w * R.h)));
    this.W = Math.round(R.w * Math.max(scale, 0.5));
    this.H = Math.round(R.h * Math.max(scale, 0.5));
    this.renderer = new THREE.WebGLRenderer({ canvas, antialias: false, preserveDrawingBuffer: exporting, powerPreference: 'high-performance' });
    this.renderer.setPixelRatio(1);
    this.renderer.setSize(this.W, this.H, false);
    this.renderer.toneMapping = THREE.NoToneMapping;
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    this.camera.aspect = this.W / this.H;
  }

  static async create(canvas: HTMLCanvasElement, ratio: Ratio, exporting: boolean): Promise<App> {
    const app = new App(canvas, ratio, exporting);
    await app.init();
    return app;
  }

  private async init() {
    const L = LAYOUTS[this.ratio];
    const [render, meta, wordmark, icons] = await Promise.all([
      loadTexture(`${GEN}una/watch-graphite.png`),
      fetch(`${GEN}una/meta.json`).then((r) => r.json() as Promise<WatchMeta>),
      loadTexture(`${GEN}una/una-wordmark.png`),
      Promise.all(FAMILY.map((f) => loadTexture(`${GEN}icons/${f.id}.png`))),
      loadFonts(GEN),
      loadPreviews(GEN, ['run', 'trail', 'ride', 'workout', 'training']),
      loadType(GEN),
    ]);
    environment(this.renderer, this.scene);
    this.lights = lights(this.scene);
    this.gridMat = grid(this.scene, L.floorY);
    this.shadow = new ContactShadow(this.scene, L.floorY);

    this.screen = new Screen();
    this.watch = buildWatch(render, meta, this.screen.mesh);
    this.scene.add(this.watch.root);
    this.feedback = new Feedback(this.scene);

    const { tl, S } = buildTimeline(this.ratio);
    this.tl = tl;
    this.S = S;
    this.world = new World(this.scene, this.ratio, icons);
    this.hud = new Hud(this.ratio, this.W, this.H, wordmark);
    this.post = createPost(this.renderer, this.scene, this.camera, this.W, this.H, this.lite);
    await Promise.all([this.world.ready(), this.hud.ready()]);
  }

  get duration() { return DURATION; }

  /** Every material's opacity and every object's visibility, in scene order (determinism checks). */
  dumpState(): string {
    const out: string[] = [];
    this.scene.traverse((o) => {
      const m = (o as THREE.Mesh).material as (THREE.Material & { opacity: number }) | undefined;
      const fo = (o as unknown as { fillOpacity?: number }).fillOpacity;
      out.push(`${o.type}:${o.visible ? 1 : 0}:${m ? m.opacity.toFixed(4) : '-'}:${fo !== undefined ? fo.toFixed(4) : '-'}:${o.position.toArray().map((v) => v.toFixed(3)).join(',')}`);
    });
    return out.join('\n');
  }

  /** Draws the frame at film time t. */
  renderAt(t: number) {
    t = Math.max(0, Math.min(DURATION, t));
    this.tl.seek(t, true);
    const S = this.S;
    const L = LAYOUTS[this.ratio];

    // Camera.
    this.camera.position.set(S.cam.px, S.cam.py, S.cam.pz);
    this.camera.fov = S.cam.fov;
    this.camera.updateProjectionMatrix();
    this.camera.lookAt(S.cam.lx, S.cam.ly, S.cam.lz);
    this.camera.updateMatrixWorld();

    // Light.
    this.scene.environmentIntensity = 0.32 * S.light.env;
    this.lights.key.intensity = S.light.key;
    this.lights.rim.intensity = S.light.rim;
    this.lights.rim.position.set(S.light.rimX, 2.5, -5);
    this.lights.fill.intensity = 0.35 * S.light.env;
    this.gridMat.uniforms.opacity.value = S.light.grid;

    // The watch: UNA's render, square to the camera with a slight idle sway; for Race to X Streak
    // it dips back and returns, the screen passing through black as one app closes and the next opens.
    const w = this.watch;
    const dip = Math.sin(Math.PI * S.watch.swap);
    w.root.position.set(0, 0, -0.9 * dip);
    w.root.scale.setScalar(1 - 0.05 * dip);
    w.root.quaternion.copy(this.camera.quaternion);
    w.root.rotateX(0.025 * Math.sin(t * 0.37));
    w.root.rotateY(0.035 * Math.sin(t * 0.29));
    const look = w.look.uniforms;
    look.level.value = Math.min(1, 0.035 + 0.965 * S.light.env);
    look.rim.value = 0.42 * S.light.rim;
    look.rimDir.value.set(S.light.rimX, 2.5).normalize();
    look.sweep.value = (S.light.rim / 4.5) * (1 - S.light.env) * 0.55;
    look.sweepPos.value = (S.light.rimX / 9) * 2.4;
    const fromSwap = Math.abs(t - (STREAK_BEATS.swap + STREAK_BEATS.swapDur / 2));
    this.screen.power = THREE.MathUtils.smoothstep(fromSwap, 0.05, 0.22);
    this.screen.draw(screenAt(t));
    const dist = this.camera.position.distanceTo(w.root.position);
    const pxToWorld = (2 * dist * Math.tan((this.camera.fov * Math.PI) / 360)) / this.H;
    this.feedback.update(t, w, this.camera, pxToWorld);

    const f: Frame = { t, S, camera: this.camera, ratio: this.ratio, L, watch: w, W: this.W, H: this.H };
    this.world.update(f);
    this.hud.update(f, this.world);

    // Draw: the contact shadow, the scene through post, then the overlay on top. Post runs with
    // auto-clear on: SMAA's edge pass discards where there is no edge and relies on the clear,
    // or the last frame's edges would leak into this one.
    this.renderer.autoClear = true;
    this.renderer.setRenderTarget(null);
    this.renderer.clear();
    w.card.visible = false;
    w.shadowCaster.visible = true;
    this.shadow.update(this.renderer, this.scene, w.root);
    w.shadowCaster.visible = false;
    w.card.visible = true;
    this.post.grain.uniforms.seed.value = Math.round(t * 60) % 997;
    this.post.composer.render();
    this.renderer.setRenderTarget(null);
    this.renderer.autoClear = false;
    this.renderer.clearDepth();
    this.hud.render(this.renderer);
  }
}
