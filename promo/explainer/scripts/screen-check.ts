// Renders check.html (every screen beside its simulator capture, quantised, with the difference)
// and saves it as out/screen-check.png, printing each screen's agreement.
//
//   npm run screen-check            (starts its own Vite server)

import { chromium } from 'playwright';
import { createServer } from 'vite';
import fs from 'node:fs';
import path from 'node:path';

const root = path.resolve(import.meta.dirname, '..');
const server = await createServer({ root, logLevel: 'error', server: { port: 5199, strictPort: true } });
await server.listen();
const browser = await chromium.launch();
try {
  const page = await browser.newPage({ viewport: { width: 1600, height: 1200 }, deviceScaleFactor: 1 });
  page.on('pageerror', (e) => console.error('page error:', e.message));
  await page.goto('http://localhost:5199/check.html');
  await page.waitForFunction(() => (window as unknown as { __check?: unknown }).__check, null, { timeout: 30000 });
  const scores = await page.evaluate(() => (window as unknown as { __check: Record<string, number> }).__check);
  fs.mkdirSync(path.join(root, 'out'), { recursive: true });
  const out = path.join(root, 'out', 'screen-check.png');
  await page.screenshot({ path: out, fullPage: true });
  for (const [k, v] of Object.entries(scores)) console.log(`${(v * 100).toFixed(1).padStart(5)}%  ${k}`);
  console.log(`screen-check: ${out}`);
} finally {
  await browser.close();
  await server.close();
}
