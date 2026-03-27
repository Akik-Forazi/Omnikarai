// bench_neuron.c — AI: ReLU Neuron Activation Layer — self-timed
#include <stdio.h>
#include <windows.h>

static inline long long relu(long long x) { return x > 0 ? x : 0; }

long long layer(int inputs, int neurons) {
    long long total = 0;
    for (int n = 0; n < neurons; n++) {
        long long acc = 0;
        for (int i = 0; i < inputs; i++) {
            long long w = ((long long)n * inputs + i + 1) % 11;
            long long x = (i - inputs / 2) * w;
            acc += relu(x);
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
    for (int rep = 0; rep < 2000; rep++)
        result = layer(128, 256);
    QueryPerformanceCounter(&t1);
    double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;
    printf("%lld\n", result);
    fflush(stdout);
    fprintf(stderr, "COMPUTE_MS=%.3f\n", ms);
    return 0;
}
