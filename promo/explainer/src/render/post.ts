// Post-processing: render, a subtle bloom (half resolution), SMAA, a faint seeded grain, output.
// Bloom's threshold is above 1.0 so the watch's screen (drawn at most 1.0) never blooms; the
// world's holograms and highlights are drawn brighter on purpose.

import * as THREE from 'three';
import { EffectComposer } from 'three/examples/jsm/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/examples/jsm/postprocessing/RenderPass.js';
import { UnrealBloomPass } from 'three/examples/jsm/postprocessing/UnrealBloomPass.js';
import { SMAAPass } from 'three/examples/jsm/postprocessing/SMAAPass.js';
import { ShaderPass } from 'three/examples/jsm/postprocessing/ShaderPass.js';
import { OutputPass } from 'three/examples/jsm/postprocessing/OutputPass.js';

const GrainShader = {
  uniforms: { tDiffuse: { value: null }, seed: { value: 0 }, amount: { value: 0.035 } },
  vertexShader: /* glsl */ `
    varying vec2 vUv;
    void main() { vUv = uv; gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0); }`,
  fragmentShader: /* glsl */ `
    uniform sampler2D tDiffuse; uniform float seed; uniform float amount; varying vec2 vUv;
    float h(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233)) + seed * 0.618) * 43758.5453); }
    void main() {
      vec4 c = texture2D(tDiffuse, vUv);
      float n = h(gl_FragCoord.xy) - 0.5;
      // Grain is strongest in the mid tones and leaves black black.
      float l = dot(c.rgb, vec3(0.299, 0.587, 0.114));
      c.rgb += n * amount * smoothstep(0.0, 0.25, l) * (1.0 - smoothstep(0.6, 1.0, l));
      gl_FragColor = c;
    }`,
};

export type Post = {
  composer: EffectComposer;
  bloom: UnrealBloomPass;
  grain: ShaderPass;
  setSize: (w: number, h: number) => void;
};

export function createPost(renderer: THREE.WebGLRenderer, scene: THREE.Scene, camera: THREE.Camera, w: number, h: number, lite: boolean): Post {
  const target = new THREE.WebGLRenderTarget(w, h, { type: THREE.HalfFloatType });
  const composer = new EffectComposer(renderer, target);
  composer.addPass(new RenderPass(scene, camera));
  const bloom = new UnrealBloomPass(new THREE.Vector2(w / 2, h / 2), 0.42, 0.45, 1.12);
  composer.addPass(bloom);
  composer.addPass(new SMAAPass());
  const grain = new ShaderPass(GrainShader);
  grain.enabled = !lite;
  composer.addPass(grain);
  composer.addPass(new OutputPass());
  return {
    composer, bloom, grain,
    setSize(nw, nh) {
      composer.setSize(nw, nh);
      bloom.resolution.set(nw / 2, nh / 2);
    },
  };
}
