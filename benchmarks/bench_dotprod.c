// bench_dotprod.c — AI: Dot Product — self-timed
#include <stdio.h>
#include <windows.h>

long long dotprod(int n) {
    long long s = 0;
    for (int i = 0; i < n; i++)
        s += (long long)i * (n - i);
    return s;
}

int main(int argc, char** argv) {
    (void)argv;
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    long long result = 0;
    // argc is always 1 at runtime — prevents compiler from folding the sum
    int n = 65536 + (argc - 1); // always 65536
    for (int rep = 0; rep < 10000; rep++)
        result = dotprod(n);
    QueryPerformanceCounter(&t1);
    double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;
    printf("%lld\n", result);
    fflush(stdout);
    fprintf(stderr, "COMPUTE_MS=%.3f\n", ms);
    return 0;
}
