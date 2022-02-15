# 开发 WasmEdge 应用

WebAssembly 技术关键价值在于它支持多种编程语言。 WebAssembly 是一种为多种语言服务的托管运行时，包括 C/C++， Rust， Go， Swift， Kotlin， AssemblyScript， Grain， JavaScript 和 Python。

* 对于编译型语言(例如 C 和 Rust), WasmEdge WebAssembly 提供了一个安全、可靠、隔离和容器化的运行时，而非 Native Client。
* 对于解释型或者托管型语言(例如 JavaScript 和 Python), WasmEdge WebAssembly 提供了一个安全、快速、轻量和容器化的运行时，而不是 Docker + guest OS + 本地解释器(native interpreter)。

在本章中，我们将讨论在 WasmEdge 中用多种语言如何编译、运行程序。