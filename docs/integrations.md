WasmEdge is a "serverless" runtime for cloud native and edge computing applications. It allows developers safely embed third-party or "native" functions into a host application or a distributed computing framework.

# Embed WasmEdge into a host application

A major use case of WasmEdge is to start an VM instance from a host application. Depending on your host application's programming language, you can use WasmEdge SDKs to start and call WasmEdge functions.

* Embed WasmEdge functions into a C-based application using the [WasmEdge C API](docs/c_api.md). Checkout the [quick start guide](docs/c_api_quick_start.md).
* Embed WasmEdge functions into a Go application using the [WasmEdge Go API](https://github.com/second-state/WasmEdge-go). Here is a [tutorial](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/) and are some [examples](https://github.com/second-state/WasmEdge-go-examples)!
* Embed WasmEdge functions into a Rust application using the [WasmEdge Rust crate](../wasmedge-rs).
* Embed WasmEdge functions into a Node.js application using the NAPI. Here is a [tutorial](https://www.secondstate.io/articles/getting-started-with-rust-function/).

However, the WebAssembly spec only supports very limited data types as input parameters and return values for the WebAssembly bytecode functions. In order to pass complex data types, such as a string of an array, as call arguments into Rust-based WasmEdge function, you should use the bindgen solution provided by the [rustwasmc](https://github.com/second-state/rustwasmc) toolchain.

We currently supports bindgen in the [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) and in [Go](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/). We are working on [supporting interface types](https://github.com/WasmEdge/WasmEdge/issues/264) in place of bindgen for future releases.

# Use WasmEdge as a Docker-like container



# Call native host functions from WasmEdge

Sometimes, the Wasm bytecode alone could prove too limiting for some applications. WasmEdge provides a [host function API](https://github.com/WasmEdge/WasmEdge/blob/master/docs/host_function.md) that allows Wasm bytecode programs to load and call native library functions from the underlying host operating system.

Alternatively, you can register external functions as callbacks from the WasmEdge runtime. [Here is an example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_ExternRef) to register external functions in Go.

> The host functions break the Wasm sandbox. But the sandbox breaking is done with explicit permission from the system’s operator.

In fact, the extensions to WasmEdge are implemented using native host functions. For example, the [Tensorflow extension](https://www.secondstate.io/articles/wasi-tensorflow/) allows Wasm bytecode to make calls to the native Tensorflow library functions.

