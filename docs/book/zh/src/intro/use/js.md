# JavaScript 或 DSL runtime

为了让 WebAssembly 被开发者广泛采用作为 runtime，它必须支持像 JavaScript 这样的“简单”语言。或者，更棒的是，通过其高级编译器工具链， WasmEdge 可以支持高性能 DSL（领域特定语言），这是专为特定任务设计的低代码解决方案。

## JavaScript

WasmEdge 可以通过嵌入 JS 执行引擎或解释器来充当云原生 JavaScript runtime 。它比在 Docker 中运行 JS 引擎更快更轻。 WasmEdge 支持 JS API 访问原生扩展库，例如网络 socket 、 tensorflow 和用户定义的共享库。它还允许将 JS 嵌入其他高性能语言（例如 Rust ）或使用 Rust/C 来实现 JS 函数。

* 教程
  * [运行 JavaScript](https://www.secondstate.io/articles/run-javascript-in-webassembly-with-wasmedge/)
  * [在 Rust 中嵌入 JavaScript](https://www.secondstate.io/articles/embed-javascript-in-rust/)
  * [用 Rust 函数创建 JavaScript API](https://www.secondstate.io/articles/embed-rust-in-javascript/)
  * [从 JavaScript调用 C 原生共享库函数](https://www.secondstate.io/articles/call-native-functions-from-javascript/)
* [例子](https://github.com/WasmEdge/WasmEdge/blob/master/examples/js/README.md)
* [WasmEdge的内嵌 QuickJS 引擎](https://github.com/second-state/wasmedge-quickjs)

## 用于图片识别的 DSL

图像识别 DSL 是一种 YAML 格式，允许用户指定 tensorflow 模型及其参数。 WasmEdge 将图像作为 DSL 的输入并输出检测到的项目名称/标签。

* 示例: [运行 YMAL 以识别图片中的食品](https://github.com/second-state/wasm-learning/blob/master/cli/classify_yml/config/food.yml)

## 用于聊天机器人的 DSL

聊天机器人 DSL 函数接受输入字符串并回复字符串进行响应。 DSL 指定了聊天机器人的内部状态转换，以及用于语言理解的 AI 模型。
