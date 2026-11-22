// The stage round the watch: near-black, a fine perspective grid that fades out, a soft contact
// shadow under the watch, RoomEnvironment reflections, a key light and a rim light.

import * as THREE from 'three';
import { RoomEnvironment } from 'three/examples/jsm/environments/RoomEnvironment.js';
import { HorizontalBlurShader } from 'three/examples/jsm/shaders/HorizontalBlurShader.js';
import { VerticalBlurShader } from 'three/examples/jsm/shaders/VerticalBlurShader.js';
import { BRAND } from '../config';

export function environment(renderer: THREE.WebGLRenderer, scene: THREE.Scene) {
  const pmrem = new THREE.PMREMGenerator(renderer);
  scene.environment = pmrem.fromScene(new RoomEnvironment(), 0.04).texture;
  pmrem.dispose();
  scene.background = new THREE.Color(BRAND.black);
}

export function lights(scene: THREE.Scene) {
  const key = new THREE.DirectionalLight(0xffffff, 1.2);
  key.position.set(-5, 6, 7);
  const rim = new THREE.DirectionalLight(0xffffff, 0);
  rim.position.set(6, 3, -6);
  const fill = new THREE.DirectionalLight(0xffffff, 0.35);
  fill.position.set(4, -2, 6);
  scene.add(key, rim, fill);
  return { key, rim, fill };
}

/** A large plane with a fine grid that fades with distance from the centre. */
export function grid(scene: THREE.Scene, floorY: number) {
  const mat = new THREE.ShaderMaterial({
    transparent: true,
    depthWrite: false,
    uniforms: { opacity: { value: 1 }, colour: { value: new THREE.Color(0xffffff) }, reach: { value: 16 } },
    vertexShader: /* glsl */ `
      varying vec3 vW;
      void main() { vec4 w = modelMatrix * vec4(position, 1.0); vW = w.xyz; gl_Position = projectionMatrix * viewMatrix * w; }`,
    fragmentShader: /* glsl */ `
      uniform float opacity; uniform vec3 colour; uniform float reach; varying vec3 vW;
      float lines(vec2 p, float step) {
        vec2 g = abs(fract(p / step - 0.5) - 0.5) / fwidth(p / step);
        return 1.0 - min(min(g.x, g.y), 1.0);
      }
      void main() {
        float d = length(vW.xz);
        float fade = 1.0 - smoothstep(reach * 0.25, reach, d);
        float a = lines(vW.xz, 0.5) * 0.06 + lines(vW.xz, 2.5) * 0.07;
        gl_FragColor = vec4(colour, a * fade * opacity);
      }`,
  });
  const mesh = new THREE.Mesh(new THREE.PlaneGeometry(80, 80), mat);
  mesh.rotation.x = -Math.PI / 2;
  mesh.position.y = floorY;
  mesh.renderOrder = -1;
  scene.add(mesh);
  return mat;
}

/**
 * Contact shadow (the three.js contact-shadow technique): the watch's depth from below, rendered
 * into a small target, blurred twice, and shown on a plane just above the grid.
 */
export class ContactShadow {
  readonly plane: THREE.Mesh;
  private rt: THREE.WebGLRenderTarget;
  private rtBlur: THREE.WebGLRenderTarget;
  private cam: THREE.OrthographicCamera;
  private depthMat: THREE.MeshDepthMaterial;
  private blurH: THREE.ShaderMaterial;
  private blurV: THREE.ShaderMaterial;
  private quad: THREE.Mesh;
  private quadScene = new THREE.Scene();
  private quadCam = new THREE.OrthographicCamera(-1, 1, 1, -1, 0, 1);

  constructor(scene: THREE.Scene, floorY: number, size = 9, height = 3.4, res = 512) {
    this.rt = new THREE.WebGLRenderTarget(res, res);
    this.rt.texture.generateMipmaps = false;
    this.rtBlur = new THREE.WebGLRenderTarget(res, res);
    this.rtBlur.texture.generateMipmaps = false;
    const mat = new THREE.MeshBasicMaterial({ map: this.rt.texture, transparent: true, opacity: 0.85, depthWrite: false });
    this.plane = new THREE.Mesh(new THREE.PlaneGeometry(size, size).rotateX(Math.PI / 2), mat);
    this.plane.scale.y = -1;
    this.plane.position.y = floorY + 0.01;
    this.plane.renderOrder = 0;
    scene.add(this.plane);
    this.cam = new THREE.OrthographicCamera(-size / 2, size / 2, size / 2, -size / 2, 0, height);
    this.cam.rotation.x = Math.PI / 2;
    this.cam.position.y = floorY;
    this.depthMat = new THREE.MeshDepthMaterial();
    this.depthMat.userData.darkness = { value: 1.4 };
    this.depthMat.onBeforeCompile = (s) => {
      s.uniforms.darkness = this.depthMat.userData.darkness;
      s.fragmentShader = `uniform float darkness;\n${s.fragmentShader.replace(
        'gl_FragColor = vec4( vec3( 1.0 - fragCoordZ ), opacity );',
        'gl_FragColor = vec4( vec3( 0.0 ), ( 1.0 - fragCoordZ ) * darkness );',
      )}`;
    };
    this.depthMat.depthTest = false;
    this.depthMat.depthWrite = false;
    this.blurH = new THREE.ShaderMaterial(HorizontalBlurShader);
    this.blurH.depthTest = false;
    this.blurV = new THREE.ShaderMaterial(VerticalBlurShader);
    this.blurV.depthTest = false;
    this.quad = new THREE.Mesh(new THREE.PlaneGeometry(2, 2));
    this.quadScene.add(this.quad);
  }

  private blur(renderer: THREE.WebGLRenderer, amount: number) {
    this.quad.material = this.blurH;
    this.blurH.uniforms.tDiffuse.value = this.rt.texture;
    this.blurH.uniforms.h.value = amount / 256;
    renderer.setRenderTarget(this.rtBlur);
    renderer.render(this.quadScene, this.quadCam);
    this.quad.material = this.blurV;
    this.blurV.uniforms.tDiffuse.value = this.rtBlur.texture;
    this.blurV.uniforms.v.value = amount / 256;
    renderer.setRenderTarget(this.rt);
    renderer.render(this.quadScene, this.quadCam);
  }

  /** Renders only `caster`'s layer from below. Call before the main render. */
  update(renderer: THREE.WebGLRenderer, scene: THREE.Scene, caster: THREE.Object3D) {
    const bg = scene.background;
    scene.background = null;
    const visible: [THREE.Object3D, boolean][] = [];
    scene.children.forEach((c) => {
      if (c !== caster) { visible.push([c, c.visible]); c.visible = false; }
    });
    scene.overrideMaterial = this.depthMat;
    const clear = renderer.getClearAlpha();
    renderer.setClearAlpha(0);
    renderer.setRenderTarget(this.rt);
    renderer.clear();
    renderer.render(scene, this.cam);
    scene.overrideMaterial = null;
    this.blur(renderer, 1.2);
    this.blur(renderer, 0.5);
    renderer.setRenderTarget(null);
    renderer.setClearAlpha(clear);
    visible.forEach(([c, v]) => (c.visible = v));
    scene.background = bg;
  }
}
