<div align="right">

  [中文](README-zh.md) | [正體中文](README-zh-TW.md) | [日本語で読む](README-ja.md)

</div>

<div align="center">
  
![WasmEdge Logo](/docs/wasmedge-runtime-logo.png)

# [🤩 WasmEdge 是在您自己的设备上运行 LLM 的最简单、最快的方法。🤩](https://llamaedge.com/docs/user-guide/llm/get-started-with-llamaedge)

<a href="https://trendshift.io/repositories/2481" target="_blank"><img src="https://trendshift.io/api/badge/repositories/2481" alt="WasmEdge%2FWasmEdge | Trendshift" style="width: 250px; height: 55px;" width="250" height="55"/></a>

WasmEdge 是一个轻量级、高性能、可扩展的 WebAssembly 运行时。它是[速度最快的 Wasm 虚拟机](https://ieeexplore.ieee.org/document/9214403)。WasmEdge 是由 [CNCF](https://www.cncf.io/) 托管的官方沙箱项目。[LlamaEdge](https://github.com/LlamaEdge/LlamaEdge) 是一个构建在 WasmEdge 之上的应用框架，用于在服务器、个人电脑和边缘设备的 GPU 上运行 GenAI 模型（例如 [LLM](https://llamaedge.com/docs/user-guide/llm/get-started-with-llamaedge)、[语音转文本](https://llamaedge.com/docs/user-guide/speech-to-text/quick-start-whisper)、[文本转图像](https://llamaedge.com/docs/user-guide/text-to-image/quick-start-sd) 和 [TTS](https://github.com/LlamaEdge/whisper-api-server)）。其他[用例](https://wasmedge.org/docs/start/usage/use-cases/)包括边缘云上的微服务、无服务器 SaaS API、嵌入式函数、智能合约和智能设备。

[![build](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml?query=event%3Apush++branch%3Amaster)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![CodeQL](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml?query=event%3Apush++branch%3Amaster)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/5059/badge)](https://bestpractices.coreinfrastructure.org/projects/5059)

</div>

# 快速入门指南

🚀 [安装](https://wasmedge.org/docs/start/install) WasmEdge \
👷🏻‍♂️ [构建](https://wasmedge.org/docs/category/build-wasmedge-from-source)并[为 WasmEdge 做出贡献](https://wasmedge.org/docs/contribute/) \
⌨️ 从 CLI 或 [Docker](https://wasmedge.org/docs/start/getting-started/quick_start_docker) [运行](https://wasmedge.org/docs/category/running-with-wasmedge)一个独立的 Wasm 程序或一个 [JavaScript 程序](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript) \
🤖 通过 [LlamaEdge](https://github.com/LlamaEdge/LlamaEdge) 与一个开源 LLM [聊天](https://llamaedge.com/docs/user-guide/llm/get-started-with-llamaedge) \
🔌 在您的 [Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge)、[Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge) 或 [C](https://wasmedge.org/docs/category/c-sdk-for-embedding-wasmedge) 应用中嵌入一个 Wasm 函数 \
🛠 使用 [Kubernetes](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes)、[数据流框架](https://wasmedge.org/docs/embed/use-case/yomo)和[区块链](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a)来管理和编排 Wasm 运行时 \
📚 **[查看我们的官方文档](https://wasmedge.org/docs/)**

# 简介

WasmEdge 运行时为其包含的 WebAssembly 字节码程序提供了一个定义明确的执行沙箱。该运行时为操作系统资源（例如，文件系统、套接字、环境变量、进程）和内存空间提供隔离和保护。WasmEdge 最重要的用例是在软件产品（例如，SaaS、软件定义汽车、边缘节点，甚至区块链节点）中作为插件安全地执行用户定义或社区贡献的代码。它使第三方开发人员、供应商、供应商和社区成员能够扩展和定制软件产品。**[在此处了解更多信息](https://wasmedge.org/docs/contribute/users)**

## 性能

* [一种用于高性能无服务器计算的轻量级设计](https://arxiv.org/abs/2010.07115)，发表于 IEEE Software，2021 年 1 月。[https://arxiv.org/abs/2010.07115](https://arxiv.org/abs/2010.07115)
* [云中 Arm 与 x86 CPU 的性能分析](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)，发表于 infoQ.com，2021 年 1 月。[https://www.infoq.com/articles/arm-vs-x86-cloud-performance/](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)
* [WasmEdge 是 Suborbital Reactr 测试套件中最快的 WebAssembly 运行时](https://blog.suborbital.dev/suborbital-wasmedge)，2021 年 12 月

## 特性

WasmEdge 可以运行从 C/C++、Rust、Swift、AssemblyScript 或 Kotlin 源代码编译的标准 WebAssembly 字节码程序。它在一个安全、快速、轻量级、可移植和容器化的沙箱中[运行 JavaScript](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript)，包括第三方 ES6、CJS 和 NPM 模块。它还支持混合这些语言（例如，[使用 Rust 实现 JavaScript API](https://wasmedge.org/docs/develop/javascript/rust)）、[Fetch API](https://wasmedge.org/docs/develop/javascript/networking#fetch-client) 和边缘服务器上的[服务器端渲染 (SSR)](https://wasmedge.org/docs/develop/javascript/ssr) 功能。

WasmEdge 支持[所有标准的 WebAssembly 功能和许多提议的扩展](https://wasmedge.org/docs/start/wasmedge/extensions/proposals)。它还支持许多为云原生和边缘计算用途量身定制的扩展（例如，[WasmEdge 网络套接字](https://wasmedge.org/docs/category/socket-networking)、[基于 Postgres 和 MySQL 的数据库驱动程序](https://wasmedge.org/docs/category/database-drivers)和 [WasmEdge AI 扩展](https://wasmedge.org/docs/category/ai-inference)）。

**了解有关 [WasmEdge 的技术亮点](https://wasmedge.org/docs/start/wasmedge/features)的更多信息。**

## 集成与管理

WasmEdge 及其包含的 wasm 程序可以从 [CLI](https://wasmedge.org/docs/category/running-with-wasmedge) 作为一个新进程启动，也可以从现有进程启动。如果从现有进程（例如，从正在运行的 [Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge) 或 [Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge) 程序）启动，WasmEdge 将简单地作为函数在进程内运行。目前，WasmEdge 还不是线程安全的。为了在您自己的应用程序或云原生框架中使用 WasmEdge，请参阅以下指南。

* [将 WasmEdge 嵌入到宿主应用程序中](https://wasmedge.org/docs/embed/overview)
* [使用容器工具编排和管理 WasmEdge 实例](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes)
* [将 WasmEdge 应用程序作为 Dapr 微服务运行](https://wasmedge.org/docs/develop/rust/dapr)

# 社区

## 贡献

我们欢迎社区的贡献！请查看我们的：
- [贡献指南](./docs/CONTRIBUTING.md) 以了解如何开始
- [治理文档](./docs/GOVERNANCE.md) 以了解项目决策过程
- [行为准则](./docs/CODE_OF_CONDUCT.md) 以了解社区标准

想成为维护者吗？请参阅我们的[贡献者阶梯](./CONTRIBUTION_LADDER.md)。

## 路线图

查看我们的[项目路线图](https://github.com/WasmEdge/WasmEdge/blob/master/docs/ROADMAP.md)以了解 WasmEdge 即将推出的功能和计划。

## 联系

如果您有任何问题，请随时在相关项目上提出 GitHub 问题或加入以下渠道：

* 邮件列表：发送电子邮件至 [WasmEdge@googlegroups.com](https://groups.google.com/g/wasmedge/)
* Discord：加入 [WasmEdge Discord 服务器](https://discord.gg/h4KDyB8XTt)！
* Slack：在 [CNCF Slack](https://slack.cncf.io/) 上加入 #WasmEdge 频道
* X (前 Twitter)：在 [X](https://x.com/realwasmedge) 上关注 @realwasmedge

## 采用者

查看我们的[采用者列表](https://wasmedge.org/docs/contribute/users/)，他们在自己的项目中使用 WasmEdge。

## 社区会议

我们每月举办一次社区会议，展示新功能、演示新用例，并设有问答环节。欢迎大家参加！

时间：每月第一个星期二，香港时间晚上 11 点/太平洋标准时间早上 7 点。

[公开会议议程/记录](https://docs.google.com/document/d/1iFlVl7R97Lze4RDykzElJGDjjWYDlkI8Rhf8g4dQ5Rk/edit#) | [Zoom 链接](https://us06web.zoom.us/j/82221747919?pwd=3MORhaxDk15rACk7mNDvyz9KtaEbWy.1)

# 许可证

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_large)
