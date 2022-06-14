# WasmEdge-Mandelbrot-Example

This example tests the performance of multi-threaded WasmEdge and NodeJS. The C code is original from [ColinEberhardt/wasm-mandelbrot](https://github.com/ColinEberhardt/wasm-mandelbrot/blob/master/wasm/mandelbrot.c), which randers image of Mandelbrot set using wasm. We adopted it into a multi-worker version that parallelly renders the image. The C code was compiled into wasm with clang toolchain and loaded by different runtimes. We demonstrate how to use WasmEdge C_API to create multiple threads to help us render the image parallelly. Also, we use the wasm thread proposal to share image memory between threads.

+ WasmEdge 
    + WasmEdge C-API
    + The wasm file is compiled into shared library using AOT compiler
    + Multi-threaded with `std::threads` 
+ NodeJS 
    + NodeJS WebAssembly (V8)
    + Multi-threaded with JS Worker threads

Please also read `tutorial.md` for more information.

## Installation && Test

Please follows the [installation step](https://wasmedge.org/book/en/start/install.html) to install WasmEdge.

```
npm install canvas
make
bash test.bash
```

`convert.js` is a simple script that converts the binary image into png.

```
node convert.js output-wasmedge.bin output-wasmedge.png
node convert.js output-node.bin output-node.png
```

## Results

The results were tested on `Intel(R) Xeon(R) Gold 6226R CPU` and `node v14.18.2`.
This experiment shows:
1. Multi-threaded WasmEdge-AOT has better thread scalability compared with multi-worker NodeJS
2. Single-threaded WasmEdge-AOT outperforms NodeJS runtime by 1.27x 

**Thread Scalability**
we compare WasmEdge-AOT, NodeJS, and WasmEdge-Interp for strong thread scalability, the figure below shows that multi-threading accelerates the image rendering on all runtime. However, WasmEdge-AOT has better thread scalability compared with NodeJS. With 10 threads, WasmEdge has 5.71x speedup while NodeJS has only 4.71x speedup.

![Thread Scalability](https://docs.google.com/spreadsheets/d/e/2PACX-1vQOPP-uuYNXXv8DMT8CJCLOU9P2RYN01KFiMn2gevPPztPrHF9P9Y3d55-km9fpbzZU5QCsYKJmvFRc/pubchart?oid=1451848374&format=image)


**Elapsed Time (ms)**
We measure the elapsed time of WasmEdge-AOT and NodeJS. Single-threaded WasmEdge-AOT outperforms NodeJS runtime by 1.27x. 

| Number of threads | WasmEdge-AOT | stdev | NodeJS | stdev |
|:-----------------:|:------------:|:-----:|:------:|:-----:|
|         1         |    525.60    |  8.75 | 668.51 | 17.41 |
|         2         |    287.54    |  5.77 | 381.90 |  5.99 |
|         3         |    234.84    |  2.22 | 323.88 |  2.66 |
|         4         |    183.08    |  6.02 | 261.05 |  1.93 |
|         5         |    159.18    |  1.49 | 236.58 |  1.76 |
|         6         |    132.43    |  4.12 | 206.75 |  0.91 |
|         7         |    118.19    |  1.64 | 191.05 |  1.41 |
|         8         |    108.19    |  1.30 | 178.92 |  2.20 |
|         9         |     95.05    |  0.89 | 167.48 |  1.62 |
|         10        |     92.11    |  2.27 | 163.58 |  1.96 |


![Speedup](https://docs.google.com/spreadsheets/d/e/2PACX-1vQOPP-uuYNXXv8DMT8CJCLOU9P2RYN01KFiMn2gevPPztPrHF9P9Y3d55-km9fpbzZU5QCsYKJmvFRc/pubchart?oid=1510018326&format=image)


### Images

**output-node.png**
![node](./output-node.png)


**output-wasmedge.png**
![wasmedge](./output-wasmedge.png)

