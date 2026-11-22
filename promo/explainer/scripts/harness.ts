// Shared by the export and the screenshot scripts: a Vite server and a Chromium page on the film in
// export mode, at the delivery size, with a call to draw any time and read the frame back.
//
// GPU: Chromium uses the machine's GPU where it has one. Without one (a cloud container), WebGL
// runs on SwiftShader: slower, the same pixels. EXPLAINER_GL=swiftshader forces it.

import { chromium, type Browser, type Page } from 'playwright';
import { createServer, type ViteDevServer } from 'vite';
import path from 'node:path';
import { RATIOS, type Ratio } from '../src/config';

export const root = path.resolve(import.meta.dirname, '..');

export type Film = { page: Page; draw: (t: number) => Promise<Buffer>; close: () => Promise<void>; duration: number };

export async function openFilm(ratio: Ratio, port = 5180 + Object.keys(RATIOS).indexOf(ratio)): Promise<Film> {
  const server: ViteDevServer = await createServer({ root, logLevel: 'error', server: { port, strictPort: false } });
  await server.listen();
  const url = server.resolvedUrls?.local[0] ?? `http://localhost:${port}/`;
  const soft = process.env.EXPLAINER_GL === 'swiftshader' || process.platform === 'linux';
  const browser: Browser = await chromium.launch({
    args: soft
      ? ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist']
      : ['--ignore-gpu-blocklist', '--enable-gpu-rasterization'],
  });
  const { w, h } = RATIOS[ratio];
  const page = await browser.newPage({ viewport: { width: w, height: h }, deviceScaleFactor: 1 });
  page.on('pageerror', (e) => console.error('page error:', e.message));
  page.on('console', (m) => { if (m.type() === 'error') console.error('console:', m.text()); });
  await page.goto(`${url}?export=1&ratio=${ratio}`);
  await page.waitForFunction(() => window.__explainer?.ready, null, { timeout: 120000 });
  const duration = await page.evaluate(() => window.__explainer!.duration);
  return {
    page,
    duration,
    async draw(t: number) {
      const b64 = await page.evaluate(async (tt) => {
        await window.__explainer!.renderAt(tt);
        const c = document.getElementById('film') as HTMLCanvasElement;
        return c.toDataURL('image/png').split(',')[1];
      }, t);
      return Buffer.from(b64, 'base64');
    },
    async close() {
      await browser.close();
      await server.close();
    },
  };
}
