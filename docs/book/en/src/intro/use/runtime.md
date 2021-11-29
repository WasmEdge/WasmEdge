# Cloud-native runtime (as a lightweight Docker alternative)

WasmEdge can be embedded into cloud-native infrastructure via its SDKs in [C](c_api.md), [Go](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/), [Rust](../bindings/rust/wasmedge-rs), and [JavaScript](https://www.secondstate.io/articles/getting-started-with-rust-function/). It is also an OCI compliant runtime that can be directly [managed by CRI-O and Docker tools](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/) as a lightweight and high-performance alternative to Docker.

## Dapr (Distributed Application Runtime)

* [Tutorial](https://www.secondstate.io/articles/dapr-wasmedge-webassembly/)
* [Code template](https://github.com/second-state/dapr-wasm)

## Service mesh (work in progress):

* Linkerd
* MOSN
* Envoy

## Orchestration and management:

* [Kubernetes](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/)
* KubeEdge
* SuperEdge
* OpenYurt

