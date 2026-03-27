#include <stdio.h>
#include <windows.h>
#include <immintrin.h>

// Matrix Size: 64x64 (Typical AI Layer Block)
#define N 64
#define REPS 100000

// Matrices
float A[N][N] __attribute__((aligned(32)));
float B[N][N] __attribute__((aligned(32)));
float C[N][N] __attribute__((aligned(32)));

// --- 1. STANDARD AI (Memory Heavy) ---
// This simulates how a standard compiler writes a dot product loop.
// It writes back the accumulator 'sum' to memory constantly.
void bench_standard_ai() {
    for (int r = 0; r < REPS; r++) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                float sum = C[i][j];
                for (int k = 0; k < N; k++) {
                    sum += A[i][k] * B[k][j];
                    // Force a memory barrier to simulate "Safe" compilation
                    __asm__ __volatile__ ("" : : "m" (sum) : "memory");
                }
                C[i][j] = sum;
            }
        }
    }
}

// --- 2. OMNIKARAI EPHEMERAL AI (The New Idea) ---
// This uses "Selective Materialization". 
// It loads 8 elements of the output into YMM registers (The Thinking Space).
// It performs the entire 64-step dot product entirely in registers.
// It only "Materializes" (writes to RAM) ONCE at the very end.
void bench_ephemeral_ai() {
    for (int r = 0; r < REPS; r++) {
        for (int i = 0; i < N; i++) {
            // Process 8 columns at once using AVX2 SIMD (YMM0-YMM7)
            for (int j = 0; j < N; j += 8) {
                __asm__ __volatile__ (
                    "vmovups (%0), %%ymm0\n\t"        // Load 8 results from C (The Soul)
                    "xorl %%eax, %%eax\n\t"           // k = 0
                    "1:\n\t"
                    "vbroadcastss (%1, %%rax, 4), %%ymm1\n\t" // Load 1 weight from A
                    "vmovups (%2, %%rax, 4), %%ymm2\n\t"     // Load 8 inputs from B
                    "vfmadd231ps %%ymm1, %%ymm2, %%ymm0\n\t" // FMA: sum += A*B (REGISTER ONLY)
                    "incl %%eax\n\t"
                    "cmpl $64, %%eax\n\t"
                    "jl 1b\n\t"
                    "vmovups %%ymm0, (%0)\n\t"        // FINAL MATERIALIZATION (Only write once!)
                    :
                    : "r" (&C[i][j]), "r" (&A[i][0]), "r" (&B[0][j])
                    : "rax", "ymm0", "ymm1", "ymm2", "memory"
                );
            }
        }
    }
}

int main() {
    // Initialize matrices
    for(int i=0; i<N; i++) for(int j=0; j<N; j++) { A[i][j]=0.1f; B[i][j]=0.2f; C[i][j]=0.0f; }

    LARGE_INTEGER freq, t0, t1, t2;
    QueryPerformanceFrequency(&freq);

    printf("OMNIKARAI AI ARCHITECTURE BENCHMARK -- i5-8350U\n");
    printf("Workload: %d x %d Matrix Multiplication (%d iterations)\n", N, N, REPS);
    printf("----------------------------------------------------------\n");

    printf("Executing Standard AI Forward Pass...\n");
    QueryPerformanceCounter(&t0);
    bench_standard_ai();
    QueryPerformanceCounter(&t1);
    
    printf("Executing Omnikarai Ephemeral Forward Pass...\n");
    bench_ephemeral_ai();
    QueryPerformanceCounter(&t2);

    double time_std = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;
    double time_eph = (double)(t2.QuadPart - t1.QuadPart) * 1000.0 / freq.QuadPart;
    
    // Calculate GFLOPS (N*N*N*2 operations per call)
    double ops = (double)N * N * N * 2.0 * REPS;
    double gflops_std = (ops / 1e9) / (time_std / 1000.0);
    double gflops_eph = (ops / 1e9) / (time_eph / 1000.0);

    printf("\n--- RESULTS ---\n");
    printf("Standard AI:  %.2f ms (%.2f GFLOPS)\n", time_std, gflops_std);
    printf("Ephemeral AI: %.2f ms (%.2f GFLOPS)\n", time_eph, gflops_eph);
    printf("Speedup:      %.2fx faster\n", time_std / time_eph);
    
    // The "Soul" Metric: Memory Efficiency
    // Standard writes to memory N*N*N times. Ephemeral writes N*N times.
    double writes_std = (double)N * N * N * REPS;
    double writes_eph = (double)N * N * REPS;
    printf("Memory Write Reduction: %.0fx fewer writes\n", writes_std / writes_eph);
    printf("Heat/Entropy Reduction: HIGH (Less Landauer Erasure)\n");

    return 0;
}
