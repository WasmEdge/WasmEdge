# WasmEdge 应用场景

WasmEdge 是由 CNCF 托管的云原生 WebAssembly runtime。它广泛应用于边缘计算、汽车、Jamstack、Serverless、SaaS、服务网格，乃至区块链应用。 WasmEdge 可以进行 AOT （提前编译）编译器优化，是当今市场上最快的 WebAssembly runtime。

## 目录

* [云原生 runtime](#cloud-native-runtime-as-a-lightweight-docker-alternative)
  * [Dapr](#dapr-distributed-application-runtime)
* [JavaScript 或 DSL runtime](#javascript-or-dsl-runtime)
  * [JavaScript](#javascript)
  * [用于图像识别的 DSL](#dsl-for-image-classification)
* [公有云中的 Serverless 函数即服务](#serverless-function-as-a-service-in-public-clouds)
  * [AWS Lambda](#aws-lambda)
  * [腾讯 Serverless 函数](#tencent-serverless-functions)
  * [Vercel Serverless 函数](#vercel-serverless-functions)
  * [Netlify 函数](#netlify-functions)
  * [Second State 函数](#second-state-functions)
* [软件定义的汽车和 AIoT](#software-defined-vehicles-and-aiot)
  * [YoMo Flow](#yomo-flow)
* [用于 SaaS 的互动函数](#reactive-functions-for-saas)
  * [Slack](#slack)
  * [飞书](#lark)


## 云原生 runtime (作为 Docker 的轻量级替代) 

WasmEdge 可以通过其 [C](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md), [Go](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/), [Rust](https://github.com/WasmEdge/WasmEdge/tree/master/wasmedge-rs),和[JavaScript](https://www.secondstate.io/articles/getting-started-with-rust-function/)的 SDK 嵌入到云原生基础设施中。它也是一个符合 OCI 的 runtime，可以由 [CRI-O 和 Docker 工具直接管理](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/) ，作为 Docker 的轻量级和高性能替代。

### Dapr (分布式应用 Runtime)

* 教程 (待发布)
* [代码教程](https://github.com/second-state/dapr-wasm)

### Service mesh (开发进行中): 

* Linkerd
* MOSN
* Envoy

### 编排和管理 (开发进行中): 

* Kubernetes
* KubeEdge
* SuperEdge


## JavaScript 或 DSL runtime 

为了让 WebAssembly 被开发者广泛采用作为 runtime，它必须支持像 JavaScript 这样的“简单”语言。或者，更棒的是，通过其高级编译器工具链，WasmEdge 可以支持高性能 DSL（领域特定语言），这是专为特定任务设计的低代码解决方案。

### JavaScript

WasmEdge 可以通过嵌入 JS 执行引擎或解释器来充当云原生 JavaScript runtime。它比在 Docker 中运行 JS 引擎更快更轻。 WasmEdge 支持 JS API 访问原生扩展库，例如网络 socket、tensorflow 和用户定义的共享库。它还允许将 JS 嵌入其他高性能语言（例如Rust）或使用 Rust/C 来实现 JS 函数。

* [教程](https://www.secondstate.io/articles/run-javascript-in-webassembly-with-wasmedge/)
* [例子](https://github.com/WasmEdge/WasmEdge/blob/master/tools/wasmedge/examples/js/README.md)
* [WasmEdge的内嵌 QuickJS 引擎](https://github.com/second-state/wasmedge-quickjs)

### 用于图片识别的 DSL

图像识别 DSL 是一种 YAML 格式，允许用户指定 tensorflow 模型及其参数。 WasmEdge 将图像作为 DSL 的输入并输出检测到的项目名称/标签。

* 示例: [运行 YMAL 以识别图片中的食品](https://github.com/second-state/wasm-learning/blob/master/cli/classify_yml/config/food.yml) 

### 用于聊天机器人的 DSL

聊天机器人 DSL 函数接受输入字符串并回复字符串进行响应。 DSL 指定了聊天机器人的内部状态转换，以及用于语言理解的 AI 模型。正在开发中。


## Serverless function-as-a-service in public clouds 

WasmEdge 与现有的 Serverless 或 Jamstack 平台配合使用，为函数提供高性能、可移植和安全的 runtime。即在这些平台上的 Docker 或 microVM 中运行，也能提供显着的好处。

### AWS Lambda 

* [教程](https://www.cncf.io/blog/2021/08/25/webassembly-serverless-functions-in-aws-lambda/)
* [代码模板](https://github.com/second-state/aws-lambda-wasm-runtime)

### 腾讯 Serverless 函数 

* [中文教程](https://my.oschina.net/u/4532842/blog/5172639)
* [代码模板](https://github.com/second-state/tencent-scf-wasm-runtime)

### Vercel Serverless 函数

* [教程](https://www.secondstate.io/articles/vercel-wasmedge-webassembly-rust/)
* [代码模版](https://github.com/second-state/vercel-wasm-runtime)

### Netlify 函数

* [教程](https://www.secondstate.io/articles/netlify-wasmedge-webassembly-rust-serverless/)
* [代码模版](https://github.com/second-state/netlify-wasm-runtime)

### Second State 函数

* [教程](https://www.secondstate.io/faas/)


## 软件定义的汽车和 AIoT

WasmEdge 非常适合在任务关键的边缘设备或边缘网络上运行。

### YoMo Flow

YoMo 是一种用于远边缘（far edge）网络的高性能数据流框架。 WasmEdge 集成到 YoMo 中以运行用户定义的工作负载，例如在工厂装配线上进行的图像识别。

* [教程](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/)
* [代码模板](https://github.com/yomorun/yomo-wasmedge-tensorflow)

### seL4 微内核操作系统

seL4 是一个高度安全的实时操作系统。 WasmEdge 是唯一可以在 seL4 上运行的 WebAssembly runtime，它以本机速度运行。我们还提供了一个管理工具来支持 wasm 模块的 OTA 部署。正在开发中。


## SaaS 的响应式函数

WasmEdge 可以使用 Serverless 函数而不是传统的网络 API 来支持定制的 SaaS 扩展或应用程序。这极大地提高了 SaaS 用户和开发者的生产力。


### Slack

* [为 Slack 创建 serverless 聊天机器人 ](http://reactor.secondstate.info/en/docs/user_guideline.html)

### 飞书

飞书为字节跳动，即抖音母公司，旗下的聊天软件。

* [为飞书创建 serverless 聊天机器人](http://reactor.secondstate.info/zh/docs/user_guideline.html)


如果关于 WasmEdge 有什么好主意，马上开 [一个 GitHub issue](https://github.com/WasmEdge/WasmEdge/issues) 来一起讨论吧。
