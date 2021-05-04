# WasmEdge Ecosystem

![WasmEdge architecture](architecture.png)

## Introduction

**WasmEdge** (formerly SSVM) is a high performance and enterprise-ready WebAssembly (WASM) Virtual Machine for cloud, AI, and Blockchain applications. The `WasmEdge` ecosystem can be separated into the above layers.

* Core: The [WasmEdge core project](https://github.com/WasmEdge/WasmEdge).
* Plug-in: The extensions of `WASM` host functions with their dependencies.
  * [SSVM-TensorFlow](https://github.com/second-state/SSVM-tensorflow) contains the host function extensions which access to [TensorFlow C library](https://www.tensorflow.org/install/lang_c).
  * [SSVM-Image](https://github.com/second-state/SSVM-image) contains the host function extensions about `JPEG` and `PNG` image decodings.
  * [SSVM-Storage](https://github.com/second-state/SSVM-storage) contains the host function extensions which access to [Rust storage library](https://github.com/second-state/rust_native_storage_library).
  * [SSVM-EVMC](https://github.com/second-state/SSVM-evmc) contains the host function extensions that are compatible with [Ethereum Environment Interface](https://github.com/ewasm/design/blob/master/eth_interface.md).
* Tools: The executables or shared libraries.
  * [SSVM-TensorFlow tools](https://github.com/second-state/SSVM-tensorflow/releases) are the released tools to execute WASM with accessing to `TensorFlow` or `TensorFlow-Lite`.
  * [SSVM-Storage static library](https://github.com/second-state/SSVM-storage) is the static library which contains the host extensions that access to [Rust storage library](https://github.com/second-state/rust_native_storage_library).
  * [SSVM-EVMC shared libraries](https://github.com/second-state/SSVM-evmc/releases) are the released shared libraries that are compatible with [EVMC](https://github.com/ethereum/evmc).
* Node.js Addon: The Node.js addon to access `SSVM`.
  * [SSVM-napi](https://github.com/second-state/SSVM-napi) is the Node.js addon project for WASM functions accessing.
  * [SSVM-napi-extensions](https://github.com/second-state/SSVM-napi-extensions) is the Node.js addon project for WASM runtime with `ssvm-tensorflow` and `ssvm-storage` extensions.

## Releasing Steps

1. [WasmEdge core](https://github.com/WasmEdge/WasmEdge) Releases.
2. Release a new version of plug-ins, tools, and libraries with updating to the new `WasmEdge core`.
    * [SSVM-Image](https://github.com/second-state/SSVM-image)
    * [SSVM-TensorFlow](https://github.com/second-state/SSVM-tensorflow)
    * [SSVM-Storage](https://github.com/second-state/SSVM-storage)
    * [SSVM-EVMC](https://github.com/second-state/SSVM-evmc)
3. Release [SSVM-napi](https://github.com/second-state/SSVM-napi) with updating to new `WasmEdge core`.
4. Release [SSVM-napi-extensions](https://github.com/second-state/SSVM-napi-extensions) with updating to new `WasmEdge core` and `SSVM-napi`.
