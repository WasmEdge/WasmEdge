# JavaScript

WebAssembly 最初是一种“浏览器的 JavaScript 替代品”。它的想法是在浏览器中安全地运行从 C/C++ 或 Rust 等语言编译的高性能程序。WebAssembly 与 JavaScript 在浏览器中并行运行。

随着 WebAssembly 在云服务中广泛使用，它变成了云原生程序的一种通用运行时。同 Linux 容器相比， WebAssembly 运行时以更少的资源消耗获得了更好的性能。

在云原生应用实例中，开发者想要使用 JavaScript 去编写业务程序。这意味着必须在 WebAssembly 中支持 JavaScript。此外，我们还需要支持在 WebAssembly 运行时中从 JavaScript 中调用 C/C++ 和 Rust 的函数，以便利用 WebAssembly 的计算效率。WasmEdge WebAssembly 运行时支持这样的操作。

<center>

![](javascript.png)

</center>

在这一章节，我们将演示如何在 WasmEdge 中运行 JavaScript 并增强(它的性能)。

* [起步](js/quickstart.md) 描述了怎样在 WasmEdge 中运行简单的 JavaScript 程序。
* [ES6 模块](js/es6.md) 展示了怎样在 WasmEdge 中运行 ES6 模块。
* [CommonJS 模块](js/cjs.md) 展示了怎样在 WasmEdge 中运行 CommonJS 模块。
* [NodeJS 和 NPM 模块](js/npm.md) 展示了怎样在 WasmEdge 中运行 NPM 模块。 
* [React SSR](js/ssr.md) 展示了在 WasmEdge 中一个 React SSR 应用实例。
* [TensorFlow](js/tensorflow.md) 展示了怎样通过 JavaScript API 去使用 WasmEdge 的 Tensorflow 扩展。
* [网络 sockets](js/networking.md) 展示了如何使用 WasmEdge 网络扩展及其提供的 JavaScript API 去构建 HTTP 客户端和服务端程序。
* [异步网络](js/async.md) 展示了怎样使用异步非阻塞 I/O 提高 HTTP 服务器性能。 
* [使用 Rust 实现 JS API](js/rust.md) 讨论了怎样使用 Rust 实现并支持在 WasmEdge 中的 JavaScript API。

## 关于 v8 的说明

现在选择 QuickJS 作为我们的 JavaScript 引擎可能引发性能的问题。由于缺乏 JIT 支持， QuickJS 是否比 v8 [慢很多](https://bellard.org/quickjs/bench.html) ？是的，但是我想说说选择 QuickJS 的理由。

第一点，QuickJS 比 v8 资源消耗小很多。事实上，它仅仅占用 v8 所消耗资源的 1/40（或 2.5%)。你可以在单机上运行比 v8 函数更多的 QuickJS 函数。

第二点，对于大多数业务逻辑应用，原生性能并不关键。应用程序可能具有计算密集型任务，比如 AI 动态推理。 WasmEdge 允许 QuickJS 应用使用高性能 WebAssembly 来完成这些任务， 而使用 v8 添加这些扩展模块并不容易。

第三点， WasmEdge 是[一种 OCI 兼容的容器](../kubernetes.md)。它默认安全，支持资源隔离，并且可以通过容器管理工具在单个 k8s 集群中与 Linux 容器上并行。

最后， v8 攻击面广泛，因此需要付出[大量努力](https://blog.cloudflare.com/mitigating-spectre-and-other-security-threats-the-cloudflare-workers-security-model/)才能使其安全运行在公共云环境上。众所周知，[许多 JavaScript 安全问题由 JIT 引起](https://www.theregister.com/2021/08/06/edge_super_duper_security_mode/) 。所以，也许在云原生环境下关闭 JIT 未必是坏事！

总而言之，在云原生环境中运行 v8 经常需要 “Linux 容器 + guest OS + node(或 deno) + v8” 全套软件工具，这使得它比 WasmEdge + QuickJS 容器运行时更重、更慢。





