import time, sys
t0 = time.perf_counter()
s = 0
for i in range(1, 100000001):
    s += i
ms = (time.perf_counter() - t0) * 1000
print(s)
print(f'COMPUTE_MS={ms:.3f}', file=sys.stderr)
