# 开发 WasmEdge 应用

WebAssembly 的一个关键点就是它支持多种编程语言。WebAssembly 是一个可控的运行时，支持多种编程语言，包含 C/C++、 Rust、 Go、 Swift、 Kotlin、 AssemblyScript、 Grain，甚至还有 JavaScript 和 Python。

* 对于编译型语言（比如 C 和 Rust）来说，WasmEdge WebAssembly 提供了一个相比于原生客户端（NaCl）更安全的、受保护的、隔离的并且容器化的运行时。
* 对于解释型语言或者是受控型语言（比如 JavaScript 和 Python）来说，WasmEdge WebAssembly 提供了一个比 Docker + 客人操作系统（Guest OS） + 原生解释器这种组合更安全、快速、轻量且容器化的运行时。

在这一章中，我们将讨论如何在 WasmEdge 中运行由不同语言编写的应用程序。
