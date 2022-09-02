# App frameworks

WasmEdge provides a safe and efficient extension mechanism for applications.
Of course, application developers can always use [WasmEdge SDKs](../../sdk.md) to embed WebAssembly functions. But some applications and frameworks opt to build their own extension / embedding APIs on top of the WasmEdge SDK, which supports more ergonomic integration with the application's native use cases and programming models.

* [YoMo](app/yomo.md) is a data stream processing framework. WasmEdge functions can be plugged into the framework to process data in-stream.
* [Reactr](app/reactr.md) is a Go language framework for managing and extending WebAssembly functions for the purpose of easy embedding into other Go applications.
