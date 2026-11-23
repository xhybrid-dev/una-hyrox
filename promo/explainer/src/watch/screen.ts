// The watch's display: a 240 x 240 canvas redrawn every frame from the screen script, shown with
// nearest-pixel magnification and quantised to the display's 64 colours (2 bits a channel) in the
// shader. Flat colour, no glow: drawn at most 1.0, under the bloom threshold.

import * as THREE from 'three';

export class Screen {
  readonly canvas: HTMLCanvasElement;
  readonly ctx: CanvasRenderingContext2D;
  readonly texture: THREE.CanvasTexture;
  readonly mesh: THREE.Mesh;
  private mat: THREE.ShaderMaterial;

  constructor() {
    this.canvas = document.createElement('canvas');
    this.canvas.width = this.canvas.height = 240;
    this.ctx = this.canvas.getContext('2d', { willReadFrequently: false })!;
    this.texture = new THREE.CanvasTexture(this.canvas);
    this.texture.colorSpace = THREE.NoColorSpace; // raw bytes: the shader quantises them
    this.texture.magFilter = THREE.NearestFilter;
    this.texture.minFilter = THREE.LinearFilter;
    this.texture.generateMipmaps = false;
    this.mat = new THREE.ShaderMaterial({
      uniforms: { map: { value: this.texture }, power: { value: 1 } },
      vertexShader: /* glsl */ `
        varying vec2 vUv;
        void main() { vUv = uv; gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0); }`,
      fragmentShader: /* glsl */ `
        uniform sampler2D map; uniform float power; varying vec2 vUv;
        vec3 toLinear(vec3 c) { return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c)); }
        void main() {
          vec3 c = texture2D(map, vUv).rgb;
          c = floor(c * 3.0 + 0.5) / 3.0;           // 0, 85, 170, 255 a channel
          gl_FragColor = vec4(toLinear(c) * power, 1.0);
        }`,
    });
    this.mesh = new THREE.Mesh(new THREE.CircleGeometry(1.0, 128), this.mat);
  }

  set power(v: number) { this.mat.uniforms.power.value = v; }

  /** Clears to black, draws, uploads. */
  draw(fn: (ctx: CanvasRenderingContext2D) => void) {
    const c = this.ctx;
    c.save();
    c.fillStyle = '#000';
    c.fillRect(0, 0, 240, 240);
    fn(c);
    c.restore();
    this.texture.needsUpdate = true;
  }
}
