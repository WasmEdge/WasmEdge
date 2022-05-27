# 应用框架

WasmEdge 为应用程序提供安全高效的扩展机制。当然，应用程序开发人员随时可以使用 [WasmEdge SDK](https://github.com/WasmEdge/WasmEdge/blob/master/docs/book/en/src/embed.md) 来嵌入 WebAssembly 函数。但有些应用程序和框架选择在 WasmEdge SDK 之上构建扩展/嵌入 API，后者支持与应用程序的本地应用场景和编程模型进行更符合人体工程学的集成。

* [YoMo](https://github.com/WasmEdge/WasmEdge/blob/master/docs/book/en/src/frameworks/app/yomo.md) 是一个数据流处理框架。WasmEdge 函数可以插入到框架中来处理流中的数据。
* [Reactr](https://github.com/WasmEdge/WasmEdge/blob/master/docs/book/en/src/frameworks/app/reactr.md) 是一个 Go 语言框架，用于管理和扩展 WebAssembly 函数，以便轻松嵌入到其他 Go 应用程序中。
