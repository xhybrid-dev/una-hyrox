// X Streak round the watch. The week's tiles fuse into one week, which climbs the last step of a
// wireframe Snowdon (the same switchbacks the watch draws) as the watch says "Week complete!";
// the counter goes 11 to 12, the summit bursts, and the ladder of five summits shows where the
// climb goes next. Then every app's line feeds one training load: fitness, fatigue and form.

import * as THREE from 'three';
import { WEEK, STREAK_BEATS } from '../config';
import type { Frame } from '../app';
import { ease, span } from '../timeline';
import { makeText } from '../hud/type';
import type { Text } from 'troika-three-text';
import { basic, lineMat, white, yellow, fade, rng } from './util';
import { CLIMBS } from '../screen/streak';
import type { Family } from './family';

const K = 0.023; // SummitGeometry units to world
const BASE = -0.95;

/** Snowdon (kDesigns[1]) in panel space. */
const P = (x: number, y: number) => new THREE.Vector3(x * K - 0.35, BASE - y * K, 0);

function trailPoints(): THREE.Vector3[] {
  // trailFor(): switchbacks at 64% and 32% across the peak between its foot and its top.
  const apex = [0, -84], left = [-86, 0], right = [86, 0];
  const yFoot = -5, yTop = apex[1] + 9, h = yFoot - yTop;
  const legs = 4;
  const edge = (base: number[], y: number) => apex[0] + ((base[0] - apex[0]) * (y - apex[1])) / (base[1] - apex[1]);
  const pts: THREE.Vector3[] = [];
  for (let j = 0; j < legs; j++) {
    const y = yFoot - (h * j) / legs;
    const xl = edge(left, y), xr = edge(right, y);
    pts.push(P(xl + (xr - xl) * (j % 2 === 0 ? 0.64 : 0.32), y));
  }
  pts.push(P(apex[0], yTop));
  return pts;
}

function along(pts: THREE.Vector3[], s: number): THREE.Vector3 {
  const lens = [0];
  for (let i = 1; i < pts.length; i++) lens.push(lens[i - 1] + pts[i].distanceTo(pts[i - 1]));
  const target = s * lens[lens.length - 1];
  for (let i = 1; i < pts.length; i++) {
    if (target <= lens[i]) return pts[i - 1].clone().lerp(pts[i], (target - lens[i - 1]) / (lens[i] - lens[i - 1] || 1));
  }
  return pts[pts.length - 1].clone();
}

export class StreakWorld {
  readonly group = new THREE.Group();
  private climb = new THREE.Group();
  private trail: THREE.Line;
  private trailLit: THREE.Line;
  private steps: THREE.Mesh[] = [];
  private block: THREE.Mesh;
  private n11: Text;
  private n12: Text;
  private ladder: Text[] = [];
  private burst: THREE.LineSegments;
  private shields: THREE.Group;
  private chart = new THREE.Group();
  private fitness: THREE.Line;
  private fatigue: THREE.Line;
  private feeds: THREE.Line[] = [];
  private tp = trailPoints();

  constructor(scene: THREE.Scene) {
    scene.add(this.group);
    this.group.add(this.climb, this.chart);

    // The mountain in line art: far ranges, shoulders, the peak, its snow line.
    const seg: THREE.Vector3[] = [];
    const add = (...p: [number, number][]) => { for (let i = 1; i < p.length; i++) seg.push(P(...p[i - 1]), P(...p[i])); };
    add([-86, 0], [0, -84], [86, 0]);
    add([0, -84], [0, 0]);
    add([52, -46], [102, 0]);
    add([-54, -34], [-86, -13.6]);
    add([-86, 0], [118, 0]);
    add([-15.5, -68.9], [0, -64.3], [15.5, -68.9]);
    const mtn = new THREE.LineSegments(new THREE.BufferGeometry().setFromPoints(seg), lineMat(white(0.9), 0.55));
    this.climb.add(mtn);

    this.trail = new THREE.Line(new THREE.BufferGeometry().setFromPoints(this.tp), lineMat(white(0.7), 0.4));
    this.trailLit = new THREE.Line(new THREE.BufferGeometry().setFromPoints(this.tp.map((p) => p.clone())), lineMat(yellow(2.2), 1));
    this.climb.add(this.trail, this.trailLit);
    // Snowdon's eight steps (weeks 5 to 12): the first seven climbed before the race.
    const stepGeo = new THREE.BoxGeometry(0.075, 0.075, 0.075);
    for (let s = 1; s <= 8; s++) {
      const m = new THREE.Mesh(stepGeo, basic(yellow(1.3), 1, true));
      m.position.copy(along(this.tp, s / 8));
      m.rotation.set(0.6, 0.6, 0);
      this.climb.add(m);
      this.steps.push(m);
    }
    // The week that arrives: the week's four sessions as one block.
    this.block = new THREE.Mesh(new THREE.BoxGeometry(0.17, 0.17, 0.17), basic(yellow(1.7), 1, true));
    this.group.add(this.block);

    // The counter.
    const label = makeText({ text: 'WEEK STREAK', size: 0.12, tracking: 0.12, color: '#ffffff', anchorX: 'left', anchorY: 'top' });
    label.position.set(-2.1, 1.25, 0);
    this.n11 = makeText({ text: String(WEEK.streakBefore), size: 0.72, bold: true, color: '#ffffff', anchorX: 'left', anchorY: 'top' });
    this.n12 = makeText({ text: String(WEEK.streakBefore + 1), size: 0.72, bold: true, color: '#FADB5C', anchorX: 'left', anchorY: 'top' });
    for (const n of [this.n11, this.n12]) n.position.set(-2.12, 1.06, 0);
    this.climb.add(label, this.n11, this.n12);

    // The ladder of summits, top down.
    [...CLIMBS].reverse().forEach((c, i) => {
      const tx = makeText({ text: `${c.name.toUpperCase()}  ${c.summitAt}`, size: 0.11, tracking: 0.1, color: '#8c8c8c', anchorX: 'right', anchorY: 'middle' });
      tx.position.set(2.05, 1.2 - i * 0.22, 0);
      this.climb.add(tx);
      this.ladder.push(tx);
    });
    const rail = new THREE.Line(new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(2.15, 1.28, 0), new THREE.Vector3(2.15, 0.25, 0)]), lineMat(white(0.6), 0.4));
    this.climb.add(rail);

    // Two shields (Streak earns one every 4 weeks, holds 2), for the callout.
    this.shields = new THREE.Group();
    const sh = [new THREE.Vector3(-0.09, 0.1, 0), new THREE.Vector3(0.09, 0.1, 0), new THREE.Vector3(0.09, -0.02, 0), new THREE.Vector3(0, -0.12, 0), new THREE.Vector3(-0.09, -0.02, 0), new THREE.Vector3(-0.09, 0.1, 0)];
    for (const x of [0, 0.24]) {
      const s = new THREE.Line(new THREE.BufferGeometry().setFromPoints(sh), lineMat(white(1.0), 0.9));
      s.position.x = x;
      this.shields.add(s);
    }
    this.shields.position.set(-2.02, 1.62, 0);
    this.climb.add(this.shields);

    // The summit burst: seeded rays.
    const r = rng(12);
    const rays: THREE.Vector3[] = [];
    for (let i = 0; i < 28; i++) {
      const a = (i / 28) * Math.PI * 2 + r() * 0.2;
      const l = 0.5 + r() * 0.6;
      rays.push(new THREE.Vector3(Math.cos(a) * 0.12, Math.sin(a) * 0.12, 0), new THREE.Vector3(Math.cos(a) * l, Math.sin(a) * l, 0));
    }
    this.burst = new THREE.LineSegments(new THREE.BufferGeometry().setFromPoints(rays), lineMat(yellow(2.6), 1));
    this.burst.position.copy(along(this.tp, 1));
    this.climb.add(this.burst);

    // The training load chart: twelve weeks of fitness (smooth) and fatigue (sharper), illustrative.
    const fx = (i: number) => -1.7 + (i / 83) * 3.05;
    const fit: THREE.Vector3[] = [], fat: THREE.Vector3[] = [];
    for (let i = 0; i <= 83; i++) {
      const u = i / 83;
      // Fitness: slow (42 days). Fatigue: quick (7 days), up with each week's sessions, above
      // fitness after a big week: form below zero, Building.
      fit.push(new THREE.Vector3(fx(i), -0.72 + 0.95 * (1 - Math.exp(-u * 2.2)) / (1 - Math.exp(-2.2)), 0));
      const wk = (i % 7) / 7;
      fat.push(new THREE.Vector3(fx(i), -0.68 + 0.95 * u + 0.07 * Math.sin(wk * Math.PI * 2) + (u > 0.9 ? (u - 0.9) * 3.4 : 0), 0));
    }
    this.fitness = new THREE.Line(new THREE.BufferGeometry().setFromPoints(fit), lineMat(yellow(2.0), 1));
    this.fatigue = new THREE.Line(new THREE.BufferGeometry().setFromPoints(fat), lineMat(white(1.0), 0.85));
    const axis = new THREE.Line(new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(-1.7, -0.85, 0), new THREE.Vector3(1.6, -0.85, 0)]), lineMat(white(0.6), 0.35));
    this.chart.add(this.fitness, this.fatigue, axis);
    const endFit = fit[83].y, endFat = fat[83].y;
    for (const [txt, y, col] of [['FATIGUE', endFat + 0.04, '#ffffff'], ['FITNESS', endFit - 0.04, '#FADB5C'], ['FORM', (endFit + endFat) / 2, '#8c8c8c']] as const) {
      const tx = makeText({ text: txt, size: 0.13, tracking: 0.1, color: col, anchorX: 'left', anchorY: 'middle' });
      tx.position.set(txt === 'FORM' ? 1.56 : 1.45, y, 0);
      this.chart.add(tx);
    }
    // A bracket for form: the gap between fitness and fatigue.
    const br = new THREE.Line(new THREE.BufferGeometry().setFromPoints([
      new THREE.Vector3(1.42, endFit, 0), new THREE.Vector3(1.5, endFit, 0), new THREE.Vector3(1.5, endFat, 0), new THREE.Vector3(1.42, endFat, 0)]), lineMat(white(0.7), 0.6));
    this.chart.add(br);
    const head = makeText({ text: 'TRAINING LOAD · EVERY APP', size: 0.12, tracking: 0.12, color: '#ffffff', anchorY: 'top' });
    head.position.set(-1.7, 1.2, 0);
    this.chart.add(head);
    for (let i = 0; i < 7; i++) {
      const l = new THREE.Line(new THREE.BufferGeometry().setFromPoints(Array.from({ length: 24 }, () => new THREE.Vector3())), lineMat(yellow(1.6), 0.7));
      l.frustumCulled = false;
      scene.add(l);
      this.feeds.push(l);
    }
  }

  /** Where the shields are (the callout's anchor). */
  shieldAnchor(): THREE.Vector3 { return this.shields.localToWorld(new THREE.Vector3(0.36, 0.08, 0)); }

  update(f: Frame, family: Family) {
    const { t, S, L } = f;
    const b = STREAK_BEATS;
    this.group.position.set(...L.aux);
    this.group.scale.setScalar(L.auxScale);
    this.group.updateMatrixWorld(true);
    const a = S.streak.a;
    const load = S.streak.load;
    fade(this.climb, a * (1 - load));
    fade(this.chart, a * load);

    // The trail lit up to the climber; the eighth step lands with the week.
    const glide = ease('power2.inOut')(span(t, b.stepUp, b.stepUp + b.glide, 'none'));
    const reach = (7 + glide) / 8;
    const lit = this.trailLit.geometry as THREE.BufferGeometry;
    const pos = lit.attributes.position as THREE.BufferAttribute;
    const lens = [0];
    for (let i = 1; i < this.tp.length; i++) lens.push(lens[i - 1] + this.tp[i].distanceTo(this.tp[i - 1]));
    const total = lens[lens.length - 1];
    for (let i = 0; i < this.tp.length; i++) {
      const p = lens[i] / total <= reach ? this.tp[i] : along(this.tp, reach);
      pos.setXYZ(i, p.x, p.y, p.z);
    }
    pos.needsUpdate = true;
    this.steps.forEach((m, i) => {
      const on = i < 7 ? 1 : t >= b.stepUp + b.glide ? 1 : 0;
      m.scale.setScalar(on ? 1 + (i === 7 ? 0.6 * Math.exp(-(t - b.stepUp - b.glide) * 5) : 0) : 0.001);
    });

    // The week block: from the week panel's place to the summit, as the watch's climber glides.
    const bk = span(t, b.toast + 0.2, b.stepUp + b.glide, 'power3.inOut');
    const top = this.climb.localToWorld(along(this.tp, 1));
    const from = this.group.localToWorld(new THREE.Vector3(0, -0.2, 0.6));
    this.block.visible = a > 0.01 && t > b.toast + 0.2 && t < b.stepUp + b.glide + 0.05;
    this.block.position.copy(this.group.worldToLocal(from.lerp(top, bk)));
    this.block.rotation.set(t * 1.3, t * 1.7, 0);

    // The counter: 11, then 12 as the week completes.
    const c = span(t, b.stepUp, b.stepUp + 0.45, 'power3.out');
    this.n11.fillOpacity = a * (1 - load) * (1 - c);
    this.n11.position.y = 1.06 + 0.3 * c;
    this.n12.fillOpacity = a * (1 - load) * c;
    this.n12.position.y = 1.06 - 0.3 * (1 - c);
    // The ladder: Snowdon lights at the summit.
    const sum = span(t, b.summit, b.summit + 0.4);
    this.ladder.forEach((tx, i) => {
      const isSnowdon = i === 3;
      (tx.color as THREE.Color).set(isSnowdon && sum > 0.5 ? '#FADB5C' : '#8c8c8c');
    });
    // The burst at the top.
    const bt = t - (b.stepUp + b.glide);
    this.burst.visible = a > 0.01 && bt > 0 && bt < 0.9;
    this.burst.scale.setScalar(0.3 + 1.2 * ease('power3.out')(Math.min(1, bt / 0.9)));
    (this.burst.material as THREE.LineBasicMaterial).opacity = Math.max(0, 1 - bt / 0.9);
    // Shields: shown from the summit, for the callout.
    fade(this.shields, a * (1 - load) * span(t, b.summit + 0.6, b.summit + 1.0));

    // Training load: the curves draw on, fed by a line from every app.
    const draw = span(t, b.load + 0.2, b.load + 1.6, 'power2.inOut');
    this.fitness.geometry.setDrawRange(0, Math.floor(84 * draw));
    this.fatigue.geometry.setDrawRange(0, Math.floor(84 * draw));
    const end = this.chart.localToWorld(new THREE.Vector3(-1.7, -0.75, 0));
    const feed = span(t, b.load, b.load + 0.9, 'power2.out');
    this.feeds.forEach((l, i) => {
      const v = a * load * S.family.a > 0.01 && feed > 0;
      l.visible = v;
      if (!v) return;
      const s = family.icons[i].position;
      const p = l.geometry.attributes.position as THREE.BufferAttribute;
      for (let j = 0; j < 24; j++) {
        const u = (j / 23) * feed;
        const q = s.clone().lerp(end, u);
        q.z += Math.sin(u * Math.PI) * 0.6;
        p.setXYZ(j, q.x, q.y, q.z);
      }
      p.needsUpdate = true;
      (l.material as THREE.LineBasicMaterial).opacity = 0.55 * load * (1 - span(t, b.load + 1.4, b.load + 2.4));
    });
  }
}
