// bench_matmul.c — AI: Matrix Multiply — self-timed
#include <stdio.h>
#include <windows.h>

long long matmul_sum(int n) {
    long long total = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            long long acc = 0;
            for (int k = 0; k < n; k++) {
                long long a = (i + k) % 17;
                long long b = (k * j + 1) % 13;
                acc += a * b;
            }
            total += acc;
        }
    return total;
}

int main(void) {
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    long long result = 0;
    for (int rep = 0; rep < 5000; rep++)
        result = matmul_sum(32);
    QueryPerformanceCounter(&t1);
    double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;
    printf("%lld\n", result);
    fflush(stdout);
    fprintf(stderr, "COMPUTE_MS=%.3f\n", ms);
    return 0;
}
