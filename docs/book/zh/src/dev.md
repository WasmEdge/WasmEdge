# 开发 WasmEdge 应用

WebAssembly 技术关键价值在于它支持多种编程语言。
WebAssembly是一种为多种语言服务的托管运行时，包括 C/C++, Rust, Go,Swift, Kotlin, AssemblyScript, Grain and even JavaScript and Python.

* 对于编译型语言(例如C和Rust), WasmEdge WebAssembly 提供了一个安全、可靠、隔离和容器化的运行时，
而非Native Client.
* 对于解释型或者托管型语言(例如JavaScript和Python), WasmEdge WebAssembly提供了一个安全、快速、轻量和容器化的运行时，
* 而不是Docker+客户系统(guest OS)+本地解释器(native interpreter)。

在本章中，我们将讨论在WasmEdge中用多种语言如何编译、运行程序。