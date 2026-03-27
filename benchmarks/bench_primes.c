// bench_primes.c — Prime Counting, 30 reps — self-timed
#include <stdio.h>
#include <windows.h>

int is_prime(int n) {
    if (n < 2) return 0;
    for (int d = 2; (long long)d * d <= n; d++)
        if (n % d == 0) return 0;
    return 1;
}

int main(void) {
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    int result = 0;
    for (int r = 0; r < 30; r++) {
        int count = 0;
        for (int n = 2; n <= 100000; n++)
            if (is_prime(n)) count++;
        result = count;
    }
    QueryPerformanceCounter(&t1);
    double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;
    printf("%d\n", result);
    fflush(stdout);
    fprintf(stderr, "COMPUTE_MS=%.3f\n", ms);
    return 0;
}
