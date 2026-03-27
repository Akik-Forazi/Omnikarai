// api/_db.js — Neon Postgres helper for Vercel serverless functions
let sql;
let dbError;

try {
  // Neon's Vercel integration may use POSTGRES_URL or DATABASE_URL
  const connStr = process.env.DATABASE_URL || process.env.POSTGRES_URL;
  if (!connStr) {
    throw new Error('No database connection string found. Set DATABASE_URL in Vercel project settings → Environment Variables.');
  }
  const { neon } = require('@neondatabase/serverless');
  sql = neon(connStr);
} catch (e) {
  dbError = e.message;
  // Return a proxy that throws a friendly 503 when any query is attempted
  sql = new Proxy({}, {
    get: () => () => {
      throw new Error(dbError);
    },
    apply: () => {
      throw new Error(dbError);
    }
  });
  // Make the proxy work as a tagged template literal too
  sql = function taggedSql() {
    throw new Error(dbError);
  };
  sql.unsafe = function() { throw new Error(dbError); };
}

module.exports = { sql, dbError };
