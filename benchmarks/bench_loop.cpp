// bench_loop.cpp — sum with data-dependent loop, prevents constant folding
#include <iostream>
#include <windows.h>
int main(int argc, char** argv) {
    (void)argv;
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    long long s = 0;
    long long start = (long long)argc;
    for (long long i = start; i <= 100000000LL; i++) s += i;
    QueryPerformanceCounter(&t1);
    double ms = (double)(t1.QuadPart-t0.QuadPart)*1000.0/freq.QuadPart;
    std::cout << s << "\n"; std::cout.flush();
    fprintf(stderr, "COMPUTE_MS=%.3f\n", ms);
    return 0;
}
