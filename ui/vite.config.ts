import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

const controlPlaneUrl = process.env.CONTROL_PLANE_URL ?? "http://127.0.0.1:8090";

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      "/api": controlPlaneUrl,
      "/health": controlPlaneUrl
    }
  }
});
