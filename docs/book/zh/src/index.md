# 快速开始

WasmEdge 最简单的使用方式是通过 WasmEdge CLI。
开发者们能使用这个命令行工具来运行我们的 WebAssembly 和 JavaScript 示例程序。
之后，我们也可以使用该工具来创建新的 WasmEdge 程序，并将这些程序部署到不同的应用或者框架中运行。

## 安装

你可以使用以下的单行命令来安装 WasmEdge。
你的系统必须预先安装 `git` 和 `curl`。

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash
```

如果你正在使用Windows 10，你可以使用Windows Package Manager Client (也称winget.exe)来安装WasmEdge。

```bash
winget install wasmedge
```

如果你希望一并安装 [Tensorflow 和图像处理扩展](https://www.secondstate.io/articles/wasi-tensorflow/)，请执行以下命令。它将尝试在你的系统上安装 Tensorflow 和图像共享库。

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -e all
```

执行以下命令能使已安装的二进制文件在当前会话中可用。

```bash
source $HOME/.wasmedge/env
```

## 使用 Docker 进行安装

如果你使用的是 Docker，你可以直接运行 WasmEdge 应用开发镜像（[x86](https://hub.docker.com/repository/docker/wasmedge/appdev_x86_64) 和 [arm64](https://hub.docker.com/repository/docker/wasmedge/appdev_aarch64)）。这些镜像里包含快速开发 WasmEdge 所需的所有工具。

```bash
$ docker pull wasmedge/appdev_x86_64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_x86_64:0.9.0
(docker) #
```

## WebAssembly 示例

这里有几个 WebAssembly 字节码的示例供您试用新安装的 WasmEdge CLI。

### Hello world

[hello.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/hello.wasm) 这个 WebAssembly 程序中包含一个 `main()` 函数。
[查看该程序的 Rust 源码项目。](https://github.com/second-state/wasm-learning/tree/master/cli/hello)
它将打印 `hello`，以及所有的命令行参数。

```bash
$ wasmedge hello.wasm second state
hello
second
state
```

### 调用一个 Rust 函数

[add.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/add.wasm) 这个 WebAssembly 程序包含一个 `add()` 函数。
[查看该程序的 Rust 源码项目。](https://github.com/second-state/wasm-learning/tree/master/cli/add)
我们在反应器模式下使用 WasmEdge 来调用 `add()`，并给它 2 个整数作为输入参数。

```bash
$ wasmedge --reactor add.wasm add 2 2
4
```

### 调用一个 WAT 函数

我们手动创建了 [fibonacci.wat](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wat) 程序，并使用了 [wat2wasm](https://github.com/WebAssembly/wabt) 编译器来构建 [fibonacci.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wasm) 这个 WebAssembly 程序。
它包含了一个 `fib()` 函数，这个函数以一个整数作为输入参数。我们在反应器模式下使用 WasmEdge 来调用这个导出函数。

```bash
$ wasmedge --reactor fibonacci.wasm fib 10
89
```

### 开启统计

CLI工具支持 `--enable-all-statistics` 标志，用于开启统计和 gas meter 的相关配置。

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

CLI工具支持 `--gas-limit` 标志，用于控制执行的成本。

```bash
# cd <path/to/WasmEdge>
$ cd examples/wasm
# gas 足够时
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

# gas 不足时
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

WasmEdge 也可以作为一个高性能、安全、可扩展、易于部署且[遵循 Kubernetes](https://github.com/second-state/wasmedge-containers-examples) 的 JavaScript 运行时。

[qjs.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/js/qjs.wasm) 是一个被编译为 WebAssembly 的 JavaScript 解释器。
[hello.js](https://github.com/WasmEdge/WasmEdge/raw/master/examples/js/hello.js) 是一个非常简单的 JavaScript 程序。

```bash
$ wasmedge --dir .:. qjs.wasm hello.js 1 2 3
Hello 1 2 3
```

[qjs_tf.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/js/qjs_tf.wasm) 则是一个 WebAssembly 版本的 JavaScript 解释器（带有 [Tensorflow 扩展](https://www.secondstate.io/articles/wasi-tensorflow/)）。
要想运行 [qjs_tf.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/js/qjs_tf.wasm)，你必须使用 `wasmedge-tensorflow-lite` 这个命令行工具；这个工具里内置了包含 Tensorflow 扩展的 WasmEdge 构建版本。
你可以下载一个[基于 Tensorflow 的完整 JavaScript 示例](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/tensorflow_lite_demo)来对图像进行分类。

```bash
# Download the Tensorflow example
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/aiy_food_V1_labelmap.txt
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/food.jpg
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/lite-model_aiy_vision_classifier_food_V1_1.tflite
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/main.js

$ wasmedge-tensorflow-lite --dir .:. qjs_tf.wasm main.js
label: Hot dog
confidence: 0.8941176470588236
```

继续阅读并学习 WasmEdge。

- [WasmEdge 的安装与卸载](start/install.md)
- [WasmEdge 命令行](start/cli.md)
- [WasmEdge 应用场景](intro/use.md)
- [WasmEdge 的优势与特点](intro/features.md)
