// config.ts — TONE3000 API configuration
// T3K_API points to production. VITE_T3K_API_DOMAIN can override for development.
// Trailing slashes are stripped so `${T3K_API}/api/...` never produces a double slash —
// Vercel 308-redirects double-slash paths, and redirects drop CORS headers.
export const T3K_API = (
  'https://www.tone3000.com'
).replace(/\/+$/, '');

export const PUBLISHABLE_KEY = "t3k_pub_bHrH8btdwXXTtxz5ryEU8sNLF-2TGRT9";

// Per-demo keys — fall back to the shared PUBLISHABLE_KEY for local dev
export const PUBLISHABLE_KEY_SELECT = PUBLISHABLE_KEY;
export const PUBLISHABLE_KEY_LOAD = PUBLISHABLE_KEY;
export const PUBLISHABLE_KEY_FULL = PUBLISHABLE_KEY;

