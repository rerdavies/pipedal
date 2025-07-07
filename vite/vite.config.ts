import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import svgr from "vite-plugin-svgr"

// https://vite.dev/config/
export default defineConfig({
  build: {
    chunkSizeWarningLimit: 2000
  },
  plugins: [react(),svgr()],
  server: {
    proxy: {
      '/resources': {
        target: 'http://localhost:8080',
        changeOrigin: false,
      },
    }
}
})
