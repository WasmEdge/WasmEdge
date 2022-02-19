# WasmEdge integrations

WasmEdge is a "serverless" runtime for cloud native and edge computing applications. It allows developers safely embed third-party or "native" functions into a host application or a distributed computing framework.

## Embed WasmEdge into a host application

A major use case of WasmEdge is to start an VM instance from a host application. Depending on your host application's programming language, you can use WasmEdge SDKs to start and call WasmEdge functions.

* Embed WasmEdge functions into a C-based application using the [WasmEdge C API](../embed/c/ref.md). Checkout the [quick start guide](../embed/c.md).
* Embed WasmEdge functions into a Go application using the [WasmEdge Go API](../embed/go.md). Here is a [tutorial](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/) and are some [examples](https://github.com/second-state/WasmEdge-go-examples)!
* Embed WasmEdge functions into a Rust application using the [WasmEdge Rust crate](../bindings/rust/wasmedge-rs).
* Embed WasmEdge functions into a Node.js application using the NAPI. Here is a [tutorial](https://www.secondstate.io/articles/getting-started-with-rust-function/).
* Embed WasmEdge functions into any application by spawning a new process. See examples for [Vercel Serverless Functions](https://www.secondstate.io/articles/vercel-wasmedge-webassembly-rust/) and [AWS Lambda](https://www.cncf.io/blog/2021/08/25/webassembly-serverless-functions-in-aws-lambda/).

However, the WebAssembly spec only supports very limited data types as input parameters and return values for the WebAssembly bytecode functions. In order to pass complex data types, such as a string of an array, as call arguments into Rust-based WasmEdge function, you should use the bindgen solution provided by the [rustwasmc](https://github.com/second-state/rustwasmc) toolchain. We currently supports bindgen in the [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) and in [Go](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/). We are working on [supporting interface types](https://github.com/WasmEdge/WasmEdge/issues/264) in place of bindgen for future releases.

## Use WasmEdge as a Docker-like container

WasmEdge provides an OCI compliant interface. You can use container tools, such as CRI-O, Docker Hub, and Kubernetes, to orchestrate and manage WasmEdge runtimes.

* [Manage WasmEdge with CRI-O and Docker Hub](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/).

## Call native host functions from WasmEdge

A key feature of WasmEdge is its extensibility. WasmEdge APIs allow developers to register "host functions" from any shared library into a WasmEdge instance, and then call these functions from the WebAssembly bytecode program.

* The WasmEdge C API supports the [C host functions](../embed/c/ref.md#host-functions).
* The WasmEdge Go API supports the [Go host functions](../embed/go/ref.md#host-functions).

[Here is an example](https://www.secondstate.io/articles/call-native-functions-from-javascript/) of a JavaScript program in WasmEdge calling a C-based host function in the underlying OS.

The host functions break the Wasm sandbox to access the underly OS or hardware. But the sandbox breaking is done with explicit permission from the system’s operator.
