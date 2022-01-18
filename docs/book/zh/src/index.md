# 快速开始

使用 WasmEdge 最简单的方式就是使用它的命令行工具(CLI).
开发者们能够用这个 CLI 工具来运行我们的 WebAssembly 和 JavaScript 示例程序.
除此之外, 我们也能够用它创建新的 WasmEdge 程序, 并将它们部署到不同的应用或者框架中去运行.

## 安装

你可以使用我们的一键安装脚本来安装 WasmEdge.
你的系统必须前置安装 `git` 和 `wget`.

```
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash
```

如果你想要安装带有 [Tensorflow and image 运行插件](https://www.secondstate.io/articles/wasi-tensorflow/) 的 WasmEdge,
请执行以下命令. 下列命令将试着在你的系统安装 Tensorflow and image 相关的依赖.

```
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -e all
```

运行这些指令会让那些安装好的可执行文件，在当前会话路径 `$HOME/.wasmedge/env` 被访问到.

## 使用 Docker

如果你使用 Docker, 你可以轻而易举的运行 WasmEdge 应用开发镜像([x86](https://hub.docker.com/repository/docker/wasmedge/appdev_x86_64) 和 [arm64](https://hub.docker.com/repository/docker/wasmedge/appdev_aarch64)). 那些镜像里包含了所有你开发 WasmEdge 程序所需要的工具.

```
docker pull wasmedge/appdev_x86_64:0.9.0
docker run --rm -v $(pwd):/app -it wasmedge/appdev_x86_64:0.9.0
(docker) #
```

## WebAssembly 示例

我们有几个 WebAssembly 字节码示例, 在你刚刚安装的 WasmEdge CLI 中, 赶紧尝试一下!

### Hello world

[hello.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/hello.wasm) 这个 WebAssembly 程序中包含一个 `main()` 函数.
这里是它的 [Rust 源代码](https://github.com/second-state/wasm-learning/tree/master/cli/hello).
它通过命令行传入参数, 并最先打印出 `hello`.

```
$ wasmedge hello.wasm second state
hello
second
state
```

### 调用一个 Rust 函数

[add.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/add.wasm) 这个 WebAssembly 程序 包含一个 `add()` 函数.
这里是它的 [Rust 源代码](https://github.com/second-state/wasm-learning/tree/master/cli/add).
我们使用 WasmEdge 的 reactor 模式来调用 `add()`, 并给它 2 个整型数字作为入参.

```
$ wasmedge --reactor add.wasm add 2 2
4
```

### 调用一个 WAT 函数

我们手工创建了 [fibonacci.wat](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/fibonacci.wat) 程序, 并使用了 [wat2wasm](https://github.com/WebAssembly/wabt) 编译器来构建 [fibonacci.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/fibonacci.wasm) 这个 WebAssembly.
它包含了一个 `fib()` 函数, 这个函数以一个整型数字作为入参. 我们使用 WasmEdge 的 reactor 模式来调用这个被导出的函数.


```
$ wasmedge --reactor fibonacci.wasm fib 10
89
```

### 开启统计

CLI工具 支持 `--enable-all-statistics` 标志位, 来启用统计和 gas meter 相关配置.

```bash
$ wasmedge --enable-all-statistics hello.wasm second state
hello
second
state
[2021-12-09 16:03:33.261] [info] ====================  Statistics  ====================
[2021-12-09 16:03:33.261] [info]  Total execution time: 268266 ns
[2021-12-09 16:03:33.261] [info]  Wasm instructions execution time: 251610 ns
[2021-12-09 16:03:33.261] [info]  Host functions execution time: 16656 ns
[2021-12-09 16:03:33.261] [info]  Executed wasm instructions count: 20425
[2021-12-09 16:03:33.261] [info]  Gas costs: 20425
[2021-12-09 16:03:33.261] [info]  Instructions per second: 81177218
[2021-12-09 16:03:33.261] [info] =======================   End   ======================
```

### 开启 gas-limit

CLI工具 支持 `--gas-limit` 标志位, 来控制执行的费用.

```bash
# cd <path/to/WasmEdge>
$ cd tools/wasmedge/examples
# With enough gas
$ wasmedge --enable-all-statistics --gas-limit 20425 hello.wasm second state
hello
second
state
[2021-12-09 16:03:33.261] [info] ====================  Statistics  ====================
[2021-12-09 16:03:33.261] [info]  Total execution time: 268266 ns
[2021-12-09 16:03:33.261] [info]  Wasm instructions execution time: 251610 ns
[2021-12-09 16:03:33.261] [info]  Host functions execution time: 16656 ns
[2021-12-09 16:03:33.261] [info]  Executed wasm instructions count: 20425
[2021-12-09 16:03:33.261] [info]  Gas costs: 20425
[2021-12-09 16:03:33.261] [info]  Instructions per second: 81177218
[2021-12-09 16:03:33.261] [info] =======================   End   ======================

# Without enough gas
$ wasmedge --enable-all-statistics --gas-limit 20 hello.wasm second state
[2021-12-23 15:19:06.690] [error] Cost exceeded limit. Force terminate the execution.
[2021-12-23 15:19:06.690] [error]     In instruction: ref.func (0xd2) , Bytecode offset: 0x00000000
[2021-12-23 15:19:06.690] [error]     At AST node: expression
[2021-12-23 15:19:06.690] [error]     At AST node: element segment
[2021-12-23 15:19:06.690] [error]     At AST node: element section
[2021-12-23 15:19:06.690] [error]     At AST node: module
[2021-12-23 15:19:06.690] [info] ====================  Statistics  ====================
[2021-12-23 15:19:06.690] [info]  Total execution time: 0 ns
[2021-12-23 15:19:06.690] [info]  Wasm instructions execution time: 0 ns
[2021-12-23 15:19:06.690] [info]  Host functions execution time: 0 ns
[2021-12-23 15:19:06.690] [info]  Executed wasm instructions count: 21
[2021-12-23 15:19:06.690] [info]  Gas costs: 20
```

## JavaScript 示例

WasmEdge 也可以作为一个高性能,安全,可扩展,易于部署,以及 [Kubernetes-compliant](https://github.com/second-state/wasmedge-containers-examples) 的 JavaScript运行时来使用.

[qjs.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/js/qjs.wasm) 是一个被编译成 WebAssembly 的 JavaScript解释器.
[hello.js](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/js/hello.js) 是一个非常简单的 JavaScript 程序.

```
$ wasmedge --dir .:. qjs.wasm hello.js 1 2 3
Hello 1 2 3
```

[qjs_tf.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/js/qjs_tf.wasm) 则是一个带有[WasmEdge Tensorflow 插件](https://www.secondstate.io/articles/wasi-tensorflow/) 的 WebAssembly 版本的 JavaScript解释器.
为了可以运行 [qjs_tf.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/js/qjs_tf.wasm), 你必须使用 `wasmedge-tensorflow-lite` 这个CLI工具, 这个工具里内置了 WasmEdge with Tensorflow extension 的构建版本.
你可以下载一个完整的 [Tensorflow-based JavaScript 示例](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/tensorflow_lite_demo) 来对图像进行分类.

```
# Download the Tensorflow example
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/aiy_food_V1_labelmap.txt
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/food.jpg
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/lite-model_aiy_vision_classifier_food_V1_1.tflite
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/main.js

$ wasmedge-tensorflow-lite --dir .:. qjs_tf.wasm main.js
label: Hot dog
confidence: 0.8941176470588236
```

继续阅读学习WasmEdge.

- [WasmEdge 的安装与卸载](start/install.md)
- [WasmEdge 命令行](start/cli.md)
- [WasmEdge 应用场景](intro/use.md)
- [WasmEdge 的优势与特点](intro/features.md)
