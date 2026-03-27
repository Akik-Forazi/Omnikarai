const { performance } = require('perf_hooks');
function relu(x) { return x > 0 ? x : 0; }
function layer(inputs, neurons) {
    let total = 0;
    for (let n = 0; n < neurons; n++) {
        let acc = 0;
        for (let i = 0; i < inputs; i++) {
            const w = (n * inputs + i + 1) % 11;
            const x = (i - (inputs >> 1)) * w;
            acc += relu(x);
        }
        total += acc;
    }
    return total;
}
const t0 = performance.now();
let result = 0;
for (let rep = 0; rep < 2000; rep++) result = layer(128, 256);
const ms = performance.now() - t0;
console.log(result);
process.stderr.write(`COMPUTE_MS=${ms.toFixed(3)}\n`);
