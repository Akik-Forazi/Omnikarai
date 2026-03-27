const { performance } = require('perf_hooks');
function is_prime(n) {
    if (n < 2) return false;
    for (let d = 2; d * d <= n; d++)
        if (n % d === 0) return false;
    return true;
}
const t0 = performance.now();
let result = 0;
for (let r = 0; r < 30; r++) {
    let count = 0;
    for (let n = 2; n <= 100000; n++)
        if (is_prime(n)) count++;
    result = count;
}
const ms = performance.now() - t0;
console.log(result);
process.stderr.write(`COMPUTE_MS=${ms.toFixed(3)}\n`);
