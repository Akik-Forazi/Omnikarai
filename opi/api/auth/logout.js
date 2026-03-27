// api/auth/logout.js — POST /api/auth/logout
module.exports = function handler(req, res) {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Set-Cookie', 'opi_token=; Path=/; Max-Age=0; HttpOnly; Secure; SameSite=Lax');
  return res.status(200).json({ ok: true });
};
