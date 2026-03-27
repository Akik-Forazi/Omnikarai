import time, sys
def is_prime(n):
    if n < 2: return False
    d = 2
    while d * d <= n:
        if n % d == 0: return False
        d += 1
    return True

t0 = time.perf_counter()
result = 0
for r in range(30):
    count = 0
    for n in range(2, 100001):
        if is_prime(n): count += 1
    result = count
ms = (time.perf_counter() - t0) * 1000
print(result)
print(f'COMPUTE_MS={ms:.3f}', file=sys.stderr)
