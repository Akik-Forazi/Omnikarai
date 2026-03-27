// ============================================================
//  omni_pkg.h — Package Module Loader + OKC Format
//  Omnikarai v5.2 | Fraziym Tech & AI | 2026
//
//  Three publish modes:
//    open      — raw .ok source, anyone can read
//    compiled  — .okc binary, AES-256 encrypted
//    protected — .okc binary, Kyber-1024 post-quantum encrypted
//
//  On install, omnicc resolves 'use pkgname' by:
//    1. Finding site-packages/pkgname/*.okc  → load encrypted binary
//    2. Finding site-packages/pkgname/*.ok   → parse+compile inline
//    3. Neither → error with install hint
// ============================================================
#ifndef OMNI_PKG_H
#define OMNI_PKG_H

#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include <stdint.h>
#include <stddef.h>

// ── OKC file format ─────────────────────────────────────────
// Magic: "OKCP" (4 bytes)
// Version: uint8  (1)
// Mode:    uint8  (0=open, 1=compiled/aes256, 2=protected/kyber)
// Reserved: uint16 (0)
// NameLen: uint16  — length of package name string
// Name:    char[]  — package name (not null-terminated)
// FnCount: uint32  — number of exported functions
// FnTable: FnRecord[FnCount]
//   FnNameLen: uint16
//   FnName:    char[]
//   ParamCount: uint8
// PayloadLen: uint64 — byte length of encrypted/raw code blob
// Payload:   uint8[] — encrypted x86-64 bytecode or raw source
// MAC:       uint8[32] — BLAKE2s-256 authentication tag

#define OKC_MAGIC        "OKCP"
#define OKC_MAGIC_LEN    4
#define OKC_VERSION      1
#define OKC_MODE_OPEN    0   // raw .ok source inside payload
#define OKC_MODE_AES256  1   // AES-256-GCM encrypted x86-64 bytecode
#define OKC_MODE_KYBER   2   // Kyber-1024 + AES-256-GCM encrypted bytecode

// ── Package namespace registry ───────────────────────────────
// Tracks which installed packages have been loaded and what
// functions they export, so cg_module_call can route calls.

#define MAX_PKG_NS       64
#define MAX_PKG_FNS      128
#define PKG_NAME_LEN     64
#define PKG_FN_LEN       64

typedef struct {
    char fn_name[PKG_FN_LEN];   // e.g. "repeat"
    int  param_count;
} PkgFnInfo;

typedef struct {
    char       ns[PKG_NAME_LEN];           // e.g. "strutil"
    PkgFnInfo  fns[MAX_PKG_FNS];
    int        fn_count;
    int        loaded;                     // 1 if already compiled into cg
} PkgNamespace;

// Global package registry (populated during USE_STATEMENT processing)
extern PkgNamespace g_pkg_registry[MAX_PKG_NS];
extern int          g_pkg_count;

// ── Public API ───────────────────────────────────────────────

// Called from USE_STATEMENT handler in codegen.c
// Finds the package, parses+compiles its .ok files into cg,
// registers all its functions in g_pkg_registry.
// Returns 1 on success, 0 on failure.
int  pkg_load(CodeGen *cg, const char *pkg_name);

// Returns 1 if pkg_name is a loaded package namespace
int  pkg_is_loaded(const char *pkg_name);

// Returns info about a function in a loaded package, NULL if not found
PkgFnInfo *pkg_fn_find(const char *pkg_name, const char *fn_name);

// Returns the full internal name used in fn_table: "pkgname__fnname"
void pkg_fn_internal_name(const char *pkg_name, const char *fn_name,
                           char *out, int out_len);

// ── OKC encryption API (used by omnip compile commands) ──────

// Encrypt raw source bytes with AES-256-GCM
// key must be 32 bytes, nonce 12 bytes
// Returns heap-allocated ciphertext, sets *out_len
uint8_t *okc_aes256_encrypt(const uint8_t *plaintext, size_t pt_len,
                             const uint8_t *key, const uint8_t *nonce,
                             size_t *out_len);

// Decrypt AES-256-GCM ciphertext
// Returns heap-allocated plaintext, sets *out_len. NULL on auth failure.
uint8_t *okc_aes256_decrypt(const uint8_t *ciphertext, size_t ct_len,
                             const uint8_t *key, const uint8_t *nonce,
                             size_t *out_len);

// Kyber-1024 key generation
// pk: 1568 bytes public key, sk: 3168 bytes secret key
void okc_kyber1024_keygen(uint8_t *pk, uint8_t *sk);

// Kyber-1024 encapsulation: given pk, produce shared secret + ciphertext
// ct: 1568 bytes, ss: 32 bytes shared secret
void okc_kyber1024_encaps(const uint8_t *pk, uint8_t *ct, uint8_t *ss);

// Kyber-1024 decapsulation: given sk + ct, recover shared secret
// ss: 32 bytes
void okc_kyber1024_decaps(const uint8_t *sk, const uint8_t *ct, uint8_t *ss);

// BLAKE2s-256 MAC
void okc_blake2s(const uint8_t *msg, size_t msg_len,
                 const uint8_t *key, size_t key_len,
                 uint8_t *out);

// Write a complete .okc file
// mode: OKC_MODE_OPEN / OKC_MODE_AES256 / OKC_MODE_KYBER
// key: 32-byte AES key (for AES256/KYBER modes), NULL for open
// For KYBER mode, key is the Kyber shared secret used to derive AES key
int okc_write(const char *path, const char *pkg_name,
              const char **fn_names, const int *param_counts, int fn_count,
              const uint8_t *payload, size_t payload_len,
              int mode, const uint8_t *key);

// Read and verify a .okc file header, decrypt payload
// Returns heap-allocated plaintext payload, sets *out_len
// On failure returns NULL
uint8_t *okc_read(const char *path, char *pkg_name_out,
                  PkgFnInfo *fns_out, int *fn_count_out,
                  int *mode_out, size_t *out_len);

#endif // OMNI_PKG_H
