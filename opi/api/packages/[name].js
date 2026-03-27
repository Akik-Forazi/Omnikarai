// api/packages/[name].js — GET/DELETE /api/packages/:name
const { sql } = require('../_db');

function cors(res) {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET,DELETE,PATCH,OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type,Authorization');
}

module.exports = async function handler(req, res) {
  cors(res);
  if (req.method === 'OPTIONS') return res.status(200).end();
  // Vercel populates req.query.name from the [name] filename, but also
  // falls back to extracting it from the URL path for rewrite-based routing
  const name = req.query.name || req.url.split('/').filter(Boolean).pop()?.split('?')[0];

  if (req.method === 'GET') {
    const rows = await sql`SELECT * FROM packages WHERE name = ${name} LIMIT 1`;
    if (!rows.length) return res.status(404).json({ error: `Package '${name}' not found` });
    const meta = rows[0];
    const vv   = await sql`SELECT version FROM package_versions WHERE name = ${name} ORDER BY published_at ASC`;
    const vParam = req.query.v || meta.latest;
    const ver  = await sql`SELECT * FROM package_versions WHERE name = ${name} AND version = ${vParam} LIMIT 1`;
    // increment downloads async
    sql`UPDATE packages SET total_downloads = total_downloads + 1 WHERE name = ${name}`.catch(()=>{});
    return res.status(200).json({
      ...meta,
      keywords: meta.keywords || [],
      total_downloads: Number(meta.total_downloads) || 0,
      versions: vv.map(r => r.version),
      source: ver[0]?.source || '',
      readme: ver[0]?.readme || '',
      latest_release: ver[0] ? {
        ...ver[0],
        keywords: ver[0].keywords || [],
        dependencies:     ver[0].dependencies     || {},
        dev_dependencies: ver[0].dev_dependencies || {},
        source: ver[0].source || '',
      } : null,
    });
  }

  if (req.method === 'DELETE') {
    const auth = req.headers.authorization;
    if (!auth?.startsWith('Bearer ')) return res.status(401).json({ error: 'Unauthorized' });
    const token = auth.slice(7);
    const trows = await sql`SELECT username FROM api_tokens WHERE token = ${token} LIMIT 1`;
    if (!trows.length) return res.status(401).json({ error: 'Invalid token' });
    const meta = await sql`SELECT owner FROM packages WHERE name = ${name} LIMIT 1`;
    if (!meta.length) return res.status(404).json({ error: 'Not found' });
    if (meta[0].owner !== trows[0].username)
      return res.status(403).json({ error: 'Only the owner can delete this package' });
    await sql`DELETE FROM package_versions WHERE name = ${name}`;
    await sql`DELETE FROM packages WHERE name = ${name}`;
    return res.status(200).json({ success: true });
  }

  return res.status(405).json({ error: 'Method not allowed' });
};
