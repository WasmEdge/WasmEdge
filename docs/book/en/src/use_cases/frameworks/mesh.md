# Service mesh and distributed runtimes

WasmEdge could be a lightweight runtime for sidecar microservices and the API proxy as the Docker alternative.

## Sidecar microservices

For sidecar frameworks that support multiple application runtimes, we could simply embed WasmEdge applications into the sidecar through its C, Go, Rust, or Node.js SDKs. In addition, WasmEdge applications could be managed directly by container tools and act as sidecar microservices.

* [Dapr](mesh/dapr.md) showcases how to run WasmEdge microservices as Dapr sidecars.
* [Apache EventMesh](mesh/eventmesh.md) showcases how to run WasmEdge microservices as Apache EventMesh sidecars

## Extension for the API proxy

The API proxy is another crucial component in the service mesh. It manages and directs API requests to sidecars in a manner that keeps the system scalable. Developers need to script those proxies to route traffic according to changing infrastructure and ops requirements. Seeing widespread demand for using WebAssembly instead of the LUA scripting language, the community came together and created the proxy-wasm spec. It defines the host interface that WebAssembly runtimes must support to plug into the proxy. WasmEdge supports proxy-wasm now.

* [MOSN](mesh/mosn.md) shows how to use WasmEdge as extensions for MOSN.
* [wasm-nginx-module](mesh/wasm-nginx-module.md) shows how to use WasmEdge run Go/Rust code in OpenResty.

If you have some great ideas on WasmEdge and microservices, feel free to create an issue or PR on the [WasmEdge](https://github.com/WasmEdge/WasmEdge) GitHub repo!
