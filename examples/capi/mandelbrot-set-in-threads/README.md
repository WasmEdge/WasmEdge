# Mandelbrot Set in Threads Example With WasmEdge C API

## Introduction

Rendering the [Mandelbrot set](https://en.wikipedia.org/wiki/Mandelbrot_set) demands tremendous computation. The multi-threaded parallel is widely used to accelerate this kind of compute-intensive workload. In this example, we are going to demonstrate how to use WasmEdge to accelerate this workload. We use WasmEdge C_API to create multiple threads to help us render the image parallelly. We also use the [WASM thread proposal](https://github.com/webassembly/threads) to share the image memory between threads to render the image parallelly.

With this example, we could compare the performance of WasmEdge and NodeJS. We show that single-threaded WasmEdge Runtime outperforms NodeJS runtime by 1.27x, and multi-threaded WasmEdge has better thread scalability compared with multi-worker NodeJS. WasmEdge is a lightweight solution and the threading with WasmEdge exhibit higher parallelism compared with NodeJS.

The C code is original from [ColinEberhardt/wasm-mandelbrot](https://github.com/ColinEberhardt/wasm-mandelbrot/blob/master/wasm/mandelbrot.c), which randers image of Mandelbrot set using WASM. We adopted it into a multi-worker version that parallelly renders the image. The C code was compiled into WASM with `clang` toolchain and loaded by different runtimes.

+ WasmEdge
  + WasmEdge C-API
  + The wasm file is compiled into shared library using AOT compiler
  + Multi-threaded with `std::threads`
+ NodeJS
  + NodeJS WebAssembly (V8)
  + Multi-threaded with JS Worker threads

## Tutorial

### WasmEdge Installation

Please follows the [installation step](https://wasmedge.org/book/en/quick_start/install.html) to install WasmEdge.

### The Mandelbrot C Program to WASM

Colin Eberhardt wrote a [blog - Exploring different approaches to building WebAssembly modules](https://blog.scottlogic.com/2017/10/17/wasm-mandelbrot.html) that demonstrate how to compile Mandelbrot set rendering C code into WASM. Please refer to his blog for more information. We use `LLVM-12` to compile the code into WASM. Furthermore, We adopted it into a multi-worker version that parallelly renders the image. We split the image into multiple strides in the y-direction, and assign each thread a stride. As illustrated in the figure below, the image is split into 4 strides and assigned to 4 threads.

![Mandelbrot picture](https://i.imgur.com/hd0pUAF.jpg)

#### Mandelbrot in Multi-Thread

The `num_threads` and `rank`(index of thread) are passed into each thread.

```c
void mandelbrot_thread(int maxIterations, int num_threads, int rank, double cx, double cy, double diameter);
```

With the information above, each thread can calculate the stride size and offset itself.

```c
int y_stride =
    (HEIGHT + num_threads - 1) / num_threads; // ceil(HEIGHT, num_threads)
int y_offset = y_stride * rank;
int y_max = (y_offset + y_stride > HEIGHT) ? HEIGHT : y_offset + y_stride;
for (int y = y_offset; y < y_max; y++) {
    ...
}
```

#### Compile C into WASM

Notice that we need to import share memory from other workers later, so we should add `--import-memory` and `--shared-memory` to the linker.

```bash
wasm-ld --no-entry mandelbrot.o -o mandelbrot.wasm --import-memory --export-all --shared-memory --features=mutable-globals,atomics,bulk-memory
```

#### Enable AOT Mode

After compiling C into WASM, the WASM file could be further compiled into a shared library with the WasmEdge AOT compiler.

```bash
wasmedgec --enable-threads mandelbrot.wasm mandelbrot.so
```

### Construct WasmEdge Runtime Workers

We are going to demonstrate how to use the [WasmEdge C API](https://wasmedge.org/book/en/embed/c/ref.html) to create multiple workers and share memory between workers.

With thread proposal enabled, we can add a flag `.Shared = true` to memory instance, so the memory could be shared between workers. The following snippet creates a new WebAssembly Memory instance with an initial size of 60 pages. Notice that, unlike unshared memories, shared memories must specify a "maximum" size.

```c
WasmEdge_Limit MemLimit = {.HasMax = true, .Shared = true, .Min = 60, .Max = 60};
WasmEdge_MemoryTypeContext *MemTypeCxt = WasmEdge_MemoryTypeCreate(MemLimit);
```

Notice that the imported memory of the WASM is at module `{env: memory}`. This could be ensured by checking the compiled WASM file. We should create the module instance and the memory instance with WasmEdge C API.

```c
WasmEdge_String ExportName = WasmEdge_StringCreateByCString("env");
WasmEdge_ModuleInstanceContext *HostModCxt = WasmEdge_ModuleInstanceCreate(ExportName);
WasmEdge_MemoryInstanceContext *HostMemory = WasmEdge_MemoryInstanceCreate(MemTypeCxt);
WasmEdge_String MemoryName = WasmEdge_StringCreateByCString("memory");
WasmEdge_ModuleInstanceAddMemory(HostModCxt, MemoryName, HostMemory);
```

After the AOT-compiled WASM `mandelbrot.so` being made, you can load the file and instantiate the WASM module with `WasmEdge_VMRegisterModuleFromFile`.

```c
WasmEdge_String ModName = WasmEdge_StringCreateByCString("mandelbrot");
WasmEdge_VMRegisterModuleFromFile(VMCxt, ModName, "./mandelbrot.so");
```

Instead of loading the shared library compiled by AOT, you can load the WASM file directly. However, the WASM binary is executed by WasmEdge in interpreter mode instead of the native environment. Concerned about the compute-intensive nature of the workload, it is recommended to compile the WASM file with the AOT compiler instead of executing in interpreter mode.

Finally, we could create multiple threads with `std::thread` and assign a thread id with each thread. Since the `WasmEdge_VMExecuteRegistered` is thread-safe, the wasm function could be called in the thread. We achieves parallel rendering on the shared image buffer with the cooperation between workers.

```cpp
WasmEdge_String FuncName = WasmEdge_StringCreateByCString("mandelbrotThread");
std::vector<std::thread> Threads;
for (int Tid = 0; Tid < NumThreads; ++Tid) {
Threads.push_back(std::thread(
    [&](int Rank) {
      WasmEdge_Value Params[6] = {
          WasmEdge_ValueGenI32(MaxIterations),
          WasmEdge_ValueGenI32(NumThreads),
          WasmEdge_ValueGenI32(Rank),
          WasmEdge_ValueGenF64(X),
          WasmEdge_ValueGenF64(Y),
          WasmEdge_ValueGenF64(D),
      };
      WasmEdge_VMExecuteRegistered(VMCxt, ModName, FuncName, Params, 6, NULL, 0);
    },
    Tid));
}
for (auto &Thread : Threads) {
    Thread.join();
}
```

### Construct NodeJS V8 Workers

With [Worker Threads](https://nodejs.org/api/worker_threads.html) support in NodeJS, we can create multiple workers by loading the worker javascript file.

Similar to what we have done in CAPI, the shared memory could be created with `WebAssembly.Memory`.

```javascript
const memory = new WebAssembly.Memory({
  initial: 60,
  maximum: 60,
  shared: true,
});
```

The main thread and workers communicate with `postMessage()` semantics. The interface sends a message to the worker's inner scope. This accepts a single parameter, which is the data to send to the worker.

```javascript
// main.js
const { Worker } = require("worker_threads");
const worker = new Worker("./worker.js", {
    workerData: { data: { memory, ... } },
});

worker.on("message", (offset) => {
    ...
});
```

```javascript
// worker.js
WebAssembly.instantiate(bytes, { env: { memory } }).then((Module) => {
  Module.instance.exports.mandelbrotThread(...);
  parentPort.postMessage(...);
});
```

### Convert the output buffer into PNG image

We need to calculate the offset of the image buffer in the memory. The `getImage` interface could return the address of the image buffer in wasm.

```c
unsigned char *getImage() { return &Image[0]; }
```

After the offset is acquired, we extract the buffer from the image and save it into a file. There are many ways to convert the raw buffer back to a png file. We use the same method in the [blog post](https://blog.scottlogic.com/2017/10/17/wasm-mandelbrot.html) that we write a simple js script that use canvas module to convert the image into PNG format.

```javascript
const { createCanvas } = require("canvas");
const canvas = createCanvas(1200, 800);
const context = canvas.getContext("2d");
const imageData = context.createImageData(1200, 800);
imageData.data.set(canvasData);
context.putImageData(imageData, 0, 0);
```

## Testing

```bash
npm install canvas
make
bash test.bash
```

`convert.js` is a simple script that converts the binary image into png.

```bash
node convert.js output-wasmedge.bin output-wasmedge.png
node convert.js output-node.bin output-node.png
```

## Results and Evaluation

To further evaluate the performance of our runtime. We test the two implementations `Wasm-AOT` and `NodeJS` with the same wasm binary. The results were tested on `Intel(R) Xeon(R) Gold 6226R CPU` and `node v14.18.2`. The experiment shows:

1. Single-threaded `WasmEdge-AOT` outperforms `NodeJS` runtime by 1.27x
2. Multi-threaded `WasmEdge-AOT` has better thread scalability compared with multi-worker `NodeJS`

### Thread Scalability

We usually measure thread scalability to show the effectiveness of parallelism. When the number of workers increases n times, the ideal performance of the whole system should also increase n times. Besides `WasmEdge-AOT` and `NodeJS`, we also compare `WasmEdge-Interp` that loads wasm binary instead of AOT-compiled native binary. The figure below shows that multi-threading accelerates the image rendering on all runtime. What's more, `WasmEdge-AOT` has better thread scalability compared with `NodeJS`. With 10 threads, `WasmEdge-AOT` has 5.71x speedup while `NodeJS` has only 4.71x speedup. This shows `WasmEdge-AOT` has smaller overhead to invoke threads compared with `NodeJS`.

Though `WasmEdge-Interp` shows lower thread scalability compared with `WasmEdge-AOT`, it is still slightly higher than `NodeJS`, and the thread-level parallelism also enables the possibility to accelerate wasm interpreter with threads.

![Thread Scalability](https://docs.google.com/spreadsheets/d/e/2PACX-1vQOPP-uuYNXXv8DMT8CJCLOU9P2RYN01KFiMn2gevPPztPrHF9P9Y3d55-km9fpbzZU5QCsYKJmvFRc/pubchart?oid=1451848374&format=image)

### Overall Performance

We measure the elapsed time of `WasmEdge-AOT` and `NodeJS`. We set `NodeJS` as the baseline. As shown in the figure below, single-threaded `WasmEdge-AOT` outperforms `NodeJS` runtime by 1.27x. This shows the AOT compiler outperforms v8 wasm compiler in NodeJS in this workload. With the larger number of threads, the gain increases due to better thread scalability of `WasmEdge-AOT`.

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
