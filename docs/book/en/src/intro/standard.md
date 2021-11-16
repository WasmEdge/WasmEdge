# WebAssembly standard extensions

WasmEdge supports optional WebAssembly features and proposals. Those proposals are likely to become official WebAssembly specifications in the future. WasmEdge supports the following proposals.

* [WASI (WebAssembly Systems Interface) spec](https://github.com/WebAssembly/WASI). WasmEdge has supported the WASI spec for WebAssembly programs to interact with the host Linux operating system securely.
* [Reference Types](https://webassembly.github.io/reference-types/core/). It allows WebAssembly programs to exchange data with host applications and operating systems.
* [Bulk memory operations](https://github.com/WebAssembly/bulk-memory-operations/blob/master/proposals/bulk-memory-operations/Overview.md). The WebAssembly program sees faster memory access and performs better with bulk memory operations.
* [SIMD (Single instruction, multiple data)](https://github.com/second-state/SSVM/blob/master/docs/simd.md). For modern devices with multiple CPU cores, the SIMD allows data processing programs to take advantage of the CPUs fully. SIMD could significantly enhance the performance of data applications.

Meanwhile, the WasmEdge team is exploring the wasi-socket proposal to support network access in WebAssembly programs. 

