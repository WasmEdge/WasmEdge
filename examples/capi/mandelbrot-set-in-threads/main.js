const { performance } = require("perf_hooks");
const { Worker } = require("worker_threads");
const { createCanvas } = require("canvas");
const fs = require("fs");
const { argv } = require("process");

const memory = new WebAssembly.Memory({
  initial: 60,
  maximum: 60,
  shared: true,
});

const config = {
  x: -0.743644786,
  y: 0.1318252536,
  d: 0.00029336,
  iterations: 10000,
};

let num_threads = 4;
if (argv.length > 1) {
  num_threads = Number(argv[2]);
}
console.log("Number of threads: ", num_threads);

const bytes = fs.readFileSync("mandelbrot.wasm");

let finished = 0;
const start = performance.now();

for (let rank = 0; rank < num_threads; rank++) {
  const worker = new Worker("./worker.js", {
    workerData: { data: { memory, config, num_threads, rank, bytes } },
  });
  worker.on("message", (offset) => {
    finished++;
    if (finished === num_threads) {
      console.log("Elapsed Time:", performance.now() - start);
      const canvasData = new Uint8Array(memory.buffer, offset, 1200 * 800 * 4);
      fs.writeFileSync("./output-node.bin", canvasData);
    }
  });
}
