// api/stats.js
const { sql } = require('./_db');
module.exports = async function handler(req, res) {
  res.setHeader('Access-Control-Allow-Origin', '*');
  try {
    const [row] = await sql`SELECT COUNT(*) as total, COALESCE(SUM(total_downloads),0) as dl FROM packages`;
    res.status(200).json({
      registry: 'OPI — Omnikarai Package Index', version: '2.1.0', status: 'live',
      total_packages: Number(row.total), total_downloads: Number(row.dl),
      url: 'https://opi-nine.vercel.app',
      endpoints: {
        list:    'GET  /api/packages',
        search:  'GET  /api/packages?q=<query>',
        info:    'GET  /api/packages/:name',
        publish: 'POST /api/packages',
        stats:   'GET  /api/stats',
      }
    });
  } catch (e) {
    res.status(503).json({ status: 'error', message: e.message });
  }
};
