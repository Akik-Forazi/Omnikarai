// C reference benchmark — 4-layer MLP, hidden=128
// Compiled with: gcc -O3 -march=native -mavx2 -mfma -o bench_c bench_mlp_c.c -lm
// This is what Omnikarai must beat per the Speed God Plan.
// Fraziym Tech & AI | March 2026

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <windows.h>

#define INPUT  128
#define HIDDEN 128
#define OUTPUT 10
#define RUNS   1000

static LARGE_INTEGER g_freq;
static void timer_init(void) { QueryPerformanceFrequency(&g_freq); }
static int64_t timer_now(void) {
    LARGE_INTEGER t; QueryPerformanceCounter(&t); return (int64_t)t.QuadPart;
}
static int64_t timer_us(int64_t t0) {
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    return ((int64_t)now.QuadPart - t0) * 1000000LL / g_freq.QuadPart;
}

// Scalar matmul (what C -O2 generates without explicit vectorization)
static void matmul_scalar(const float* A, const float* x, float* y, int rows, int cols) {
    for (int r = 0; r < rows; r++) {
        float sum = 0.0f;
        const float* Ar = A + r * cols;
        for (int c = 0; c < cols; c++) sum += Ar[c] * x[c];
        y[r] = sum;
    }
}

// ReLU scalar
static void relu(float* x, int n) {
    for (int i = 0; i < n; i++) if (x[i] < 0.0f) x[i] = 0.0f;
}

// Single MLP forward pass
static void mlp_forward(
    const float* W1, const float* W2,
    const float* W3, const float* W4,
    const float* x,
    float* h1, float* h2, float* h3, float* out)
{
    matmul_scalar(W1, x,  h1, HIDDEN, INPUT);  relu(h1, HIDDEN);
    matmul_scalar(W2, h1, h2, HIDDEN, HIDDEN); relu(h2, HIDDEN);
    matmul_scalar(W3, h2, h3, HIDDEN, HIDDEN); relu(h3, HIDDEN);
    matmul_scalar(W4, h3, out, OUTPUT, HIDDEN);
}

int main(void) {
    timer_init();

    // 64-byte aligned allocs (same as Omnikarai)
    float* W1  = (float*)_aligned_malloc(HIDDEN * INPUT  * sizeof(float), 64);
    float* W2  = (float*)_aligned_malloc(HIDDEN * HIDDEN * sizeof(float), 64);
    float* W3  = (float*)_aligned_malloc(HIDDEN * HIDDEN * sizeof(float), 64);
    float* W4  = (float*)_aligned_malloc(OUTPUT * HIDDEN * sizeof(float), 64);
    float* x   = (float*)_aligned_malloc(INPUT  * sizeof(float), 64);
    float* h1  = (float*)_aligned_malloc(HIDDEN * sizeof(float), 64);
    float* h2  = (float*)_aligned_malloc(HIDDEN * sizeof(float), 64);
    float* h3  = (float*)_aligned_malloc(HIDDEN * sizeof(float), 64);
    float* out = (float*)_aligned_malloc(OUTPUT * sizeof(float), 64);

    // Init weights = 1/128 (same as Omnikarai benchmark)
    float wv = 1.0f / 128.0f;
    for (int i = 0; i < HIDDEN*INPUT;  i++) W1[i] = wv;
    for (int i = 0; i < HIDDEN*HIDDEN; i++) W2[i] = wv;
    for (int i = 0; i < HIDDEN*HIDDEN; i++) W3[i] = wv;
    for (int i = 0; i < OUTPUT*HIDDEN; i++) W4[i] = wv;
    for (int i = 0; i < INPUT; i++) x[i] = 1.0f;

    // Warmup
    for (int w = 0; w < 10; w++)
        mlp_forward(W1, W2, W3, W4, x, h1, h2, h3, out);

    // Timed run
    int64_t t0 = timer_now();
    for (int r = 0; r < RUNS; r++)
        mlp_forward(W1, W2, W3, W4, x, h1, h2, h3, out);
    int64_t total_us = timer_us(t0);

    printf("C scalar  total_us=%lld  per_inference=%lld us\n",
           (long long)total_us, (long long)(total_us / RUNS));
    printf("out[0] = %f\n", out[0]);

    _aligned_free(W1); _aligned_free(W2); _aligned_free(W3); _aligned_free(W4);
    _aligned_free(x);  _aligned_free(h1); _aligned_free(h2); _aligned_free(h3);
    _aligned_free(out);
    return 0;
}
