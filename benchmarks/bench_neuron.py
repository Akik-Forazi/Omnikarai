import time, sys
def relu(x):
    return x if x > 0 else 0

def layer(inputs, neurons):
    total = 0
    for n in range(neurons):
        acc = 0
        for i in range(inputs):
            w = (n * inputs + i + 1) % 11
            x = (i - inputs // 2) * w
            acc += relu(x)
        total += acc
    return total

t0 = time.perf_counter()
result = 0
for _ in range(500):
    result = layer(128, 256)
ms = (time.perf_counter() - t0) * 1000
print(result)
print(f'COMPUTE_MS={ms:.3f}', file=sys.stderr)
