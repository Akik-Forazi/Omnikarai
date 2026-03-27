const { performance } = require('perf_hooks');
const t0 = performance.now();
let s = 0n;
for (let i = 1n; i <= 100000000n; i++) s += i;
const ms = performance.now() - t0;
console.log(s.toString());
process.stderr.write(`COMPUTE_MS=${ms.toFixed(3)}\n`);
