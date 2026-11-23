import { defineConfig } from 'vite';

// Relative base, so the built site can sit in any folder on hybridx.club.
export default defineConfig({
  base: './',
  server: { host: true, port: 5173 },
  build: { target: 'es2022', chunkSizeWarningLimit: 2000 },
});
