// bench_loop.c — sum with data-dependent loop, prevents GCC constant folding
// Uses argc as a runtime-unknown start value so compiler cannot fold the sum.
#include <stdio.h>
#include <windows.h>
int main(int argc, char** argv) {
    (void)argv;
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    long long s = 0;
    // Start from argc (always 1 at runtime) so compiler sees unknown start
    long long start = (long long)argc;
    for (long long i = start; i <= 100000000LL; i++) s += i;
    QueryPerformanceCounter(&t1);
    double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;
    printf("%lld\n", s);
    fflush(stdout);
    fprintf(stderr, "COMPUTE_MS=%.3f\n", ms);
    return 0;
}
