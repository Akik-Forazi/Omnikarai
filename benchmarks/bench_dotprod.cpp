// bench_dotprod.cpp — AI: Dot Product
#include <iostream>
#include <windows.h>

long long dotprod(int n) {
    long long s = 0;
    for (int i = 0; i < n; i++)
        s += (long long)i * (n - i);
    return s;
}

int main(int argc, char** argv) { (void)argv;
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    long long result = 0;
    int n = 65536 + (argc - 1); // argc always 1
    for (int rep = 0; rep < 10000; rep++)
        result = dotprod(n);
    QueryPerformanceCounter(&t1);
    double ms = (double)(t1.QuadPart-t0.QuadPart)*1000.0/freq.QuadPart;
    std::cout << result << "\n"; std::cout.flush();
    fprintf(stderr, "COMPUTE_MS=%.3f\n", ms);
    return 0;
}
