# Example of Embedding WasmEdge SDK in C++

This is an example of embedding WasmEdge SDK in C++, and how to build WebAssembly from C++ sources.

First, download and extract [wasi-sdk](https://github.com/WebAssembly/wasi-sdk/releases), setup environment variable `WASI_SDK_HOME=path/to/wasi-sdk` .

Build with cmake

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_USE_LLVM=OFF; cmake --build build
```

Run `embed_cxx`

```bash
$ cd build
$ ./embed_cxx
Get result: 3524578
run native fib(32), ints : 0.006482 s
Get result: 3524578
run wasm fib(32), ints : 2.84775 s
```
