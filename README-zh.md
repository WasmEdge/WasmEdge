
# 快速开始指引

🚀 [安装](docs/install.md) WasmEdge\
🤖 [从源码编译](docs/build.md)  [贡献](docs/contribution.md) WasmEdge\
⌨️  [从 CLI](docs/run.md)或 [Node.js](https://github.com/second-state/wasm-learning/tree/master/ssvm/file-example)  [Golang](https://github.com/second-state/WasmEdge-go/tree/master/examples/go_mtcnn)跑一个独立的 Wasm 程序\
💭 [部署一个 Wasm 函数](https://www.secondstate.io/articles/getting-started-with-function-as-a-service-in-rust/)即 web 服务 (FaaS)\
🛠 [嵌入一个用户定义的 Wasm 函数](http://reactor.secondstate.info/docs/user-create-a-bot.html)在  SaaS 平台上\
🔩 [嵌入一个 Wasm 函数](https://www.secondstate.io/articles/getting-started-with-rust-function/)在你的 Node.js web app 里\
🔌 [嵌入一个 Wasm 函数](https://github.com/second-state/WasmEdge-go/tree/master/examples/go_PassBytes)在你的 Golang app 里\
🔗 [嵌入一个 Wasm 函数](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a)即区块链智能合约 

![build](https://github.com/WasmEdge/WasmEdge/workflows/build/badge.svg)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/WasmEdge/WasmEdge.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/WasmEdge/WasmEdge/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/WasmEdge/WasmEdge.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/WasmEdge/WasmEdge/context:cpp)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)

# 介绍

WasmEdge (之前名为 SSVM) 是为边缘计算优化的高性能 WebAssembly (Wasm) 虚拟机，包括了边缘云和软件定义的汽车。在AOT模式下, WasmEdge 是目前市场上 [最快的 Wasm 虚拟机](https://ieeexplore.ieee.org/document/9214403)。

WasmEdge 是一个 [CNCF](https://www.cncf.io/) (Cloud Native Computing Foundation云原生计算基金会)托管的官方沙盒项目。

WasmEdge 的最重要应用是在一个软件产品（例如，SaaS、汽车操作系统、边缘节点，甚至区块链节点）中安全地执行用户定义或者社区贡献的代码。它使第三方开发者、软件提供商、供应商和社区成员能够扩展和定制软件产品。 使用了 WasmEdge，软件产品可以成为托管平台。

WasmEdge 为其包含的 Wasm 字节码程序提供了一个定义良好的执行沙箱。通过配置选项，可以控制虚拟机启动时能够访问的系统资源(即基于能力的安全模型），意即没有虚拟机的明确许可，字节码程序无法访问操作系统资源（例如，文件系统、套接字、环境变量、进程）。

WasmEdge 还为其运行的 wasm 程序提供内存保护。 如果程序尝试访问分配给虚拟机的区域之外的内存，则虚拟机将终止并显示一条错误消息。

WasmEdge 及其运行的 wasm 程序可以作为新进程或从现有进程从 CLI 启动。 如果从现有进程启动（例如，从正在运行的 [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) 或 [Golang](https://github.com/second-state/wasmedge-go) 程序），虚拟机将简单地作为一个函数在进程内运行。 也可以将 WasmEdge 虚拟机实例作为线程启动。 目前，WasmEdge 还不是线程安全的，这意味着在同一进程中的不同线程中运行的虚拟机实例可能能够访问彼此的内存。 未来，我们计划让 WasmEdge 做到线程安全。


# 将 WasmEdge 嵌入 host 应用

WasmEdge 的一个主要应用场景是从一个 host 应用程序启动一个虚拟机实例。一般而言，你可以使用 [WasmEdge C API](https://github.com/WasmEdge/WasmEdge/blob/master/include/api/wasmedge.h.in) 做到这一点。

可是， Wasm spec 和 [WasmEdge C API](https://github.com/WasmEdge/WasmEdge/blob/master/include/api/wasmedge.h.in), 仅支持非常有限的数据类型作为包含的 Wasm 字节码函数的输入参数和返回值。 如果要将复杂的数据类型（例如数组的字符串）作为调用参数传递给所包含的函数，应该使用 [rustwasmc](https://github.com/second-state/rustwasmc) 工具链提供的 bindgen 解决方案。

我们目前[支持 Node.js 环境中的bindgen](https://www.secondstate.io/articles/getting-started-with-rust-function/). 我们正在致力于在 Golang 和基于 Rust 的 host 应用程序中支持 bindgen。

# 从 WasmEdge调用原生 host 程序

有时， Wasm 字节码本身被证明对有些应用来说限制太多。 WasmEdge 提供一个 [host 函数 API](https://github.com/WasmEdge/WasmEdge/blob/master/docs/host_function.md)这允许 Wasm 字节码程序从底层 host 操作系统加载和调用原生库函数。

>此功能破坏了Wasm沙箱。 但是沙箱破坏是在系统操作员的明确许可下完成的。

事实上，对 WasmEdge 的扩展是使用原生 host 函数实现的。 例如，[Tensorflow 扩展](https://www.secondstate.io/articles/wasi-tensorflow/) 允许 Wasm 字节码调用原生 Tensorflow 库函数。

# 管理 WasmEdge 虚拟机实例

有了 [WasmEdge C API](docs/c_api.md), 您可以编写程序来启动、停止和管理您自己的应用程序中的 WasmEdge 虚拟机实例。您也可以参阅[WasmEdge C API 快速开始指引](docs/c_api_quick_start.md)。例如 

* 当 WasmEdge 函数嵌入在了 [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) 或者在 [飞书](http://reactor.secondstate.info/docs/user-create-a-bot.html), 当有传入请求时，虚拟机由应用程序启动。
* 当 WasmEdge 函数被插入到像 [YoMo](https://github.com/yomorun/yomo-flow-ssvm-example) 这样的数据流引擎中时，当一个新的数据点流经系统时，虚拟机就会启动。

如果您有兴趣使用 Kubernetes 来管理 WasmEdge 虚拟机，您可以安装我们的自定义 [runw](https://github.com/second-state/runw) 实用程序。 他们可以像加载 Docker 镜像一样加载 Wasm 字节码程序文件，然后根据配置的策略启动、运行和停止虚拟机实例。

# 支持 wasm 标准接口

WasmEdge 支持可选的 WebAssembly 特性和提案。 这些提议很可能在未来成为官方的 WebAssembly 规范。 WasmEdge 支持以下提案。

* [WASI (WebAssembly 系统接口) spec](https://github.com/WebAssembly/WASI). WasmEdge 支持 WebAssembly 程序的 WASI 规范，以安全地与宿主机 Linux 操作系统交互。
* [Reference Types](https://webassembly.github.io/reference-types/core/). 它允许 WebAssembly 程序来与 host应用交换数据和操作系统。 
* [Bulk memory operations](https://github.com/WebAssembly/bulk-memory-operations/blob/master/proposals/bulk-memory-operations/Overview.md). WebAssembly 程序内存访问更快并在大容量内存操作中表现更好。
* [SIMD (Single instruction, multiple data)](https://github.com/second-state/SSVM/blob/master/docs/simd.md)。 对于具有多个 CPU 内核的现代设备，SIMD 允许数据处理程序充分利用 CPU。 SIMD 可以显着提高数据应用程序的性能。

同时， WasmEdge 团队正[探索wasi-socket提案](https://github.com/second-state/w13e_wasi_socket) 支持 WebAssembly 程序中的网络访问。 

# WasmEdge 扩展

WasmEdge 与其它的 WebAssembly 虚拟机的关键区别是它对非标准扩展的支持。WASI 规范为开发者提供了一种有效且安全地扩展 WebAssembly 虚拟机的机制。 WasmEdge 团队根据现实世界的客户需求创建了以下类似 WASI 的扩展。

* [Tensorflow](https://github.com/second-state/wasmedge-tensorflow). 开发者可以使用 [一个简单的 Rust API](https://crates.io/crates/ssvm_tensorflow_interface) 编写 Tensorflow 推理函数，然后在 WasmEdge 内以本机速度安全地运行该函数。
* 其他AI框架。除了 Tensorflow，Second State 团队还在为 AI 框架（如 用于 WasmEdge 的ONNX 和 Tengine）构建类 WASI 的扩展。
* [存储](https://github.com/second-state/wasmedge-storage)。 WasmEdge [存储接口](https://github.com/second-state/rust_native_storage_library) 允许 WebAssembly 程序读取和写入键值存储。
* [命令界面](https://github.com/second-state/wasmedge_process_interface)。WasmEdge 让 Webassembly 功能可以执行宿主机操作系统的本地命令。它支持传递参数、环境变量、STDIN/STDOUT pipes 和宿主机访问的安全策略。
* [以太坊](https://github.com/second-state/wasmedge-evmc)。 WasmEdge Ewasm 扩展支持编译为 WebAssembly 的以太坊智能合约。它是以太坊风格的 WebAssembly (Ewasm) 的领先实现。
* [Substrate](https://github.com/second-state/substrate-ssvm-node)。 [Pallet](https://github.com/second-state/pallet-ssvm) 让 WasmEdge 能在任何基于 Substrate 的区块链上充当以太坊智能合约执行引擎。


# 应用场景

* *Jamstack 应用* 由带有 JavaScript 的静态前端组成，用于与后端 API 进行交互。这是现在流行的[现代web应用程序架构](https://jamstack.org/)。前端静态文件可以通过 CDN 分发，后端函数可以托管在边缘节点上。 [基于云的 WasmEdge](https://www.secondstate.io/faas/) 为Jamstack app 托管安全且高性能的后端 Serverless 函数，特别是在边缘云上。 
  * 案例：[给web app上的任意图片增加水印](https://second-state.github.io/wasm-learning/faas/watermark/html/index.html).
  * 案例：[基于腾讯云的serverless Tensorflow函数](https://github.com/second-state/tencent-tensorflow-scf).
* *SaaS 应用程序* 通常需要根据客户要求“在边缘”进行定制或定制。 使用 WasmEdge，SaaS 应用程序可以直接嵌入和执行用户提交的代码作为工作流的一部分（例如作为处理来自 SaaS 应用程序的事件的回调函数）。
  * 案例：[飞书应用平台](http://reactor.secondstate.info/docs/user-create-a-bot.html)可以通过 WasmEdge 嵌入用户提交的 serverless 函数来回复消息（例如[聊天机器人](https://app.feishu.cn/app/cli_a08fe99f8169900d)）。
  * 案例：[WasmEdge运行自定义代码来处理IoT流数据框架YoMo中的事件。](https://github.com/yomorun/yomo-flow-ssvm-example)
* WasmEdge 被调整为适用*边缘设备*的各种嵌入式和实时操作系统。 这让开发者只需用 Rust 或 C 编写一次高性能应用程序，就能在许多边缘设备平台上安全地运行。 
  * 案例： [RIOS Lab示例：RIOS 实验室的 RISC-V 堆栈](https://rioslab.org/)。
  * 进行中：将 WasmEdge 移植到 SeL4 实时操作系统。
  * 计划中： WasmEdge 可用作自动驾驶汽车中软件模块的 RTOS 代码运行环境。
* *区块链智能合约* 是用户提交代码，由网络中的所有节点执行。 WasmEdge 得到头部的区块链项目采用，作为智能合约执行引擎。
  * 案例: [Substrate 和 Polkadot 上的 EWASM 智能合约](https://github.com/ParaState/substrate-ssvm-node)
  
  
## 社区

### 贡献

如果您想为 WasmEdge 项目做出贡献，请参阅我们的 [CONTRIBUTING](docs/CONTRIBUTING.md) 文档了解详情。 想要获得灵感，可查看[需求清单](docs/wish_list.md)!

### 联系

如有任何疑问，请随时在相关项目上提GitHub issue，或：

* 发送邮件至 [WasmEdge@googlegroups.com](https://groups.google.com/g/wasmedge/)
* Slack: 加入 #WasmEdge 组群： [CNCF Slack](https://slack.cncf.io/)

## License
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_large)
