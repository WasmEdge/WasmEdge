# 雲端原生 runtime （作為 Docker 的輕量級替代方案）

WasmEdge 可以透過其 [C](../../embed/c.md) 、 [Go](../../embed/go.md) 、 [Rust](../../embed/rust.md) 和[JavaScript](../../embed/node.md)的 SDK 嵌入到雲端原生基礎架構中。它也是一個符合 OCI 的 runtime ，可以由 [CRI-O 和 Docker 工具直接管理](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/) ，作為 Docker 的輕量級和高效能替代方案。

## Dapr （分散式應用程式 Runtime）

* [教學](https://www.secondstate.io/articles/dapr-wasmedge-webassembly/)
* [程式碼教學](https://github.com/second-state/dapr-wasm)

## Service mesh （開發進行中）

* Linkerd
* MOSN
* Envoy

## 編排和管理

* [Kubernetes](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/)
* KubeEdge
* SuperEdge
* OpenYurt
