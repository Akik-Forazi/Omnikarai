const { performance } = require('perf_hooks');
function fib(n) {
    if (n <= 1) return n;
    return fib(n-1) + fib(n-2);
}
const t0 = performance.now();
let result = 0;
for (let r = 0; r < 5; r++) result = fib(40);
const ms = performance.now() - t0;
console.log(result);
process.stderr.write(`COMPUTE_MS=${ms.toFixed(3)}\n`);
