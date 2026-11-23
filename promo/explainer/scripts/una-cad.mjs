// UNA's watch housing from UNA's own CAD (github.com/UNAWatch/una-hardware, una-watch/mechanical,
// CC BY 4.0): the bezel, the housing, the four buttons, the rear cover and the sensor window,
// tessellated with OpenCascade (occt-import-js) and written as one GLB, public/gen/una/watch-cad.glb.
// The battery compartment, the antenna and the threaded inserts are inside, so they are left out.
//
// Axes as the film's watch: x towards 3 o'clock, y towards 12 o'clock, z out of the face; the origin
// at the display's centre, z = 0 at the bezel's top. Units: metres (glTF's), so 0.001 is 1 mm.
// The CAD's own axes: x towards 3 o'clock, y out of the face (0 at the bezel's top), z towards 6.
//
//   node scripts/una-cad.mjs            (UNA_HARDWARE, else a shallow clone into .cache/una-hardware)
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import occtimportjs from 'occt-import-js';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.join(here, '..');
const out = path.join(root, 'public', 'gen', 'una', 'watch-cad.glb');
let hw = process.env.UNA_HARDWARE;
if (!hw) {
  hw = path.join(root, '.cache', 'una-hardware');
  if (!fs.existsSync(path.join(hw, 'una-watch'))) {
    fs.mkdirSync(path.dirname(hw), { recursive: true });
    execFileSync('git', ['clone', '--depth', '1', 'https://github.com/UNAWatch/una-hardware', hw], { stdio: 'inherit' });
  }
}
const dir = path.join(hw, 'una-watch', 'mechanical');
const occt = await occtimportjs();
const OFFSET_Z = -0.2; // mm: the CAD's centre is 0.2 mm off the display's (fitted to UNA's render)
// The CAD matches UNA's render to within its outline at 15.35 mm per display radius (a 30.7 mm
// display), fitted by overlap (PLAN.md section 2). The four buttons: TouchGFX degrees and the outer
// face's radius (mm), from UNA's front render; the depth of the housing's button bosses (mm under
// the bezel's top), from the housing itself.
const BUTTONS = { l1: [299.25, 24.87], l2: [239.5, 25.21], r1: [60.5, 24.79], r2: [120.5, 25.15] };
const BUTTON_DEPTH = -6.5;

function read(file, deflection = 0.02) {
  const r = occt.ReadStepFile(new Uint8Array(fs.readFileSync(path.join(dir, file))), {
    linearUnit: 'millimeter', linearDeflectionType: 'absolute_value', linearDeflection: deflection, angularDeflection: 0.2,
  });
  if (!r.success) throw new Error(`could not read ${file}`);
  return r.meshes;
}

/** CAD (mm) to the film's axes (m), with an optional placement first (a 4x3 row-major matrix in mm). */
function convert(mesh, place) {
  const p = mesh.attributes.position.array, n = mesh.attributes.normal.array;
  const P = new Float32Array(p.length), N = new Float32Array(n.length);
  for (let i = 0; i < p.length; i += 3) {
    let [x, y, z] = [p[i], p[i + 1], p[i + 2]];
    let [nx, ny, nz] = [n[i], n[i + 1], n[i + 2]];
    if (place) {
      [x, y, z] = place.pos(x, y, z);
      [nx, ny, nz] = place.dir(nx, ny, nz);
    }
    P[i] = x / 1000; P[i + 1] = -(z - OFFSET_Z) / 1000; P[i + 2] = y / 1000;
    N[i] = nx; N[i + 1] = -nz; N[i + 2] = ny;
  }
  return { position: P, normal: N, index: Uint32Array.from(mesh.index.array) };
}

const parts = [];
const add = (name, mesh, place) => parts.push({ name, ...convert(mesh, place) });
for (const m of read('Bezel_501-0016_Rev-2.stp')) add('bezel', m);
for (const m of read('Housing_501-0015_Rev-2.stp')) if (!m.name.startsWith('604-')) add('housing', m); // 604-0001: threaded inserts, inside
for (const m of read('Rear_Cover_501-0019_Rev-1.stp', 0.04)) add('rear-cover', m);
for (const m of read('Rear_Cover_Dome_501-0023_Rev-1.stp')) add(m.name.endsWith('SKIN') ? 'sensor-window' : 'sensor-dome', m);
const button = read('Button_400-0015_Rev-3.stp');
for (const [k, [deg, rOut]] of Object.entries(BUTTONS)) {
  // The button's own axes: x out of the case (its face at x = 0.7), y along the edge, z the case's depth.
  // In CAD space the face's direction at TouchGFX angle a is (sin a, 0, -cos a) (z runs to 6 o'clock).
  const a = (deg * Math.PI) / 180;
  const u = [Math.sin(a), 0, -Math.cos(a)];
  const t = [-Math.cos(a), 0, -Math.sin(a)]; // u, t and the depth axis as a rotation, not a mirror
  const shift = rOut - 0.7;
  const place = {
    pos: (x, y, z) => [u[0] * (x + shift) + t[0] * y, BUTTON_DEPTH + z, u[2] * (x + shift) + t[2] * y],
    dir: (x, y, z) => [u[0] * x + t[0] * y, z, u[2] * x + t[2] * y],
  };
  for (const m of button) add(m.name === '501-0022' ? `button-${k}` : `button-${k}-stem`, m, place);
}

// GLB: one buffer, one mesh and node per part.
const views = [], accessors = [], meshes = [], nodes = [];
let bin = [];
let offset = 0;
const pushView = (arr, target) => {
  const bytes = Buffer.from(arr.buffer, arr.byteOffset, arr.byteLength);
  const pad = (4 - (bytes.length % 4)) % 4;
  views.push({ buffer: 0, byteOffset: offset, byteLength: bytes.length, target });
  bin.push(bytes, Buffer.alloc(pad));
  offset += bytes.length + pad;
  return views.length - 1;
};
for (const p of parts) {
  const lo = [Infinity, Infinity, Infinity], hi = [-Infinity, -Infinity, -Infinity];
  for (let i = 0; i < p.position.length; i += 3) for (let k = 0; k < 3; k++) { lo[k] = Math.min(lo[k], p.position[i + k]); hi[k] = Math.max(hi[k], p.position[i + k]); }
  const vp = pushView(p.position, 34962), vn = pushView(p.normal, 34962), vi = pushView(p.index, 34963);
  const n = p.position.length / 3;
  accessors.push({ bufferView: vp, componentType: 5126, count: n, type: 'VEC3', min: lo, max: hi });
  accessors.push({ bufferView: vn, componentType: 5126, count: n, type: 'VEC3' });
  accessors.push({ bufferView: vi, componentType: 5125, count: p.index.length, type: 'SCALAR' });
  const a = accessors.length - 3;
  meshes.push({ name: p.name, primitives: [{ attributes: { POSITION: a, NORMAL: a + 1 }, indices: a + 2 }] });
  nodes.push({ name: p.name, mesh: meshes.length - 1 });
}
const json = {
  asset: { version: '2.0', generator: 'una-cad.mjs', copyright: 'UNA Watch housing: UNA, github.com/UNAWatch/una-hardware, CC BY 4.0' },
  scene: 0, scenes: [{ name: 'una-watch', nodes: nodes.map((_, i) => i) }], nodes, meshes, accessors, bufferViews: views,
  buffers: [{ byteLength: offset }],
};
const binBuf = Buffer.concat(bin);
let jsonBuf = Buffer.from(JSON.stringify(json));
jsonBuf = Buffer.concat([jsonBuf, Buffer.alloc((4 - (jsonBuf.length % 4)) % 4, 0x20)]);
const header = Buffer.alloc(12);
header.writeUInt32LE(0x46546c67, 0); header.writeUInt32LE(2, 4); header.writeUInt32LE(12 + 8 + jsonBuf.length + 8 + binBuf.length, 8);
const ch = (len, type) => { const b = Buffer.alloc(8); b.writeUInt32LE(len, 0); b.writeUInt32LE(type, 4); return b; };
fs.mkdirSync(path.dirname(out), { recursive: true });
fs.writeFileSync(out, Buffer.concat([header, ch(jsonBuf.length, 0x4e4f534a), jsonBuf, ch(binBuf.length, 0x004e4942), binBuf]));
const tris = parts.reduce((s, p) => s + p.index.length / 3, 0);
console.log(`una-cad: ${parts.length} parts, ${tris} triangles, ${(fs.statSync(out).size / 1e6).toFixed(1)} MB → ${out}`);
