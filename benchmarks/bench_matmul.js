const { performance } = require('perf_hooks');
function matmulSum(n) {
    let total = 0;
    for (let i = 0; i < n; i++)
        for (let j = 0; j < n; j++) {
            let acc = 0;
            for (let k = 0; k < n; k++)
                acc += ((i + k) % 17) * ((k * j + 1) % 13);
            total += acc;
        }
    return total;
}
const t0 = performance.now();
let result = 0;
for (let rep = 0; rep < 5000; rep++) result = matmulSum(32);
const ms = performance.now() - t0;
console.log(result);
process.stderr.write(`COMPUTE_MS=${ms.toFixed(3)}\n`);
