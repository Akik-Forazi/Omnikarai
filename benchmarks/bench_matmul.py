import time, sys
def matmul_sum(n):
    total = 0
    for i in range(n):
        for j in range(n):
            acc = 0
            for k in range(n):
                a = (i + k) % 17
                b = (k * j + 1) % 13
                acc += a * b
            total += acc
    return total

t0 = time.perf_counter()
result = 0
for _ in range(200):
    result = matmul_sum(32)
ms = (time.perf_counter() - t0) * 1000
print(result)
print(f'COMPUTE_MS={ms:.3f}', file=sys.stderr)
