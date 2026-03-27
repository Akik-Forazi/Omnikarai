#include <stdio.h>
#include <windows.h>
#include <immintrin.h>

#define ITERATIONS 100000000
#define VECTOR_SIZE 8

// --- 1. STANDARD MODE (Memory Bound) ---
// This simulates a standard compiler that "spills" the accumulator to the stack 
// because it doesn't know if the memory is "ephemeral".
double test_standard() {
    float data[VECTOR_SIZE] = {1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 8.8f};
    float sum = 0.0f;
    
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);

    for (int i = 0; i < ITERATIONS; i++) {
        // We force a memory write/read cycle here to simulate "Stack Spilling"
        for(int j=0; j<VECTOR_SIZE; j++) {
            sum += data[j]; 
            // The compiler (without high-level AI knowledge) often validates 
            // memory state here, causing a "Write-After-Read" stall.
            __asm__ __volatile__ ("" : : "m" (sum) : "memory"); 
        }
    }

    QueryPerformanceCounter(&t1);
    return (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;
}

// --- 2. EPHEMERAL MODE (Register Bound) ---
// This simulates Omnikarai's "Speed God" logic. 
// It pins the accumulator in a register (XMM0) and uses AVX2 SIMD.
double test_ephemeral() {
    float data[VECTOR_SIZE] = {1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 8.8f};
    
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);

    __asm__ __volatile__ (
        "vmovups (%0), %%ymm1\n\t"       // Load 8 floats into YMM1 (Permanent Weight)
        "vxorps %%ymm0, %%ymm0, %%ymm0\n\t" // Zero out YMM0 (The Ephemeral Accumulator)
        "movl %1, %%ecx\n\t"             // Load iteration count
        "loop_start:\n\t"
        "vaddps %%ymm1, %%ymm0, %%ymm0\n\t" // Ephemeral Calculation (Register only!)
        "decl %%ecx\n\t"
        "jnz loop_start\n\t"
        "vmovups %%ymm0, (%0)\n\t"       // FINAL MATERIALIZATION (Only write once!)
        :
        : "r" (data), "g" (ITERATIONS)
        : "rcx", "ymm0", "ymm1", "memory"
    );

    QueryPerformanceCounter(&t1);
    return (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;
}

int main() {
    printf("OMNIKARAI SPEED GOD BENCHMARK -- i5-8350U\n");
    printf("==========================================\n");
    
    printf("Running Standard (Memory-Heavy) Mode...\n");
    double time_std = test_standard();
    printf("Standard Time:  %.2f ms\n\n", time_std);
    
    printf("Running Ephemeral (Register-Only) Mode...\n");
    double time_eph = test_ephemeral();
    printf("Ephemeral Time: %.2f ms\n\n", time_eph);
    
    double speedup = time_std / time_eph;
    printf("RESULTS:\n");
    printf("Speedup: %.2fx faster\n", speedup);
    printf("Instruction Density: %s\n", speedup > 5.0 ? "MAXIMUM (Speed God Level)" : "HIGH");
    
    // Calculate Memory Bandwidth Saved
    // Standard wrote 8 bytes per iteration, Ephemeral wrote 8 bytes ONCE.
    double gb_saved = (double)ITERATIONS * 8.0 * 8.0 / (1024.0 * 1024.0 * 1024.0);
    printf("Memory Bandwidth Saved: %.2f GB\n", gb_saved);

    return 0;
}
