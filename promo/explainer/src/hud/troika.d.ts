// The parts of troika-three-text the explainer uses (the package ships no types).
declare module 'troika-three-text' {
  import type { Mesh, Color, Material } from 'three';
  export class Text extends Mesh {
    text: string;
    font: string | null;
    fontSize: number;
    letterSpacing: number;
    lineHeight: number | 'normal';
    maxWidth: number;
    anchorX: number | 'left' | 'center' | 'right' | string;
    anchorY: number | 'top' | 'top-baseline' | 'middle' | 'bottom-baseline' | 'bottom' | string;
    textAlign: 'left' | 'right' | 'center' | 'justify';
    color: string | number | Color;
    fillOpacity: number;
    depthOffset: number;
    material: Material;
    textRenderInfo: { blockBounds: [number, number, number, number] } | null;
    sync(cb?: () => void): void;
    dispose(): void;
  }
  export function preloadFont(opts: { font?: string; characters?: string }, cb: () => void): void;
}
