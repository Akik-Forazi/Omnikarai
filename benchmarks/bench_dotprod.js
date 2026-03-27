const { performance } = require('perf_hooks');
function dotprod(n) {
    let s = 0;
    for (let i = 0; i < n; i++) s += i * (n - i);
    return s;
}
const t0 = performance.now();
let result = 0;
for (let rep = 0; rep < 100000; rep++) result = dotprod(1024);
const ms = performance.now() - t0;
console.log(result);
process.stderr.write(`COMPUTE_MS=${ms.toFixed(3)}\n`);
