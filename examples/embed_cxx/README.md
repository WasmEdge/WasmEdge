This is a example for embedding WasmEdge Sdk in C++, and how to build WebAssembly form C++ sources.

First, download and extract [wasi-sdk](https://github.com/WebAssembly/wasi-sdk/releases), setup environment variable `WASI_SDK_HOME=path/to/wasi-sdk` .

Build with cmake
```
$ cmake -Bbuild -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_AOT_RUNTIME=OFF; cmake --build build
```

Run `embed_cxx`
```
$ cd build
$ ./embed_cxx
Get result: 3524578
run native fib(32), ints : 0.006482 s
Get result: 3524578
run wasm fib(32), ints : 2.84775 s
```
