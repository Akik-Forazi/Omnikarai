import time, sys
def dotprod(n):
    s = 0
    for i in range(n):
        s += i * (n - i)
    return s

t0 = time.perf_counter()
result = 0
for _ in range(100000):
    result = dotprod(1024)
ms = (time.perf_counter() - t0) * 1000
print(result)
print(f'COMPUTE_MS={ms:.3f}', file=sys.stderr)
