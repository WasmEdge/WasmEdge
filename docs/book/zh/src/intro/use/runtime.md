# 云原生的 runtime

WasmEdge 可以通过其 [C](../../embed/c.md), [Go](../../embed/go.md), [Rust](../../embed/rust.md), 和 [JavaScript](../../embed/node.md) 的 SDK 嵌入到云原生基础设施中。它也是一个符合 OCI 的 runtime ，可以由 [CRI-O 和 Docker 工具直接管理](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/) ，作为 Docker 的轻量级和高性能替代。

## Dapr (分布式应用 Runtime)

* [教程](https://www.secondstate.io/articles/dapr-wasmedge-webassembly/)
* [代码教程](https://github.com/second-state/dapr-wasm)

## Service mesh (开发进行中)

* Linkerd
* MOSN
* Envoy

## 编排和管理

* [Kubernetes](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/)
* KubeEdge
* SuperEdge
* OpenYurt
