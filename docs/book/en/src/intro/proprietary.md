# WasmEdge extensions

A key differentiator of WasmEdge from other WebAssembly VMs is its support for non-standard extensions. The WASI spec provides a mechanism for developers to extend WebAssembly VMs efficiently and securely. The WasmEdge team created the following WASI-like extensions based on real-world customer demands.

* [Tensorflow](https://github.com/second-state/wasmedge-tensorflow). Developers can write Tensorflow inference functions using [a simple Rust API](https://github.com/second-state/wasmedge_tensorflow_interface), and then run the function securely and at native speed inside WasmEdge.
* Other AI frameworks. Besides Tensorflow, the Second State team is building WASI-like extensions for AI frameworks such as ONNX and Tengine for WasmEdge.
* [Image processing](https://github.com/second-state/WasmEdge-image). WasmEdge uses native libraries to manipulate images for computer vision tasks.
* [KV Storage](https://github.com/second-state/wasmedge-storage). The WasmEdge [storage interface](https://github.com/second-state/rust_native_storage_library) allows WebAssembly programs to read and write a key value store.
* [Network sockets](https://github.com/second-state/wasmedge_wasi_socket). WasmEdge applications can access the network sockets for [TCP and HTTP connections](https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples).
* [Command interface](https://github.com/second-state/wasmedge_process_interface). WasmEdge enables webassembly functions execute native commands in the host operating system. It supports passing arguments, environment variables, STDIN / STDOUT pipes, and security policies for host access.
* [Ethereum](https://github.com/second-state/wasmedge-evmc). The WasmEdge Ewasm extension supports Ethereum smart contracts compiled to WebAssembly. It is a leading implementation for Ethereum flavored WebAssembly (Ewasm).
* [Substrate](https://github.com/second-state/substrate-ssvm-node). The [Pallet](https://github.com/second-state/pallet-ssvm) allows WasmEdge to act as an Ethereum smart contract execution engine on any Substrate based blockchains.
