// bench_fib.c — Recursive Fibonacci, fib(40) x 5 reps
// Self-timed: prints COMPUTE_MS=<n> to stderr, result to stdout
#include <stdio.h>
#include <windows.h>

long long fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main(void) {
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    long long result = 0;
    for (int r = 0; r < 5; r++) result = fib(40);
    QueryPerformanceCounter(&t1);
    double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;
    printf("%lld\n", result);
    fflush(stdout);
    fprintf(stderr, "COMPUTE_MS=%.3f\n", ms);
    return 0;
}
