# SSVM Ecosystem

![SSVM architecture](architecture.png)

## Instroduction

**SSVM** is a high performance and enterprise-ready WebAssembly (WASM) Virtual Machine for cloud, AI, and Blockchain applications. The `SSVM` ecosystem can be separated into the above layers.

* Core: The [SSVM core project](https://github.com/second-state/ssvm).
* Plug-in: The extensions of `WASM` host functions with their dependencies.
  * [SSVM-TensorFlow](https://github.com/second-state/ssvm-tensorflow) contains the host function extensions which access to [TensorFlow C library](https://www.tensorflow.org/install/lang_c).
  * [SSVM-Storage](https://github.com/second-state/ssvm-storage) contains the host function extensions which access to [Rust storage library](https://github.com/second-state/rust_native_storage_library).
  * [SSVM-EVMC](https://github.com/second-state/ssvm-evmc) contains the host function extensions that are compatible with [Ethereum Environment Interface](https://github.com/ewasm/design/blob/master/eth_interface.md).
* Tools: The executables or shared libraries.
  * [SSVM-TensorFlow tools](https://github.com/second-state/ssvm-tensorflow/releases) are the released tools to execute WASM with accessing to `TensorFlow` or `TensorFlow-Lite`.
  * [SSVM-Storage static library](https://github.com/second-state/ssvm-storage) is the static library which contains the host extensions that access to [Rust storage library](https://github.com/second-state/rust_native_storage_library).
  * [SSVM-EVMC shared libraries](https://github.com/second-state/ssvm-evmc/releases) are the released shared libraries that are compatible with [EVMC](https://github.com/ethereum/evmc).
* Node.js Addon: The Node.js addon to access `SSVM`.
  * [SSVM-napi](https://github.com/second-state/ssvm-napi) is the Node.js addon project for WASM functions accessing.
  * [SSVM-napi-extensions](https://github.com/second-state/ssvm-napi-extensions) is the Node.js addon project for WASM runtime with `ssvm-tensorflow` and `ssvm-storage` extensions.

## Releasing Steps

1. [SSVM core](https://github.com/second-state/ssvm) Releases.
2. Release a new version of tools and libraries with updating to the new `SSVM core`.
    * [SSVM-TensorFlow](https://github.com/second-state/ssvm-tensorflow)
    * [SSVM-Storage](https://github.com/second-state/ssvm-storage)
    * [SSVM-EVMC](https://github.com/second-state/ssvm-evmc)
3. Release [SSVM-napi](https://github.com/second-state/ssvm-napi) with updating to new `SSVM core`.
4. Release [SSVM-napi-extensions](https://github.com/second-state/ssvm-napi-extensions) with updating to new `SSVM core` and `SSVM-napi`.
