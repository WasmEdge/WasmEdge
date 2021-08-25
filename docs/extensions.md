The WasmEdge Runtime supports all standard WebAssembly features, proposed WebAssembly extensions, as well as its own set of extensions specifically designed for cloud and edge applications.

> If you want to WasmEdge applications to access your own native libraries, you can do that by registering a host function from the WasmEdge [C](c_api.md#Host-Functions) or [Go](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_ExternRef) API.

# WebAssembly standard extensions

WasmEdge supports optional WebAssembly features and proposals. Those proposals are likely to become official WebAssembly specifications in the future. WasmEdge supports the following proposals.

* [WASI (WebAssembly Systems Interface) spec](https://github.com/WebAssembly/WASI). WasmEdge has supported the WASI spec for WebAssembly programs to interact with the host Linux operating system securely.
* [Reference Types](https://webassembly.github.io/reference-types/core/). It allows WebAssembly programs to exchange data with host applications and operating systems.
* [Bulk memory operations](https://github.com/WebAssembly/bulk-memory-operations/blob/master/proposals/bulk-memory-operations/Overview.md). The WebAssembly program sees faster memory access and performs better with bulk memory operations.
* [SIMD (Single instruction, multiple data)](https://github.com/second-state/SSVM/blob/master/docs/simd.md). For modern devices with multiple CPU cores, the SIMD allows data processing programs to take advantage of the CPUs fully. SIMD could significantly enhance the performance of data applications.

Meanwhile, the WasmEdge team is [exploring the wasi-socket proposal](https://github.com/second-state/w13e_wasi_socket) to support network access in WebAssembly programs. 

# WasmEdge extensions

A key differentiator of WasmEdge from other WebAssembly VMs is its support for non-standard extensions. The WASI spec provides a mechanism for developers to extend WebAssembly VMs efficiently and securely. The WasmEdge team created the following WASI-like extensions based on real-world customer demands.

* [Tensorflow](https://github.com/second-state/wasmedge-tensorflow). Developers can write Tensorflow inference functions using [a simple Rust API](https://github.com/second-state/wasmedge_tensorflow_interface), and then run the function securely and at native speed inside WasmEdge.
* Other AI frameworks. Besides Tensorflow, the Second State team is building WASI-like extensions for AI frameworks such as ONNX and Tengine for WasmEdge.
* [Image processing](https://github.com/second-state/WasmEdge-image). WasmEdge uses native libraries to manipulate images for computer vision tasks.
* [KV Storage](https://github.com/second-state/wasmedge-storage). The WasmEdge [storage interface](https://github.com/second-state/rust_native_storage_library) allows WebAssembly programs to read and write a key value store.
* [Command interface](https://github.com/second-state/wasmedge_process_interface). WasmEdge enables webassembly functions execute native commands in the host operating system. It supports passing arguments, environment variables, STDIN / STDOUT pipes, and security policies for host access.
* [Ethereum](https://github.com/second-state/wasmedge-evmc). The WasmEdge Ewasm extension supports Ethereum smart contracts compiled to WebAssembly. It is a leading implementation for Ethereum flavored WebAssembly (Ewasm).
* [Substrate](https://github.com/second-state/substrate-ssvm-node). The [Pallet](https://github.com/second-state/pallet-ssvm) allows WasmEdge to act as an Ethereum smart contract execution engine on any Substrate based blockchains.
