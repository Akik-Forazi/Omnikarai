/* =============================================================
   omnip v6.0.0 - Omnikarai Package Manager
   Fraziym Tech & AI | 2026  |  https://opi-nine.vercel.app

   Inspired by pip's wheel/RECORD model:
     - Packages stored as directory trees (not flat blobs)
     - RECORD file tracks every installed file (clean uninstall)
     - Source published as JSON file-map {"rel/path": "content"}
     - Recursive collect on publish, recursive restore on install

   Install: %LOCALAPPDATA%\Programs\omnikarai\site-packages\<n>\
   Data:    %LOCALAPPDATA%\Programs\omnikarai\omnip\
============================================================= */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define OMNIP_VERSION  "6.0.0"
#define OPI_HOST       L"opi-nine.vercel.app"
#define OPI_PORT       INTERNET_DEFAULT_HTTPS_PORT
#define SITE_PKGS      "Programs\\omnikarai\\site-packages"
#define OMNIP_DATA     "Programs\\omnikarai\\omnip"
#define INSTALLED_FILE "Programs\\omnikarai\\omnip\\installed.json"
#define TOKEN_FILE     "Programs\\omnikarai\\omnip\\token"
#define TOML_FILE      "omnikarai.toml"
#define MAX_BUF        (16*1024*1024)
#define PATHBUF        1024

#define GRN "\033[92m"
#define YLW "\033[93m"
#define CYN "\033[96m"
#define RED "\033[91m"
#define BLD "\033[1m"
#define DIM "\033[2m"
#define RST "\033[0m"

static void enable_ansi(void) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD m = 0; GetConsoleMode(h, &m);
    SetConsoleMode(h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

/* ── PATH HELPERS ──────────────────────────────────────────── */
static void get_localappdata(char *b, int n) {
    const char *p = getenv("LOCALAPPDATA");
    if (p && p[0]) { strncpy_s(b, n, p, n - 1); return; }
    const char *h = getenv("USERPROFILE");
    if (!h) h = "C:\\Users\\Default";
    snprintf(b, n, "%s\\AppData\\Local", h);
}
static void site_packages_dir(char *b, int n) {
    char la[PATHBUF]; get_localappdata(la, sizeof la);
    snprintf(b, n, "%s\\" SITE_PKGS, la);
}
static void module_dir(char *b, int n, const char *name) {
    char base[PATHBUF]; site_packages_dir(base, sizeof base);
    snprintf(b, n, "%s\\%s", base, name);
}
static void omnip_data_dir(char *b, int n) {
    char la[PATHBUF]; get_localappdata(la, sizeof la);
    snprintf(b, n, "%s\\" OMNIP_DATA, la);
}
static void installed_path_fn(char *b, int n) {
    char la[PATHBUF]; get_localappdata(la, sizeof la);
    snprintf(b, n, "%s\\" INSTALLED_FILE, la);
}
static void token_path_fn(char *b, int n) {
    char la[PATHBUF]; get_localappdata(la, sizeof la);
    snprintf(b, n, "%s\\" TOKEN_FILE, la);
}
static void mkdir_p(const char *path) {
    char tmp[PATHBUF]; strncpy_s(tmp, sizeof tmp, path, sizeof(tmp) - 1);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '\\' || *p == '/') { *p = 0; CreateDirectoryA(tmp, NULL); *p = '\\'; }
    }
    CreateDirectoryA(tmp, NULL);
}
static int path_exists(const char *p) {
    return GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES;
}
static int write_file_s(const char *path, const char *data, DWORD len) {
    char dir[PATHBUF]; strncpy_s(dir, sizeof dir, path, sizeof(dir)-1);
    char *last = strrchr(dir, '\\');
    if (last) { *last = 0; mkdir_p(dir); }
    HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return 0;
    DWORD wr = 0; WriteFile(f, data, len, &wr, NULL); CloseHandle(f);
    return (int)wr;
}
static char *read_file_s(const char *path) {
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return NULL;
    DWORD sz = GetFileSize(f, NULL);
    char *buf = (char *)malloc(sz + 2);
    if (!buf) { CloseHandle(f); return NULL; }
    DWORD rd = 0; ReadFile(f, buf, sz, &rd, NULL); buf[rd] = 0;
    CloseHandle(f); return buf;
}

/* ── TOKEN ─────────────────────────────────────────────────── */
static char *get_token(void) {
    const char *env = getenv("OPI_TOKEN");
    if (env && env[0]) return _strdup(env);
    char tp[PATHBUF]; token_path_fn(tp, sizeof tp);
    char *tok = read_file_s(tp);
    if (!tok) return _strdup("");
    char *p = tok;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    char *e = p + strlen(p) - 1;
    while (e > p && (*e == ' ' || *e == '\t' || *e == '\r' || *e == '\n')) *e-- = 0;
    char *out = _strdup(p); free(tok); return out;
}
static void save_token(const char *tok) {
    char dp[PATHBUF]; omnip_data_dir(dp, sizeof dp); mkdir_p(dp);
    char tp[PATHBUF]; token_path_fn(tp, sizeof tp);
    write_file_s(tp, tok, (DWORD)strlen(tok));
}

/* ── JSON HELPERS ──────────────────────────────────────────── */
static int json_str(const char *json, const char *key, char *out, int olen) {
    char pat[256]; snprintf(pat, sizeof pat, "\"%s\"", key);
    const char *p = strstr(json, pat); if (!p) return 0;
    p += strlen(pat);
    while (*p == ' ' || *p == '\t') p++;
    if (*p != ':') return 0;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '"') {
        p++; int i = 0;
        while (*p && i < olen - 2) {
            if (*p == '\\' && *(p+1)) { out[i++] = *p++; out[i++] = *p++; }
            else if (*p == '"') break;
            else out[i++] = *p++;
        }
        out[i] = 0; return 1;
    }
    int i = 0;
    while (*p && *p != ',' && *p != '}' && *p != '\n' && i < olen - 1) out[i++] = *p++;
    out[i] = 0;
    for (int j = i - 1; j >= 0 && (out[j] == ' ' || out[j] == '\r'); j--) out[j] = 0;
    return i > 0;
}
static long long json_int_val(const char *json, const char *key) {
    char v[32] = "0"; json_str(json, key, v, sizeof v); return atoll(v);
}
static int json_arr_raw(const char *json, const char *key, char *out, int olen) {
    char pat[256]; snprintf(pat, sizeof pat, "\"%s\"", key);
    const char *p = strstr(json, pat); if (!p) return 0;
    p += strlen(pat);
    while (*p && *p != '[') p++;
    if (*p != '[') return 0;
    int depth = 0, i = 0;
    do {
        if (*p == '[') depth++; else if (*p == ']') depth--;
        if (i < olen - 1) out[i++] = *p++; else p++;
    } while (*p && depth > 0);
    out[i] = 0; return 1;
}
static void json_escape(const char *src, char *dst, int dlen) {
    int o = 0;
    for (const char *s = src; *s && o < dlen - 2; s++) {
        if (*s == '"')       { if (o < dlen-3) { dst[o++]='\\'; dst[o++]='"';  } }
        else if (*s == '\\') { if (o < dlen-3) { dst[o++]='\\'; dst[o++]='\\'; } }
        else if (*s == '\n') { if (o < dlen-3) { dst[o++]='\\'; dst[o++]='n';  } }
        else if (*s == '\r') { /* skip */ }
        else if (*s == '\t') { if (o < dlen-3) { dst[o++]='\\'; dst[o++]='t';  } }
        else dst[o++] = *s;
    }
    dst[o] = 0;
}
static void json_unescape(char *s) {
    char *r = s, *w = s;
    while (*r) {
        if (*r == '\\' && *(r+1)) {
            r++;
            switch (*r) {
                case 'n':  *w++ = '\n'; break;
                case 't':  *w++ = '\t'; break;
                case 'r':  *w++ = '\r'; break;
                case '\\': *w++ = '\\'; break;
                case '"':  *w++ = '"';  break;
                default:   *w++ = '\\'; *w++ = *r; break;
            }
        } else { *w++ = *r; }
        r++;
    }
    *w = 0;
}

/* ── TOML READER ───────────────────────────────────────────── */
static int toml_get(const char *toml, const char *key, char *out, int olen) {
    const char *p = toml;
    while ((p = strstr(p, key))) {
        if (p > toml && *(p-1) != '\n') { p++; continue; }
        const char *k = p + strlen(key);
        while (*k == ' ' || *k == '\t') k++;
        if (*k != '=') { p++; continue; }
        k++;
        while (*k == ' ' || *k == '\t') k++;
        int i = 0;
        if (*k == '"') {
            k++;
            while (*k && *k != '"' && i < olen - 1) out[i++] = *k++;
        } else {
            while (*k && *k != '\n' && *k != '\r' && i < olen - 1) out[i++] = *k++;
            while (i > 0 && (out[i-1] == ' ' || out[i-1] == '\t')) i--;
        }
        out[i] = 0; return 1;
    }
    return 0;
}
/* ── HTTPS REQUEST ─────────────────────────────────────────── */
typedef struct { char *body; DWORD len; int status; } HttpResp;

static HttpResp http_req(const wchar_t *method, const wchar_t *path,
                         const char *body_utf8, const char *token) {
    HttpResp r = {NULL, 0, 0};
    HINTERNET ses = WinHttpOpen(L"omnip/6.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!ses) return r;
    HINTERNET con = WinHttpConnect(ses, OPI_HOST, OPI_PORT, 0);
    if (!con) { WinHttpCloseHandle(ses); return r; }
    HINTERNET req = WinHttpOpenRequest(con, method, path, NULL, WINHTTP_NO_REFERER,
                                       WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!req) { WinHttpCloseHandle(con); WinHttpCloseHandle(ses); return r; }
    wchar_t hdrs[512] = L"Content-Type: application/json\r\n";
    if (token && token[0]) {
        wchar_t th[256]; swprintf_s(th, 256, L"Authorization: Bearer %S\r\n", token);
        wcscat_s(hdrs, 512, th);
    }
    WinHttpAddRequestHeaders(req, hdrs, (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);
    DWORD blen = (DWORD)(body_utf8 ? strlen(body_utf8) : 0);
    if (!WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            (LPVOID)body_utf8, blen, blen, 0) ||
        !WinHttpReceiveResponse(req, NULL)) {
        WinHttpCloseHandle(req); WinHttpCloseHandle(con); WinHttpCloseHandle(ses); return r;
    }
    DWORD sc = 0, scl = sizeof sc;
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        NULL, &sc, &scl, NULL);
    r.status = (int)sc;
    char *buf = (char *)malloc(MAX_BUF);
    if (!buf) { WinHttpCloseHandle(req); WinHttpCloseHandle(con); WinHttpCloseHandle(ses); return r; }
    DWORD total = 0, avail = 0;
    while (WinHttpQueryDataAvailable(req, &avail) && avail > 0) {
        if (total + avail >= MAX_BUF) break;
        DWORD rd = 0; WinHttpReadData(req, buf + total, avail, &rd); total += rd;
    }
    buf[total] = 0; r.body = buf; r.len = total;
    WinHttpCloseHandle(req); WinHttpCloseHandle(con); WinHttpCloseHandle(ses);
    return r;
}
static void http_free(HttpResp *r) { if (r->body) { free(r->body); r->body = NULL; } }

/* ── INSTALLED REGISTRY ────────────────────────────────────── */
static char *load_installed(void) {
    char ip[PATHBUF]; installed_path_fn(ip, sizeof ip);
    char *d = read_file_s(ip);
    return d ? d : _strdup("[]");
}
static void save_installed(const char *json) {
    char dp[PATHBUF]; omnip_data_dir(dp, sizeof dp); mkdir_p(dp);
    char ip[PATHBUF]; installed_path_fn(ip, sizeof ip);
    write_file_s(ip, json, (DWORD)strlen(json));
}
static void installed_add(const char *name, const char *version,
                          const char *source, const char *record) {
    char *cur = load_installed();
    char *end = strrchr(cur, ']'); if (end) *end = 0;
    /* heap-allocate escape buffer to avoid stack overflow */
    int rlen = record ? (int)(strlen(record) * 2 + 4) : 4;
    char *e_record = (char *)malloc(rlen);
    if (!e_record) { free(cur); return; }
    json_escape(record ? record : "", e_record, rlen);
    char *newjson = (char *)malloc(strlen(cur) + strlen(e_record) + 512);
    if (!newjson) { free(cur); return; }
    sprintf(newjson, "%s%s{\"name\":\"%s\",\"version\":\"%s\","
            "\"source\":\"%s\",\"record\":\"%s\"}]",
            cur, (strlen(cur) > 1) ? "," : "",
            name, version, source, e_record);
    save_installed(newjson); free(newjson); free(e_record); free(cur);
}
static void installed_remove(const char *name) {
    char *cur = load_installed();
    char *out = (char *)malloc(strlen(cur) + 4);
    if (!out) { free(cur); return; }
    int olen = 0; out[olen++] = '[';
    const char *p = cur; int first = 1;
    while ((p = strstr(p, "{\"name\":\""))) {
        char ename[128] = ""; json_str(p, "name", ename, sizeof ename);
        if (strcmp(ename, name) == 0) { p += 9; continue; }
        const char *e = p; int d = 0;
        while (*e) {
            if (*e == '{') d++; else if (*e == '}') { d--; if (!d) { e++; break; } }
            e++;
        }
        if (!first) out[olen++] = ',';
        first = 0;
        int sz = (int)(e - p);
        memcpy(out + olen, p, sz); olen += sz;
        p = e;
    }
    out[olen++] = ']'; out[olen] = 0;
    save_installed(out); free(out); free(cur);
}
static int installed_find(const char *name, char *ver_out, int vlen,
                          char *record_out, int rlen) {
    char *cur = load_installed(); const char *p = cur; int found = 0;
    while ((p = strstr(p, "{\"name\":\""))) {
        char ename[128] = ""; json_str(p, "name", ename, sizeof ename);
        if (strcmp(ename, name) == 0) {
            if (ver_out)    json_str(p, "version", ver_out, vlen);
            if (record_out) { json_str(p, "record", record_out, rlen); json_unescape(record_out); }
            found = 1; break;
        }
        p += 9;
    }
    free(cur); return found;
}
/* ── FILE-MAP: pip-wheel-style {"rel/path":"content"} ─────── */
static void filemap_append(char **buf, size_t *cap, size_t *len,
                           const char *relpath, const char *content) {
    size_t rlen = strlen(relpath) * 2 + 4;
    size_t clen = strlen(content) * 2 + 4;
    size_t need = *len + rlen + clen + 8;
    if (need > *cap) {
        *cap = need * 2 + (1024 * 1024);
        *buf = (char *)realloc(*buf, *cap);
        if (!*buf) return;
    }
    char *ep = (char *)malloc(rlen); json_escape(relpath, ep, (int)rlen);
    char *ec = (char *)malloc(clen); json_escape(content, ec, (int)clen);
    int need_comma = (*len > 1);
    *len += snprintf(*buf + *len, *cap - *len, "%s\"%s\":\"%s\"",
                     need_comma ? "," : "", ep, ec);
    free(ep); free(ec);
}
static void filemap_collect(char **buf, size_t *cap, size_t *len,
                            const char *base_root, const char *cur_rel) {
    char search[PATHBUF];
    if (cur_rel[0]) snprintf(search, sizeof search, "%s\\%s\\*", base_root, cur_rel);
    else            snprintf(search, sizeof search, "%s\\*", base_root);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(search, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
        char rel[PATHBUF];
        if (cur_rel[0]) snprintf(rel, sizeof rel, "%s/%s", cur_rel, fd.cFileName);
        else            snprintf(rel, sizeof rel, "%s", fd.cFileName);
        char abs_path[PATHBUF];
        snprintf(abs_path, sizeof abs_path, "%s\\%s", base_root, rel);
        for (char *p = abs_path; *p; p++) if (*p == '/') *p = '\\';
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            filemap_collect(buf, cap, len, base_root, rel);
        } else {
            const char *ext = strrchr(fd.cFileName, '.');
            int include = 0;
            if (ext) {
                if (!_stricmp(ext, ".ok"))   include = 1;
                if (!_stricmp(ext, ".toml")) include = 1;
                if (!_stricmp(ext, ".md"))   include = 1;
                if (!_stricmp(ext, ".txt"))  include = 1;
            }
            if (!_stricmp(fd.cFileName, "LICENSE")) include = 1;
            if (!include) continue;
            char *content = read_file_s(abs_path);
            if (!content) continue;
            filemap_append(buf, cap, len, rel, content);
            free(content);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}
static void filemap_restore(const char *filemap, const char *install_dir,
                            char *record_buf, int record_cap) {
    record_buf[0] = 0;
    int rec_len = 0;
    const char *p = filemap;
    while (*p && *p != '{') p++;
    if (*p == '{') p++;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',') p++;
        if (*p == '}' || *p == '\0') break;
        if (*p != '"') { p++; continue; }
        char relpath[PATHBUF] = "";
        p++; int ki = 0;
        while (*p && ki < PATHBUF - 1) {
            if (*p == '\\' && *(p+1)) {
                p++;
                switch (*p) {
                    case 'n': relpath[ki++] = '\n'; break;
                    case 't': relpath[ki++] = '\t'; break;
                    case '\\': relpath[ki++] = '\\'; break;
                    case '"':  relpath[ki++] = '"';  break;
                    case '/':  relpath[ki++] = '/';  break;
                    default:   relpath[ki++] = '\\'; relpath[ki++] = *p; break;
                }
            } else if (*p == '"') { p++; break; }
            else relpath[ki++] = *p++;
        }
        relpath[ki] = 0;
        while (*p == ' ' || *p == '\t') p++;
        if (*p != ':') continue;
        p++;
        while (*p == ' ' || *p == '\t') p++;
        if (*p != '"') continue;
        p++;
        size_t vcap = 4096, vlen = 0;
        char *val = (char *)malloc(vcap);
        if (!val) continue;
        while (*p) {
            if (*p == '\\' && *(p+1)) {
                p++;
                char ch = 0;
                switch (*p) {
                    case 'n':  ch = '\n'; break;
                    case 't':  ch = '\t'; break;
                    case 'r':  ch = '\r'; break;
                    case '\\': ch = '\\'; break;
                    case '"':  ch = '"';  break;
                    default:   if (vlen+2 < vcap) { val[vlen++]='\\'; val[vlen++]=*p; } p++; continue;
                }
                if (vlen + 1 >= vcap) { vcap *= 2; val = (char *)realloc(val, vcap); }
                val[vlen++] = ch;
            } else if (*p == '"') { p++; break; }
            else {
                if (vlen + 1 >= vcap) { vcap *= 2; val = (char *)realloc(val, vcap); }
                val[vlen++] = *p++;
            }
        }
        val[vlen] = 0;
        char abs_path[PATHBUF];
        snprintf(abs_path, sizeof abs_path, "%s\\%s", install_dir, relpath);
        for (char *q = abs_path; *q; q++) if (*q == '/') *q = '\\';
        write_file_s(abs_path, val, (DWORD)vlen);
        free(val);
        if (rec_len + (int)strlen(relpath) + 2 < record_cap) {
            if (rec_len > 0) record_buf[rec_len++] = '\n';
            memcpy(record_buf + rec_len, relpath, strlen(relpath));
            rec_len += (int)strlen(relpath);
            record_buf[rec_len] = 0;
        }
    }
}
/* ── COMMANDS ──────────────────────────────────────────────── */
static int cmd_install_remote(const char *name, const char *ver_pin) {
    char path[256];
    if (ver_pin && ver_pin[0]) snprintf(path, sizeof path, "/api/packages/%s?v=%s", name, ver_pin);
    else                       snprintf(path, sizeof path, "/api/packages/%s", name);
    wchar_t wpath[512]; swprintf_s(wpath, 512, L"%S", path);
    char *tok = get_token();
    printf(DIM "  fetching %s from OPI...\n" RST, name);
    HttpResp r = http_req(L"GET", wpath, NULL, tok); free(tok);
    if (r.status != 200) {
        printf(RED "error:" RST " package '%s' not found on OPI (HTTP %d)\n", name, r.status);
        http_free(&r); return 1;
    }
    char version[64] = "unknown";
    json_str(r.body, "latest", version, sizeof version);
    if (ver_pin && ver_pin[0]) strncpy_s(version, sizeof version, ver_pin, sizeof(version)-1);
    char *filemap_raw = (char *)calloc(1, MAX_BUF);
    if (!filemap_raw) { http_free(&r); return 1; }
    json_str(r.body, "source", filemap_raw, MAX_BUF);
    /* json_unescape converts \\n -> real newline etc. in the filemap JSON.
       filemap_restore then handles the inner values correctly. */
    json_unescape(filemap_raw);
    http_free(&r);
    char mdir[PATHBUF]; module_dir(mdir, sizeof mdir, name); mkdir_p(mdir);
    char *record_buf = (char *)calloc(1, 64 * 1024);
    int file_count = 0;
    if (filemap_raw[0] == '{') {
        filemap_restore(filemap_raw, mdir, record_buf, 64 * 1024);
        for (char *p = record_buf; *p; p++) if (*p == '\n') file_count++;
        if (record_buf[0]) file_count++;
    } else {
        char sfile[PATHBUF]; snprintf(sfile, sizeof sfile, "%s\\%s.ok", mdir, name);
        write_file_s(sfile, filemap_raw, (DWORD)strlen(filemap_raw));
        snprintf(record_buf, 64 * 1024, "%s.ok", name);
        file_count = 1;
    }
    free(filemap_raw);
    char recpath[PATHBUF]; snprintf(recpath, sizeof recpath, "%s\\RECORD", mdir);
    write_file_s(recpath, record_buf, (DWORD)strlen(record_buf));
    char vfile[PATHBUF]; snprintf(vfile, sizeof vfile, "%s\\version.txt", mdir);
    write_file_s(vfile, version, (DWORD)strlen(version));
    installed_add(name, version, "opi", record_buf);
    free(record_buf);
    printf(GRN "  installed" RST " %s v%s (%d file%s)\n",
           name, version, file_count, file_count == 1 ? "" : "s");
    return 0;
}
static int cmd_install_local(void) {
    char toml_raw[16384] = "";
    char *raw = read_file_s(TOML_FILE);
    if (!raw) { printf(RED "error:" RST " no omnikarai.toml in current directory\n"); return 1; }
    strncpy_s(toml_raw, sizeof toml_raw, raw, sizeof(toml_raw)-1); free(raw);
    char name[128] = "", version[64] = "0.0.0";
    toml_get(toml_raw, "name", name, sizeof name);
    toml_get(toml_raw, "version", version, sizeof version);
    if (!name[0]) { printf(RED "error:" RST " omnikarai.toml missing 'name'\n"); return 1; }
    char mdir[PATHBUF]; module_dir(mdir, sizeof mdir, name); mkdir_p(mdir);
    char cwd[PATHBUF]; GetCurrentDirectoryA(sizeof cwd, cwd);
    size_t fmcap = 1024 * 1024, fmlen = 1;
    char *fmbuf = (char *)malloc(fmcap);
    if (!fmbuf) return 1;
    fmbuf[0] = '{';
    filemap_collect(&fmbuf, &fmcap, &fmlen, cwd, "");
    if (fmlen < fmcap - 1) { fmbuf[fmlen++] = '}'; fmbuf[fmlen] = 0; }
    char *record_buf = (char *)calloc(1, 64 * 1024);
    filemap_restore(fmbuf, mdir, record_buf, 64 * 1024);
    free(fmbuf);
    int file_count = 0;
    for (char *p = record_buf; *p; p++) if (*p == '\n') file_count++;
    if (record_buf[0]) file_count++;
    char recpath[PATHBUF]; snprintf(recpath, sizeof recpath, "%s\\RECORD", mdir);
    write_file_s(recpath, record_buf, (DWORD)strlen(record_buf));
    char vfile[PATHBUF]; snprintf(vfile, sizeof vfile, "%s\\version.txt", mdir);
    write_file_s(vfile, version, (DWORD)strlen(version));
    installed_add(name, version, "local", record_buf);
    free(record_buf);
    printf(GRN "  installed" RST " %s v%s (local, %d file%s)\n",
           name, version, file_count, file_count == 1 ? "" : "s");
    return 0;
}
static int cmd_install(const char *arg) {
    if (!arg) { printf(RED "error:" RST " usage: omnip install <package>|.\n"); return 1; }
    if (strcmp(arg, ".") == 0) return cmd_install_local();
    char name[128] = "", ver[64] = "";
    const char *at = strchr(arg, '@');
    if (at) {
        strncpy_s(name, sizeof name, arg, (size_t)(at - arg));
        strncpy_s(ver, sizeof ver, at + 1, sizeof(ver)-1);
    } else {
        strncpy_s(name, sizeof name, arg, sizeof(name)-1);
    }
    return cmd_install_remote(name, ver[0] ? ver : NULL);
}
static int cmd_uninstall(const char *name) {
    if (!name) { printf(RED "error:" RST " usage: omnip uninstall <package>\n"); return 1; }
    char ver[64] = "";
    char *record_buf = (char *)calloc(1, 64 * 1024);
    if (!installed_find(name, ver, sizeof ver, record_buf, 64 * 1024)) {
        printf(YLW "  warning:" RST " '%s' is not installed\n", name);
        free(record_buf); return 0;
    }
    char mdir[PATHBUF]; module_dir(mdir, sizeof mdir, name);
    int removed = 0;
    if (record_buf[0]) {
        char *line = record_buf;
        while (line && *line) {
            char *nl = strchr(line, '\n'); if (nl) *nl = 0;
            char fp[PATHBUF]; snprintf(fp, sizeof fp, "%s\\%s", mdir, line);
            for (char *p = fp; *p; p++) if (*p == '/') *p = '\\';
            if (DeleteFileA(fp)) removed++;
            line = nl ? nl + 1 : NULL;
        }
    } else {
        char pat[PATHBUF]; snprintf(pat, sizeof pat, "%s\\*", mdir);
        WIN32_FIND_DATAA fd; HANDLE h = FindFirstFileA(pat, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
                char fp[PATHBUF]; snprintf(fp, sizeof fp, "%s\\%s", mdir, fd.cFileName);
                if (DeleteFileA(fp)) removed++;
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
    }
    free(record_buf);
    char recpath[PATHBUF]; snprintf(recpath, sizeof recpath, "%s\\RECORD", mdir);
    DeleteFileA(recpath);
    char vfile[PATHBUF]; snprintf(vfile, sizeof vfile, "%s\\version.txt", mdir);
    DeleteFileA(vfile);
    RemoveDirectoryA(mdir);
    installed_remove(name);
    printf(GRN "  removed" RST " %s v%s (%d file%s)\n",
           name, ver, removed, removed == 1 ? "" : "s");
    return 0;
}
static int cmd_publish(void) {
    char toml[16384] = ""; char *raw = read_file_s(TOML_FILE);
    if (!raw) { printf(RED "error:" RST " no omnikarai.toml. Run 'omnip init' first.\n"); return 1; }
    strncpy_s(toml, sizeof toml, raw, sizeof(toml)-1); free(raw);
    char name[128]="", version[64]="", desc[512]="", author[128]="";
    char license[64]="MIT", homepage[256]="", repo[256]="";
    toml_get(toml, "name",        name,     sizeof name);
    toml_get(toml, "version",     version,  sizeof version);
    toml_get(toml, "description", desc,     sizeof desc);
    toml_get(toml, "author",      author,   sizeof author);
    toml_get(toml, "license",     license,  sizeof license);
    toml_get(toml, "homepage",    homepage, sizeof homepage);
    toml_get(toml, "repository",  repo,     sizeof repo);
    if (!name[0] || !version[0]) {
        printf(RED "error:" RST " omnikarai.toml must have 'name' and 'version'\n"); return 1;
    }
    for (char *c = name; *c; c++) {
        if (!(*c>='a'&&*c<='z') && !(*c>='0'&&*c<='9') && *c!='-' && *c!='_') {
            printf(RED "error:" RST " package name: lowercase letters/numbers/-/_ only\n"); return 1;
        }
    }
    char *tok = get_token();
    if (!tok[0]) {
        printf(RED "error:" RST " no API token. Run: omnip login <token>\n");
        printf(DIM "  get a token from https://opi-nine.vercel.app/dashboard\n" RST);
        free(tok); return 1;
    }
    char cwd[PATHBUF]; GetCurrentDirectoryA(sizeof cwd, cwd);
    size_t fmcap = 2 * 1024 * 1024, fmlen = 1;
    char *fmbuf = (char *)malloc(fmcap);
    if (!fmbuf) { free(tok); return 1; }
    fmbuf[0] = '{';
    filemap_collect(&fmbuf, &fmcap, &fmlen, cwd, "");
    if (fmlen < fmcap - 1) { fmbuf[fmlen++] = '}'; fmbuf[fmlen] = 0; }
    int fc = 0; { const char *p = fmbuf; while ((p = strstr(p, "\":\""))) { fc++; p += 3; } }
    printf("  " DIM "collecting %d file%s...\n" RST, fc, fc==1?"":"s");
    char e_name[256], e_ver[128], e_desc[1024], e_author[256];
    char e_lic[128], e_hp[512], e_repo[512], e_readme[16384];
    json_escape(name,    e_name,   sizeof e_name);
    json_escape(version, e_ver,    sizeof e_ver);
    json_escape(desc,    e_desc,   sizeof e_desc);
    json_escape(author,  e_author, sizeof e_author);
    json_escape(license, e_lic,    sizeof e_lic);
    json_escape(homepage,e_hp,     sizeof e_hp);
    json_escape(repo,    e_repo,   sizeof e_repo);
    char readme_raw[8192] = "";
    char *rmd = read_file_s("README.md");
    if (rmd) { strncpy_s(readme_raw, sizeof readme_raw, rmd, sizeof(readme_raw)-1); free(rmd); }
    json_escape(readme_raw, e_readme, sizeof e_readme);
    char *e_source = (char *)malloc(fmcap * 2 + 8);
    if (!e_source) { free(fmbuf); free(tok); return 1; }
    json_escape(fmbuf, e_source, (int)(fmcap * 2 + 8));
    free(fmbuf);
    size_t bodycap = strlen(e_source) + 8192;
    char *body = (char *)malloc(bodycap);
    if (!body) { free(e_source); free(tok); return 1; }
    snprintf(body, bodycap,
             "{\"name\":\"%s\",\"version\":\"%s\",\"description\":\"%s\","
             "\"author\":\"%s\",\"license\":\"%s\",\"homepage\":\"%s\","
             "\"repository\":\"%s\",\"keywords\":[],\"dependencies\":{},"
             "\"readme\":\"%s\",\"source\":\"%s\"}",
             e_name, e_ver, e_desc, e_author, e_lic, e_hp, e_repo, e_readme, e_source);
    free(e_source);
    printf("  publishing " BLD "%s" RST " v%s ...\n", name, version);
    HttpResp r = http_req(L"POST", L"/api/packages", body, tok);
    free(tok); free(body);
    if (r.status == 201) {
        printf(GRN "  published" RST " %s v%s\n", name, version);
        printf(DIM "  https://opi-nine.vercel.app/package/%s\n" RST, name);
    } else {
        char err[512] = "";
        json_str(r.body ? r.body : "{}", "error", err, sizeof err);
        if (!err[0] && r.body) strncpy_s(err, sizeof err, r.body, sizeof(err)-1);
        printf(RED "  publish failed" RST " (HTTP %d): %s\n",
               r.status, err[0] ? err : "no response");
    }
    http_free(&r); return 0;
}
static int cmd_list(void) {
    char *data = load_installed();
    if (!data || strcmp(data, "[]") == 0) {
        printf(DIM "  no packages installed\n" RST); free(data); return 0;
    }
    printf(BLD "  installed packages:\n" RST);
    const char *p = data; int count = 0;
    while ((p = strstr(p, "{\"name\":\""))) {
        char name[128]="", ver[64]="", src[32]="";
        json_str(p, "name", name, sizeof name);
        json_str(p, "version", ver, sizeof ver);
        json_str(p, "source", src, sizeof src);
        printf("  " CYN "%-20s" RST "  v%-12s  " DIM "%s" RST "\n", name, ver, src);
        count++; p += 9;
    }
    printf(DIM "  %d package%s\n" RST, count, count == 1 ? "" : "s");
    free(data); return 0;
}
static int cmd_info(const char *name) {
    if (!name) { printf(RED "error:" RST " usage: omnip info <package>\n"); return 1; }
    char ver[64] = "";
    if (installed_find(name, ver, sizeof ver, NULL, 0)) {
        printf(BLD "  %s" RST "  v%s  (installed)\n", name, ver);
        char mdir[PATHBUF]; module_dir(mdir, sizeof mdir, name);
        printf("  path:     %s\n", mdir);
        char recpath[PATHBUF]; snprintf(recpath, sizeof recpath, "%s\\RECORD", mdir);
        char *rec = read_file_s(recpath);
        if (rec) {
            printf("  files:\n");
            char *line = rec;
            while (line && *line) {
                char *nl = strchr(line, '\n'); if (nl) *nl = 0;
                printf("    " DIM "%s\n" RST, line);
                line = nl ? nl + 1 : NULL;
            }
            free(rec);
        }
    }
    wchar_t wpath[256]; swprintf_s(wpath, 256, L"/api/packages/%S", name);
    char *tok = get_token();
    HttpResp r = http_req(L"GET", wpath, NULL, tok); free(tok);
    if (r.status == 200) {
        char desc[512]="", latest[64]="", owner[64]="", lic[32]="";
        json_str(r.body, "description", desc, sizeof desc);
        json_str(r.body, "latest", latest, sizeof latest);
        json_str(r.body, "owner", owner, sizeof owner);
        json_str(r.body, "license", lic, sizeof lic);
        long long dl = json_int_val(r.body, "total_downloads");
        printf(BLD "\n  %s" RST "  v%s\n", name, latest);
        printf("  owner:    @%s\n  desc:     %s\n  license:  %s\n  downloads:%lld\n  install:  omnip install %s\n",
               owner, desc, lic, dl, name);
    } else if (r.status == 404) {
        if (!ver[0]) printf(RED "  '%s' not found\n" RST, name);
    } else {
        printf(YLW "  OPI unreachable (HTTP %d)\n" RST, r.status);
    }
    http_free(&r); return 0;
}
static int cmd_search(const char *query) {
    if (!query) { printf(RED "error:" RST " usage: omnip search <query>\n"); return 1; }
    wchar_t wpath[512]; swprintf_s(wpath, 512, L"/api/packages?q=%S&limit=20", query);
    char *tok = get_token();
    HttpResp r = http_req(L"GET", wpath, NULL, tok); free(tok);
    if (r.status != 200) {
        printf(RED "error:" RST " search failed (HTTP %d)\n", r.status);
        http_free(&r); return 1;
    }
    char *arr = (char *)calloc(1, MAX_BUF / 4);
    if (!arr) { http_free(&r); return 1; }
    if (!json_arr_raw(r.body, "packages", arr, MAX_BUF / 4)) {
        printf(DIM "  no results\n" RST); http_free(&r); free(arr); return 0;
    }
    int count = 0; const char *p = arr;
    while ((p = strstr(p, "{\"name\":\""))) {
        char pname[128]="", ver[64]="", desc[256]="";
        json_str(p, "name", pname, sizeof pname);
        json_str(p, "latest", ver, sizeof ver);
        json_str(p, "description", desc, sizeof desc);
        if (strlen(desc) > 60) { desc[57]='.'; desc[58]='.'; desc[59]='.'; desc[60]=0; }
        printf("  " CYN "%-20s" RST "  v%-10s  %s\n", pname, ver, desc);
        count++; p += 9;
    }
    free(arr); http_free(&r);
    if (!count) printf(DIM "  no results for '%s'\n" RST, query);
    else printf(DIM "\n  %d result%s\n" RST, count, count == 1 ? "" : "s");
    return 0;
}
static int cmd_update(const char *name) {
    char *data = load_installed();
    if (!data || strcmp(data, "[]") == 0) {
        printf(DIM "  no packages installed\n" RST); free(data); return 0;
    }
    int updated = 0; const char *p = data;
    while ((p = strstr(p, "{\"name\":\""))) {
        char ename[128]="", ever[64]="", esrc[32]="";
        json_str(p, "name", ename, sizeof ename);
        json_str(p, "version", ever, sizeof ever);
        json_str(p, "source", esrc, sizeof esrc);
        p += 9;
        if (name && strcmp(name, ename) != 0) continue;
        if (strcmp(esrc, "local") == 0) continue;
        wchar_t wpath[256]; swprintf_s(wpath, 256, L"/api/packages/%S", ename);
        char *tok = get_token();
        HttpResp r = http_req(L"GET", wpath, NULL, tok); free(tok);
        if (r.status == 200) {
            char latest[64] = ""; json_str(r.body, "latest", latest, sizeof latest);
            if (strcmp(latest, ever) != 0) {
                printf("  updating %s  %s -> %s\n", ename, ever, latest);
                http_free(&r); cmd_uninstall(ename); cmd_install_remote(ename, NULL); updated++;
            } else { printf(DIM "  %s v%s up to date\n" RST, ename, ever); http_free(&r); }
        } else { http_free(&r); }
    }
    free(data);
    if (!updated) printf(GRN "  all packages up to date\n" RST);
    return 0;
}
static int cmd_init(void) {
    if (path_exists(TOML_FILE)) {
        printf(YLW "  warning:" RST " omnikarai.toml already exists\n"); return 0;
    }
    char cwd[512] = "my_package"; GetCurrentDirectoryA(sizeof cwd, cwd);
    char *last = strrchr(cwd, '\\'); if (last) memmove(cwd, last + 1, strlen(last));
    for (char *c = cwd; *c; c++) if (*c >= 'A' && *c <= 'Z') *c += 32;
    char buf[1024];
    snprintf(buf, sizeof buf,
             "[metadata]\nname        = \"%s\"\nversion     = \"0.1.0\"\n"
             "description = \"A short description\"\nauthor      = \"Your Name\"\n"
             "license     = \"MIT\"\nhomepage    = \"\"\nrepository  = \"\"\n\n"
             "[dependencies]\n# math = \">=1.0\"\n", cwd);
    write_file_s(TOML_FILE, buf, (DWORD)strlen(buf));
    printf(GRN "  created" RST " omnikarai.toml\n");
    printf(DIM "  edit description/author, then: omnip publish\n" RST);
    return 0;
}
static int cmd_login(const char *token) {
    if (!token || !token[0]) {
        printf(RED "error:" RST " usage: omnip login <api-token>\n");
        printf(DIM "  get a token from https://opi-nine.vercel.app/dashboard\n" RST);
        return 1;
    }
    if (strstr(token, "...")) {
        printf(RED "error:" RST " masked/truncated token detected.\n");
        printf(DIM "  Use the FULL token from dashboard -> API Tokens -> Copy\n" RST);
        return 1;
    }
    save_token(token);
    printf(GRN "  token saved" RST "\n");
    printf(DIM "  path: %%LOCALAPPDATA%%\\Programs\\omnikarai\\omnip\\token\n" RST);
    return 0;
}
static int cmd_version(void) {
    printf(BLD "omnip" RST " v%s  - Omnikarai Package Manager\n", OMNIP_VERSION);
    printf(DIM "  registry: https://opi-nine.vercel.app\n" RST);
    printf(DIM "  packages: %%LOCALAPPDATA%%\\Programs\\omnikarai\\site-packages\\\n" RST);
    return 0;
}
static void print_usage(void) {
    printf(BLD "omnip" RST " v%s - Omnikarai Package Manager\n\n", OMNIP_VERSION);
    printf("  " CYN "omnip install"   RST "   <pkg>[@ver]   install from OPI\n");
    printf("  " CYN "omnip install"   RST "   .             install local package\n");
    printf("  " CYN "omnip uninstall" RST " <pkg>           remove package\n");
    printf("  " CYN "omnip list"      RST "                 list installed\n");
    printf("  " CYN "omnip info"      RST "      <pkg>      package details + file list\n");
    printf("  " CYN "omnip search"    RST "    <query>      search OPI\n");
    printf("  " CYN "omnip publish"   RST "                 publish to OPI\n");
    printf("  " CYN "omnip update"    RST "    [pkg]        update package(s)\n");
    printf("  " CYN "omnip init"      RST "                 create omnikarai.toml\n");
    printf("  " CYN "omnip login"     RST "     <token>     save API token\n");
    printf("  " CYN "omnip version"   RST "                 show version\n");
    printf("\n" DIM "  env var OPI_TOKEN overrides saved token\n" RST);
}
int main(int argc, char **argv) {
    enable_ansi();
    if (argc < 2) { print_usage(); return 1; }
    const char *cmd = argv[1];
    const char *arg = (argc >= 3) ? argv[2] : NULL;
    if (!strcmp(cmd, "install"))                              return cmd_install(arg);
    if (!strcmp(cmd, "uninstall") || !strcmp(cmd, "remove")) return cmd_uninstall(arg);
    if (!strcmp(cmd, "list") || !strcmp(cmd, "ls"))           return cmd_list();
    if (!strcmp(cmd, "info"))                                 return cmd_info(arg);
    if (!strcmp(cmd, "search"))                               return cmd_search(arg);
    if (!strcmp(cmd, "publish"))                              return cmd_publish();
    if (!strcmp(cmd, "update"))                               return cmd_update(arg);
    if (!strcmp(cmd, "init"))                                 return cmd_init();
    if (!strcmp(cmd, "login"))                                return cmd_login(arg);
    if (!strcmp(cmd, "version") || !strcmp(cmd, "--version")) return cmd_version();
    if (!strcmp(cmd,"help")||!strcmp(cmd,"--help")||!strcmp(cmd,"-h")) { print_usage(); return 0; }
    printf(RED "error:" RST " unknown command '%s'\n\n", cmd); print_usage(); return 1;
}
