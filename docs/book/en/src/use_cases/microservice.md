# Microservices

The edge cloud can run application logic microservices very close to the client device.

* The microservices could be stateless computational tasks, such as [AI inference](../write_wasm/rust/tensorflow.md) and [stream data analysis](frameworks/app/yomo.md), which offload computation from the client.
* The microservices could also [interact with data cache services](https://github.com/second-state/wasmedge-anna-rs) that sync with backend databases.

The edge cloud has advantages such as low latency, high security, and high performance. Operationally, WasmEdge can be embedded into cloud-native infrastructure via its SDKs in [C](../sdk/c.md), [Go](../sdk/go.md) and [Rust](../sdk/rust.md). It is also an OCI compliant runtime that can be directly [managed by container tools](kubernetes/cri.md) as a lightweight and high-performance alternative to Linux containers. The following application frameworks have been tested to work with WasmEdge-based microservices.

## Dapr (Distributed Application Runtime)

* [Tutorial](frameworks/mesh/dapr.md)
* [Code template](https://github.com/second-state/dapr-wasm)

## Service mesh (work in progress)

* Linkerd
* MOSN
* Envoy

## Orchestration and management

* [Kubernetes](kubernetes.md)
* [KubeEdge](kubernetes/kubernetes/kubeedge.md)
* [SuperEdge](kubernetes/kubernetes/superedge.md)
* [OpenYurt](kubernetes/kubernetes/openyurt.md)
