// config.ts — TONE3000 API configuration
// T3K_API points to production. VITE_T3K_API_DOMAIN can override for development.
// Trailing slashes are stripped so `${T3K_API}/api/...` never produces a double slash —
// Vercel 308-redirects double-slash paths, and redirects drop CORS headers.
export const T3K_API = (
  (import.meta.env.VITE_T3K_API_DOMAIN as string | undefined) ?? 'https://www.tone3000.com'
).replace(/\/+$/, '');

export const PUBLISHABLE_KEY = import.meta.env.VITE_PUBLISHABLE_KEY as string;

// Per-demo keys — fall back to the shared PUBLISHABLE_KEY for local dev
export const PUBLISHABLE_KEY_SELECT =
  (import.meta.env.VITE_PUBLISHABLE_KEY_SELECT as string | undefined) ?? PUBLISHABLE_KEY;
export const PUBLISHABLE_KEY_LOAD =
  (import.meta.env.VITE_PUBLISHABLE_KEY_LOAD as string | undefined) ?? PUBLISHABLE_KEY;
export const PUBLISHABLE_KEY_FULL =
  (import.meta.env.VITE_PUBLISHABLE_KEY_FULL as string | undefined) ?? PUBLISHABLE_KEY;

export const REDIRECT_URI =
  (import.meta.env.VITE_REDIRECT_URI as string | undefined) ?? 'http://localhost:3001';