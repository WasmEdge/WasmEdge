# WasmEdge Proprietary Extensions

A key differentiator of WasmEdge from other WebAssembly runtimes is its support for non-standard extensions. The [WebAssembly System Interface (WASI)](https://github.com/WebAssembly/WASI) provides a mechanism for developers to extend WebAssembly efficiently and securely. The WasmEdge team created the following WASI-like extensions based on real-world customer demands.

* [Tensorflow](https://github.com/second-state/wasmedge-tensorflow). Developers can [write Tensorflow inference functions](https://www.secondstate.io/articles/wasi-tensorflow/) using [a simple Rust API](https://github.com/second-state/wasmedge_tensorflow_interface), and then run the function securely and at native speed inside WasmEdge.
* [Image processing](https://github.com/second-state/WasmEdge-image). WasmEdge uses native libraries to manipulate images for computer vision tasks.
* [KV Storage](https://github.com/second-state/wasmedge-storage). The WasmEdge [storage interface](https://github.com/second-state/rust_native_storage_library) allows WebAssembly programs to read and write a key value store.
* [Network sockets](https://github.com/second-state/wasmedge_wasi_socket). WasmEdge applications can access the network sockets for [TCP and HTTP connections](https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples).
* [Command interface](https://github.com/second-state/wasmedge_process_interface). WasmEdge enables webassembly functions execute native commands in the host operating system. It supports passing arguments, environment variables, `STDIN`/`STDOUT` pipes, and security policies for host access.
* [Ethereum](https://github.com/second-state/wasmedge-evmc). The WasmEdge Ewasm extension supports Ethereum smart contracts compiled to WebAssembly. It is a leading implementation for Ethereum flavored WebAssembly (Ewasm).
* [Substrate](https://github.com/second-state/substrate-ssvm-node). The [Pallet](https://github.com/second-state/pallet-ssvm) allows WasmEdge to act as an Ethereum smart contract execution engine on any Substrate based blockchains.

## Extension Supported Platforms

| Extension                     | Description                                             | `x86_64 Linux` | `aarch64 Linux` | `arm64 Android` | `x86_64 Darwin` |
| ----------------------------- | ------------------------------------------------------- | -------------- | --------------- | --------------- | --------------- |
| [WasmEdge-Image][]            | Image host function extension with shared library.      | since `0.9.0`  | since `0.9.1`   | since `0.9.1`   | since `0.10.0`  |
| [WasmEdge-Tensorflow][]       | TensorFlow host function extension with shared library. | TensorFlow and TensorFlow-Lite since `0.9.0` | TensorFlow-Lite since `0.9.1` | TensorFlow-Lite since `0.9.1` | TensorFlow and TensorFlow-Lite since `0.10.0` |
| [WasmEdge-Tensorflow-Tools][] | WasmEdge CLI tools with TensorFlow and image extension. | since `0.9.0`  | since `0.9.1`   | since `0.9.1`   | since `0.10.0`  |

[WasmEdge-Image]: https://github.com/second-state/wasmedge-image
[WasmEdge-Tensorflow]: https://github.com/second-state/wasmedge-tensorflow
[WasmEdge-Tensorflow-Tools]: https://github.com/second-state/wasmedge-tensorflow-tools
