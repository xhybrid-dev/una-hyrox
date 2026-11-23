// One week, in sync: a panel of seven days with the week's target as four pips. Each session
// leaves its app's icon as a tile, flies to its day and lands the moment that app's Saved screen
// shows the Streak line; the race's tile flies from the watch after Save.

import * as THREE from 'three';
import { WEEK, WEEK_BEATS, RACE_BEATS, FAMILY } from '../config';
import type { Frame } from '../app';
import { ease, span } from '../timeline';
import { makeText } from '../hud/type';
import { basic, lineMat, roundRectLine, roundRectMesh, white, yellow, arcPath, fade } from './util';
import type { Family } from './family';

const COL_W = 0.46, COL_GAP = 0.04, SLOT_H = 0.8;
const colX = (i: number) => -((7 * COL_W + 6 * COL_GAP) / 2) + COL_W / 2 + i * (COL_W + COL_GAP);

export class WeekStrip {
  readonly group = new THREE.Group();
  private frame = new THREE.Group();
  private pips: THREE.Mesh[] = [];
  private pipRings: THREE.Mesh[] = [];
  private tiles: THREE.Group[] = [];
  private tileGlow: THREE.LineLoop[] = [];
  /** Where each session lands, launches and lands (film time). */
  readonly flights: { launch: number; land: number }[];

  constructor(scene: THREE.Scene, icons: THREE.Texture[]) {
    scene.add(this.group);
    this.group.add(this.frame);
    // The panel.
    const back = roundRectMesh(3.75, 1.86, 0.08, basic('#0b0b0b', 0.72));
    back.position.z = -0.02;
    this.frame.add(back);
    back.renderOrder = 10;
    this.frame.add(roundRectLine(3.75, 1.86, 0.08, lineMat(white(0.55), 0.5)));
    const head = makeText({ text: 'THIS WEEK', size: 0.13, tracking: 0.12, color: '#ffffff', anchorY: 'middle' });
    head.position.set(-1.72, 0.66, 0);
    this.frame.add(head);
    const target = makeText({ text: `TARGET ${WEEK.target}`, size: 0.1, tracking: 0.12, color: '#8c8c8c', anchorX: 'right', anchorY: 'middle' });
    target.position.set(1.72 - 4 * 0.2 - 0.06, 0.66, 0);
    this.frame.add(target);
    const disc = new THREE.CircleGeometry(0.06, 32), ring = new THREE.RingGeometry(0.05, 0.065, 32);
    for (let i = 0; i < WEEK.target; i++) {
      const x = 1.72 - 0.06 - (WEEK.target - 1 - i) * 0.2;
      const r = new THREE.Mesh(ring, basic(white(0.6), 0.6));
      r.position.set(x, 0.66, 0);
      const p = new THREE.Mesh(disc, basic(yellow(1.9), 0, true));
      p.position.set(x, 0.66, 0.001);
      this.frame.add(r, p);
      this.pipRings.push(r);
      this.pips.push(p);
    }
    WEEK.days.forEach((d, i) => {
      const lab = makeText({ text: d, size: 0.1, tracking: 0.12, color: '#8c8c8c', anchorX: 'center', anchorY: 'middle' });
      lab.position.set(colX(i), 0.38, 0);
      this.frame.add(lab);
      const slot = roundRectLine(COL_W - 0.04, SLOT_H, 0.05, lineMat(white(0.5), 0.35));
      slot.position.set(colX(i), -0.17, 0);
      this.frame.add(slot);
    });

    this.frame.children.forEach((c) => { if (c !== back) c.renderOrder = 11; });

    // A tile for each session: its app's icon and its minutes.
    this.flights = WEEK.sessions.map((s, i) => {
      if (s.app === 'race') return { launch: RACE_BEATS.savePress - 0.05, land: RACE_BEATS.savedLine };
      const t0 = WEEK_BEATS.start + i * WEEK_BEATS.step;
      return { launch: t0 + 0.12, land: t0 + 0.7 };
    });
    WEEK.sessions.forEach((s) => {
      const g = new THREE.Group();
      const bg = roundRectMesh(COL_W - 0.08, SLOT_H - 0.06, 0.05, basic('#151515', 0.95));
      g.add(bg);
      const glow = roundRectLine(COL_W - 0.08, SLOT_H - 0.06, 0.05, lineMat(yellow(2.0), 1));
      g.add(glow);
      this.tileGlow.push(glow);
      const icon = new THREE.Mesh(new THREE.CircleGeometry(0.13, 48), new THREE.MeshBasicMaterial({ map: icons[FAMILY.findIndex((f) => f.id === s.app)], transparent: true, depthWrite: false }));
      icon.position.set(0, 0.17, 0.002);
      g.add(icon);
      const min = makeText({ text: String(s.minutes), size: 0.2, bold: true, color: '#ffffff', anchorX: 'center', anchorY: 'middle' });
      min.position.set(0, -0.08, 0.002);
      const unit = makeText({ text: 'MIN', size: 0.075, tracking: 0.12, color: '#8c8c8c', anchorX: 'center', anchorY: 'middle' });
      unit.position.set(0, -0.24, 0.002);
      g.add(min, unit);
      // Draw order inside a tile, whatever the camera: card, outline, icon, words.
      bg.renderOrder = 20; glow.renderOrder = 21; icon.renderOrder = 22; min.renderOrder = unit.renderOrder = 23;
      scene.add(g); // in world space: it flies from an icon to the panel
      this.tiles.push(g);
    });
  }

  /** The slot of session i, in world space. */
  slotWorld(i: number): THREE.Vector3 {
    return this.group.localToWorld(new THREE.Vector3(colX(WEEK.sessions[i].day), -0.17, 0.01));
  }

  /** The panel's top-left corner, in world space (for the "shared" callout). */
  corner(): THREE.Vector3 {
    return this.group.localToWorld(new THREE.Vector3(-1.87, 0.93, 0));
  }

  update(f: Frame, family: Family) {
    const { t, S, L, camera } = f;
    this.group.position.set(...L.aux);
    this.group.scale.setScalar(L.auxScale);
    this.group.updateMatrixWorld(true);
    // The panel draws on from its centre line.
    const a = S.week.a;
    this.frame.scale.set(1, 0.08 + 0.92 * ease('power3.out')(Math.min(1, a * 1.25)), 1);
    fade(this.frame, a);

    WEEK.sessions.forEach((s, i) => {
      const { launch, land } = this.flights[i];
      const g = this.tiles[i];
      const k = (t - launch) / (land - launch);
      if (t < launch || a <= 0.01) { g.visible = false; } else {
        g.visible = true;
        const start = s.app === 'race' ? new THREE.Vector3(1.55, 0.1, 0.5) : family.positionOf(s.app);
        const end = this.slotWorld(i);
        const e = ease('power3.inOut')(Math.min(1, k));
        g.position.copy(k >= 1 ? end : arcPath(start, end, e, 0.9));
        g.scale.setScalar(L.auxScale * (0.55 + 0.45 * e));
        // Facing the camera in flight, settling into the panel.
        const q = camera.quaternion.clone().slerp(this.group.getWorldQuaternion(new THREE.Quaternion()), e);
        g.quaternion.copy(q);
        fade(g, a * Math.min(1, k * 4));
        (this.tileGlow[i].material as THREE.LineBasicMaterial).opacity = a * (k < 1 ? 0.9 : Math.max(0.25, 1 - (t - land) * 1.2));
      }
      // The pip fills as the tile lands.
      const pk = span(t, land, land + 0.25, 'back.out(3)');
      this.pips[i].scale.setScalar(Math.max(0.001, pk));
      (this.pips[i].material as THREE.MeshBasicMaterial).opacity = a * (t >= land ? 1 : 0);
    });
  }
}
