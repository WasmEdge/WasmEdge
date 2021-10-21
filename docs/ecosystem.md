# WasmEdge Ecosystem

![WasmEdge architecture](architecture.png)

## Introduction

**WasmEdge** (formerly SSVM) is a high performance and enterprise-ready WebAssembly (WASM) Virtual Machine for cloud, AI, and Blockchain applications. The `WasmEdge` ecosystem can be separated into the above layers.

* Core: The [WasmEdge core project](https://github.com/WasmEdge/WasmEdge).
* Plug-ins: The extensions of `WASM` host functions with their dependencies.
  * [WasmEdge-TensorFlow](https://github.com/second-state/WasmEdge-tensorflow) contains the host function extensions which access to [TensorFlow C library](https://www.tensorflow.org/install/lang_c).
  * [WasmEdge-Image](https://github.com/second-state/WasmEdge-image) contains the host function extensions about `JPEG` and `PNG` image decodings.
  * [WasmEdge-Storage](https://github.com/second-state/WasmEdge-storage) contains the host function extensions which access to [Rust storage library](https://github.com/second-state/rust_native_storage_library).
  * [WasmEdge-EVMC](https://github.com/second-state/WasmEdge-evmc) contains the host function extensions that are compatible with [Ethereum Environment Interface](https://github.com/ewasm/design/blob/master/eth_interface.md).
* Tools: The runtime executables.
  * [WasmEdge-TensorFlow-Tools](https://github.com/second-state/WasmEdge-tensorflow-tools) are the released tools to execute WASM with accessing to `TensorFlow` or `TensorFlow-Lite`.
* Language supports: The `WasmEdge` triggering in other languages.
  * The [C API](c_api.md) is embedded in the core release as a header file and shared library.
  * [WasmEdge-core NAPI package](https://github.com/second-state/wasmedge-core) is the Node.js addon project for `WASM` functions.
  * [WasmEdge-extensions NAPI package](https://github.com/second-state/wasmedge-extensions) is the Node.js addon project for `WASM` runtime with `wasmedge-tensorflow`, `wasmedge-image`, and `wasmedge-storage` extensions.
  * [WasmEdge-go](https://github.com/second-state/WasmEdge-go) is the [Golang](https://golang.org/) binding for `WasmEdge` C API.
  * [WasmEdge-rs](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust) is the Rust binding for `WasmEdge` C API.

## Releasing Steps

1. [WasmEdge](https://github.com/WasmEdge/WasmEdge) RC pre-releases.
    * In this step, the main features are completed and pre-preleases for the bugs fixing and testing.
    * No more important feature will be merged after the first RC pre-release in this version.
2. Extensions RC pre-releases.
    * In this step, the host function extensions and shared libraries based on the `WasmEdge` will update the dependency.
    * The pre-release tags of extensions should be matched to the `WasmEdge` pre-release tags.
    * The following projects will be updated:
      * [WasmEdge-Image](https://github.com/second-state/WasmEdge-image)
      * [WasmEdge-TensorFlow-Deps](https://github.com/second-state/WasmEdge-tensorflow-deps)
      * [WasmEdge-TensorFlow](https://github.com/second-state/WasmEdge-tensorflow)
    * If there are issues or bugs in the `WasmEdge` or the above extensions, return to the first step and create a new RC pre-release.
3. Release a new `WasmEdge` version.
    * After fixing the issues, the official `WasmEdge` version will be released.
4. Release the extensions.
    * After fixing the issues and releasing `WasmEdge`, the official version of `WasmEde-Image`, `WasmEdge-TensorFlow-Deps`, and `WasmEdge-TensorFlow` will be released.
    * From this step, users can get the `WasmEdge` and the extensions through the installer.
5. Release other tools, language bindings, and side-projects.
    * Release a new version of tools.
      * [WasmEdge-TensorFlow-Tools](https://github.com/second-state/WasmEdge-tensorflow-tools)
    * Release a new version of [Go SDK](https://github.com/second-state/WasmEdge-go)
    * Release a new version of [Rust crate](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust)
    * Release [WasmEdge-core NAPI package](https://github.com/second-state/wasmedge-core) with updating to the new `WasmEdge`.
    * Release [WasmEdge-extensions NAPI package](https://github.com/second-state/wasmedge-extensions) with updating to the new extensions.
