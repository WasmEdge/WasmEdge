# Service mesh 与分布式框架

WasmEdge 可以作为 sidecar 微服务的轻量级运行时，也可以作为 Docker 替代方案的 API 代理。

## Sidecar 微服务

对于支持多个应用程序运行时的 sidecar 框架，我们可以通过其 C、Go、Rust 或 Node.js SDK 将 WasmEdge 应用程序简单地嵌入到 sidecar 中。 另外，WasmEdge 应用程序可以直接由容器工具管理并充当 sidecar 微服务。

* [Dapr](mesh/dapr.md) 展示了如何将 WasmEdge 微服务作为 Dapr sidecar 运行。
* [Apache EventMesh](mesh/evenmesh.md) 展示了如何将 WasmEdge 微服务作为 Apache EventMesh sidecar 运行

## API porxy 的扩展

API proxy 是服务网格中的另一个关键组件。 它以保持系统可扩展性的方式管理 API 请求并将其定向到 sidecar。 开发人员需要编写这些代理脚本，以根据不断变化的基础设施和运营要求来路由流量。 看到用户希望使用 WebAssembly 而不是 LUA 脚本语言的广泛需求，社区一起创建了 proxy-wasm 规范。 它定义了 WebAssembly 运行时必须支持的插入 proxy 的主机接口。 WasmEdge 现在已经支持 proxy-wasm。

* [MOSN](mesh/mosn.md) 展示了如何使用 WasmEdge 作为 MOSN 的扩展。

如果你对 WasmEdge 和微服务有一些好的想法，请随时在 [WasmEdge](https://github.com/WasmEdge/WasmEdge) GitHub 存储库上创建问题或 PR！
