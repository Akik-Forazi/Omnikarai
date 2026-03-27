import time, sys

def fib(n):
    if n <= 1: return n
    return fib(n-1) + fib(n-2)

t0 = time.perf_counter()

result = 0

for r in range(5):
    result = fib(40)

ms = (time.perf_counter() - t0) * 1000

print(result)
print(f'COMPUTE_MS={ms:.3f}', file=sys.stderr)
