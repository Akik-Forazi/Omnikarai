// bench_primes.cpp — self-timed
#include <iostream>
#include <windows.h>
bool is_prime(int n) {
    if (n < 2) return false;
    for (int d = 2; (long long)d * d <= n; d++)
        if (n % d == 0) return false;
    return true;
}
int main() {
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
    double ms = (double)(t1.QuadPart-t0.QuadPart)*1000.0/freq.QuadPart;
    std::cout << result << "\n"; std::cout.flush();
    fprintf(stderr, "COMPUTE_MS=%.3f\n", ms);
    return 0;
}
