# OPI — Omnikarai Package Index

The official package registry for the [Omnikarai](https://github.com/fraziym/omnikarai) language.

**Live at:** https://opi-nine.vercel.app

---

## Stack

- **Next.js 14** App Router + TypeScript
- **Vercel KV** (Redis) for persistent storage
- **bcryptjs** + **jose** for auth (JWT, bcrypt)
- **Tailwind CSS** for styling
- Deployed on **Vercel**

## Deploying

```bash
cd opi
npm install
vercel --prod
```

Set these environment variables in your Vercel dashboard:

| Variable | Where to get it |
|---|---|
| `KV_REST_API_URL` | Vercel → Storage → KV → your store |
| `KV_REST_API_TOKEN` | Vercel → Storage → KV → your store |
| `JWT_SECRET` | Any random 64-char string |

## Project Structure

```
opi/
  src/
    app/
      api/
        auth/register/   POST — create account
        auth/login/      POST — sign in
        auth/logout/     POST — sign out
        auth/me/         GET  — current user
        packages/        GET (list/search), POST (publish)
        packages/[name]/ GET (info), DELETE, PATCH (yank)
        search/          GET — search
        stats/           GET — registry stats
        user/tokens/     GET/POST/DELETE — API tokens
        user/profile/    PATCH — update profile
      (pages)/
        search/          Package search page
        package/[name]/  Package detail page
        login/           Sign in
        register/        Create account
        dashboard/       User dashboard + API tokens
        user/[username]/ Public profile
        settings/        Account settings
        help/            Full documentation
      page.tsx           Homepage
      layout.tsx         Root layout
      globals.css        Tailwind + custom styles
      not-found.tsx      404 page
    lib/
      kv.ts              Vercel KV wrapper + key schema
      auth.ts            JWT, bcrypt, token helpers
      types.ts           TypeScript interfaces
      packages.ts        Package CRUD, search, seed data
    components/
      Navbar.tsx         Sticky navbar with auth
```

## API Reference

| Method | Endpoint | Auth | Description |
|--------|----------|------|-------------|
| GET | `/api/packages` | — | List / search packages |
| GET | `/api/packages/:name` | — | Package info |
| POST | `/api/packages` | ✓ | Publish package |
| DELETE | `/api/packages/:name` | ✓ owner | Delete package |
| PATCH | `/api/packages/:name` | ✓ owner | Yank version |
| GET | `/api/search?q=` | — | Search |
| GET | `/api/stats` | — | Registry stats |
| POST | `/api/auth/register` | — | Create account |
| POST | `/api/auth/login` | — | Sign in |
| GET | `/api/auth/me` | ✓ | Current user |
| GET/POST/DELETE | `/api/user/tokens` | ✓ | API tokens |
| PATCH | `/api/user/profile` | ✓ | Update profile |

## omnip CLI

```bash
# Build
gcc -O2 -o bin/omnip.exe omnip/src/omnip.c -lkernel32 -lwinhttp

# Set API token for publishing
set OPI_TOKEN=opi_your_token_here

# Commands
omnip install <package>   # install from OPI
omnip install .           # install local
omnip publish .           # publish to OPI
omnip search <query>      # search
omnip list                # installed packages
omnip info <package>      # package details
omnip update              # update all
omnip uninstall <package> # remove
omnip init                # create omnikarai.toml
```

---

*Fraziym Tech & AI · 2026*
