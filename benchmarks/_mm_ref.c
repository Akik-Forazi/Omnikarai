#include <stdio.h>
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
int main(void) { printf("%lld\n%lld\n", matmul_sum(4), matmul_sum(32)); return 0; }
