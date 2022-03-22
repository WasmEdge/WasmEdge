# Serverless 平台

在云原生基础设施中，我们希望将 WebAssembly 作为轻量级运行时的替代方案，与 Docker 和 microVMs 一同使用。WebAssembly 提供了比类似 Docker 的容器或 microVMs 更高的性能和更少的资源消耗。然而，公有云只支持在 microVM 中运行 WebAssembly。尽管如此，在 microVM 中运行 WebAssembly 函数仍然比与运行容器化的 NaCI 程序有诸多优势。

与直接在 Docker 容器中运行 NaCI 程序相比，在类似 Docker 的容器中运行 WebAssembly 函数具有如下优势：

**首先**，WebAssembly 为单个函数提供细粒度的运行时隔离。一个微服务可以在一个类似 Docker 的容器中运行多个函数和支持服务。WebAssembly 可以使微服务更安全、更稳定。

**第二点**，WebAssembly 字节码是可移植的。开发人员只需构建一次，无需担心底层 Vercel serverless 容器（操作系统和硬件）的修改或更新。它还允许开发人员在其他云环境中复用相同的 WebAssembly 函数功能。

**第三点**，WebAssembly 应用程序易于部署和管理。与 NaCI 动态库和可执行文件相比，它们的平台依赖性和复杂性要少得多。

**最后**，[WasmEdge Tensorflow API](https://www.secondstate.io/articles/wasi-tensorflow/) 提供了用 Rust 语言编写的示例，以最符合人类的思维方式来执行 Tensorflow 模型。WasmEdge 安装了使用 Tensorflow 所需的依赖库，并为开发人员提供了统一的 API。

在本节中，我们将向你展示如何在公有云中运行 WebAssembly serverless 函数。每个平台都有其对应的代码模板，其中包含两个 Rust 语言示例，一个是正常的图像处理，另一个是使用 WasmEdge TensorFlow SDK 的 TensorFlow 推理。

* [Vercel](serverless/vercel.md) 如何利用 WasmEdge 加载在 Vercel 上部署的 Jamstack 应用程序
* [Netlify](serverless/netlify.md) 如何利用 WasmEdge 加载在 Netlify 上部署的 Jamstack 应用程序
* [AWS Lambda](serverless/aws.md) 如何利用 WasmEdge 加载在 AWS Lambda 上部署的 serverless 函数
* [Tencent](serverless/tencent.md) 如何利用 WasmEdge 加载在腾讯云上部署的 serverless 函数

> 如果你想在公有云平台上添加更多 WasmEdge 示例，比如谷歌云，请随时为 WasmEdge 创建一个 PR，并让社区知道你做了什么。

![serverless-wasmedge.png](serverless-wasmedge.png)

在公有云上的 Docker 容器中运行 WasmEdge，是 web 应用程序提高性能的一种简单方法。展望未来，更好的方法是使用 [WasmEdge 作为容器](https://www.computer.org/csdl/magazine/so/5555/01/09214403/1nHNGfu2Ypi)。不需要 Docker 和 Node.js 来引入 WasmEdge。这样，我们可以更加高效的执行 serverless 函数。

* [Second State Functions](serverless/secondstate.md) 将讨论如何使用 WasmEdge 作为容器，因为 Second State Functions 是一个纯 WebAssembly/WasmEdge 的 serverless 平台。
