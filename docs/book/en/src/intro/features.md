# Features

* One of the [fastest](https://github.com/WasmEdge/WasmEdge#performance) WebAssembly VMs on the market (based on **LLVM AOT**)
* WasmEdge feature extensions
  * **Network sockets** ([Rust](https://github.com/second-state/wasmedge_wasi_socket) and [JavaScript](https://github.com/second-state/wasmedge-quickjs#http-request) SDKs)
  * **Async polling** (for Rust Future and JS async)
  * Tensorflow inference ([Tutorial](https://www.secondstate.io/articles/wasi-tensorflow/))
  * Key value storage
  * Database connector
  * **Gas meters** for resource constraints
* JavaScript support
  * [ES6 module](https://github.com/second-state/wasmedge-quickjs#es6-module-support) and std API support
  * **Implement JS APIs in Rust** ([Tutorial](https://www.secondstate.io/articles/embed-rust-in-javascript/))
  * Import C native shared library functions as JS functions ([Tutorial](https://www.secondstate.io/articles/call-native-functions-from-javascript/))
* Cloud native management and orchestration
  * [CRI-O and Kubernetes compatibility](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/)
  * Sidecar apps in Kubernetes-based service meshes
  * Dapr microservices ([Tutorial](https://www.secondstate.io/articles/dapr-wasmedge-webassembly/))
* Cross-platform support
  * Linux OSes dated back to 2010 for both x86 and ARM CPUs
  * Mac OS X for both x86 and m1 CPUs
  * Windows
  * Microkernel and RTOS (e.g., the highly secure [seL4 microkernel](https://github.com/second-state/wasmedge-sel4))
* Easy extensibility
  * Build customized runtimes with native functions in [C](../embed/c/ref.md#host-functions) or [GO](../embed/go/ref.md#host-functions)
* Easy to embed into a host application
  * Embed WasmEdge functions in [C](../embed/c.md), [Go](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/), [Rust](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust), [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) and Python host applications
  * Embedded function runtime for service mesh proxies (e.g., [proxy-wasm](https://github.com/proxy-wasm/proxy-wasm-cpp-host/pull/193) for Envoy and MOSN proxies)
