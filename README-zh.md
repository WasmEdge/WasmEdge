<div align="right">

  [Readme in English](README.md) | [正體中文文件](README-zh-TW.md) | [日本語で読む](README-ja.md)

</div>

<div align="center">

![WasmEdge Logo](/docs/wasmedge-runtime-logo.png)

WasmEdge (之前名为 SSVM) 是为边缘计算优化的轻量级、高性能、可扩展的 WebAssembly (Wasm) 虚拟机，可用于云原生、边缘和去中心化的应用。WasmEdge 是目前市场上 [最快的 Wasm 虚拟机](https://ieeexplore.ieee.org/document/9214403)。WasmEdge 是由 [CNCF](https://www.cncf.io/) (Cloud Native Computing Foundation 云原生计算基金会)托管的官方沙箱项目。其[应用场景](https://wasmedge.org/docs/zh/start/usage/use-cases)包括 serverless apps, 嵌入式函数、微服务、智能合约和 IoT 设备。

[![build](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml?query=event%3Apush++branch%3Amaster)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![CodeQL](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml?query=event%3Apush++branch%3Amaster)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/5059/badge)](https://bestpractices.coreinfrastructure.org/projects/5059)

</div>

# 快速开始指引

🚀 [安装](https://wasmedge.org/docs/zh/start/install) WasmEdge\
👷🏻‍♂️ [Build](https://wasmedge.org/docs/zh/category/build-wasmedge-from-source) 并[贡献](https://wasmedge.org/docs/zh/contribute/)给 WasmEdge\
⌨️ [从 CLI 跑](https://wasmedge.org/docs/zh/category/running-with-wasmedge)一个独立的 Wasm 程序或 [JavaScript 程序](https://wasmedge.org/docs/zh/category/develop-wasm-apps-in-javascript) \
🤖 通过 [LlamaEdge](https://github.com/LlamaEdge/LlamaEdge) 与开源 LLM [聊天](https://llamaedge.com/docs/intro) \
🔌 嵌入一个 Wasm 函数在你的 [Go语言](https://wasmedge.org/docs/zh/category/go-sdk-for-embedding-wasmedge)、[Rust](https://wasmedge.org/docs/zh/category/rust-sdk-for-embedding-wasmedge) 或 [C](https://wasmedge.org/docs/zh/category/c-sdk-for-embedding-wasmedge) 应用里 \
🛠 使用 [Kubernetes](https://wasmedge.org/docs/zh/category/deploy-wasmedge-apps-in-kubernetes)、[数据流框架](https://wasmedge.org/docs/embed/use-case/yomo), 和 [区块链](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a) 管理和编排 Wasm runtimes \
📚 **[查看我们的官方文档](https://wasmedge.org/docs/)**

# 介绍

WasmEdge Runtime为其执行的 Wasm 字节码程序提供了一个有良好定义的执行沙箱。本 Runtime 为操作系统资源（例如，文件系统、sockets、环境变量、进程）和内存空间提供隔离和保护。 WasmEdge 最重要的用例是作为软件产品（例如，SaaS、软件定义的汽车、边缘节点，甚至区块链节点）中的插件安全地执行用户定义或社区贡献的代码。 它使第三方开发者、软件供应商和社区成员能够扩展和定制软件产品。

<div align="center">

**查看 WasmEdge 的[应用场景](https://wasmedge.org/docs/zh/contribute/users)。**

</div>

## 性能

* 论文：[高性能 Serverless 计算的轻量级设计](https://arxiv.org/abs/2010.07115)，发布于 IEEE Software, 2021年1月。 [https://arxiv.org/abs/2010.07115](https://arxiv.org/abs/2010.07115)
* 文章：[Performance Analysis for Arm vs. x86 CPUs in the Cloud](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)，发布于 infoQ.com, 2021年1月。 [https://www.infoq.com/articles/arm-vs-x86-cloud-performance/](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)

## 特性

WasmEdge 可以运行从 C/C++、Rust、Swift、AssemblyScript 或 Kotlin 源代码编译的标准 WebAssembly 字节码程序。 它还通过嵌入式 [QuickJS 引擎](https://github.com/second-state/wasmedge-quickjs)[运行 JavaScript](https://wasmedge.org/docs/zh/category/develop-wasm-apps-in-javascript)。 WasmEdge 支持所有标准的 WebAssembly 特性和提议的扩展。 它还支持许多为云原生和边缘计算用途量身定制的扩展（例如，[WasmEdge Tensorflow 扩展](https://www.secondstate.io/articles/wasi-tensorflow/)）。

* [WebAssembly 标准扩展](docs/extensions.md#webassembly-standard-extensions)
* [WasmEdge 扩展](docs/extensions.md#wasmedge-extensions)

WebAssembly 的 WasmEdge 扩展通常作为 Rust SDK 或 [JavaScript APIs](docs/run_javascript.md) 提供给开发者。

## 集成

WasmEdge 及其包含的 wasm 程序可以作为新进程或从现有进程从 CLI 启动。 如果从现有进程（例如，从正在运行的 [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) 或 [Go](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/) 或 [Rust](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust) 程序）启动，WasmEdge 将简单地作为函数在进程内运行。 目前，WasmEdge 还不是线程安全的。 为了在您自己的应用程序或云原生框架中使 WasmEdge，请参考以下指南。

* [将 WasmEdge 嵌入 host 应用](https://wasmedge.org/docs/zh/embed/overview)
* [使用容器工具管理和编排 Wasm 实例](https://wasmedge.org/docs/zh/category/deploy-wasmedge-apps-in-kubernetes)
* [从 WasmEdge 调用原生 host 程序](docs/integrations.md#call-native-host-functions-from-wasmedge)

# 社区

## 贡献

我们欢迎社区的贡献！请查看我们的：
- [贡献指南](./docs/CONTRIBUTING.md) 了解如何开始
- [治理文档](./docs/GOVERNANCE.md) 了解项目决策流程
- [行为准则](./docs/CODE_OF_CONDUCT.md) 了解社区标准

想成为维护者吗？请查看我们的 [贡献者阶梯](./docs/CONTRIBUTOR_LADDER.md)。

## 路线图

查看我们的 [项目路线图](https://github.com/WasmEdge/WasmEdge/blob/master/docs/ROADMAP.md) 以了解 WasmEdge 即将推出的功能和计划。

## 联系

如有任何疑问，请随时在相关项目上提 GitHub issue，或加入下列频道：

* 邮件清单：发送邮件至 [WasmEdge@googlegroups.com](https://groups.google.com/g/wasmedge/)
* Discord: 加入 [WasmEdge Discord 服务器](https://discord.gg/h4KDyB8XTt)!
* Slack: 加入 #WasmEdge 组群： [CNCF Slack](https://slack.cncf.io/)
* 推特 (X)：在 [X](https://x.com/realwasmedge) 关注 @realwasmedge

## 采用者

查看我们在项目中使用了 WasmEdge 的 [采用者列表](https://wasmedge.org/docs/zh/contribute/users/)。

## 社区会议

我们每月举行一次社区会议，展示新功能、演示新用例并进行问答。欢迎所有人参加！

时间：每月第一个星期二，香港时间晚上 11 点 / 太平洋标准时间早上 7 点。

[公开会议议程/笔记](https://docs.google.com/document/d/1iFlVl7R97Lze4RDykzElJGDjjWYDlkI8Rhf8g4dQ5Rk/edit#) | [Zoom 链接](https://us06web.zoom.us/j/82221747919?pwd=3MORhaxDk15rACk7mNDvyz9KtaEbWy.1)

## License

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_large)
