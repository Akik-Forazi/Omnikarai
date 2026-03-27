// api/migrate/index.js — GET /api/migrate
// Creates schema + seeds data. Safe to call multiple times (idempotent).
const { sql } = require('../_db');

module.exports = async function handler(req, res) {
  res.setHeader('Access-Control-Allow-Origin', '*');
  const secret = req.headers['x-migrate-secret'] || req.query.secret;
  if (process.env.MIGRATE_SECRET && secret !== process.env.MIGRATE_SECRET)
    return res.status(403).json({ error: 'Forbidden' });

  try {
    // Create tables using tagged template literals (Neon serverless compatible)
    await sql`CREATE TABLE IF NOT EXISTS users (
      id SERIAL PRIMARY KEY,
      username TEXT UNIQUE NOT NULL,
      email TEXT UNIQUE NOT NULL,
      password TEXT NOT NULL,
      display_name TEXT,
      bio TEXT DEFAULT '',
      website TEXT DEFAULT '',
      joined_at TIMESTAMPTZ DEFAULT NOW(),
      is_verified BOOLEAN DEFAULT FALSE,
      packages TEXT[] DEFAULT '{}'
    )`;

    await sql`CREATE TABLE IF NOT EXISTS api_tokens (
      id SERIAL PRIMARY KEY,
      token TEXT UNIQUE NOT NULL,
      username TEXT NOT NULL,
      label TEXT DEFAULT 'default',
      created_at TIMESTAMPTZ DEFAULT NOW()
    )`;

    await sql`CREATE TABLE IF NOT EXISTS packages (
      id SERIAL PRIMARY KEY,
      name TEXT UNIQUE NOT NULL,
      owner TEXT NOT NULL,
      description TEXT DEFAULT '',
      latest TEXT NOT NULL,
      license TEXT DEFAULT 'MIT',
      homepage TEXT,
      repository TEXT,
      keywords TEXT[] DEFAULT '{}',
      total_downloads BIGINT DEFAULT 0,
      created_at TIMESTAMPTZ DEFAULT NOW(),
      updated_at TIMESTAMPTZ DEFAULT NOW()
    )`;

    await sql`CREATE TABLE IF NOT EXISTS package_versions (
      id SERIAL PRIMARY KEY,
      name TEXT NOT NULL,
      version TEXT NOT NULL,
      description TEXT DEFAULT '',
      author TEXT DEFAULT '',
      license TEXT DEFAULT 'MIT',
      keywords TEXT[] DEFAULT '{}',
      dependencies JSONB DEFAULT '{}',
      dev_dependencies JSONB DEFAULT '{}',
      readme TEXT DEFAULT '',
      changelog TEXT DEFAULT '',
      source TEXT DEFAULT '',
      yanked BOOLEAN DEFAULT FALSE,
      published_at TIMESTAMPTZ DEFAULT NOW(),
      published_by TEXT NOT NULL,
      UNIQUE(name, version)
    )`;

    await sql`CREATE INDEX IF NOT EXISTS idx_pkg_updated ON packages(updated_at DESC)`;
    await sql`CREATE INDEX IF NOT EXISTS idx_pkg_owner ON packages(owner)`;
    await sql`CREATE INDEX IF NOT EXISTS idx_pkgver_name ON package_versions(name)`;
    await sql`CREATE INDEX IF NOT EXISTS idx_tok_username ON api_tokens(username)`;

    // Add source column to existing tables if missing (idempotent)
    await sql`ALTER TABLE package_versions ADD COLUMN IF NOT EXISTS source TEXT DEFAULT ''`;

    return res.status(200).json({ ok: true, message: 'Schema ready. Run /api/migrate/seed to insert sample packages.' });
  } catch (e) {
    return res.status(500).json({ error: e.message });
  }
};
