// bench_fib.cpp — self-timed
#include <iostream>
#include <windows.h>
long long fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}
int main() {
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    long long result = 0;
    for (int r = 0; r < 5; r++) result = fib(40);
    QueryPerformanceCounter(&t1);
    double ms = (double)(t1.QuadPart-t0.QuadPart)*1000.0/freq.QuadPart;
    std::cout << result << "\n"; std::cout.flush();
    fprintf(stderr, "COMPUTE_MS=%.3f\n", ms);
    return 0;
}
