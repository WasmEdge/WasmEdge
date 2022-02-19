# Getting started

The easiest way to get started with WasmEdge is to use its command line tools (CLI).
You can then run our example WebAssembly and JavaScript programs in the WasmEdge CLI.
After that, you can create new programs for WasmEdge and run them in different host applications or frameworks.

## Install

You can install WasmEdge using our one-line installer.
Your system should have `git` and `curl` as prerequisites.

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash
```

For Windows 10, you could use Windows Package Manager Client (aka winget.exe) to install WasmEdge with a single command in your terminal.

```bash
winget install wasmedge
```

If you would like to install WasmEdge with its [Tensorflow and image processing extensions](https://www.secondstate.io/articles/wasi-tensorflow/), please run the following command. It will attempt to install Tensorflow and image shared libraries on your system.

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -e all
```

Run the following command to make the installed binary available in the current session.

```bash
source $HOME/.wasmedge/env
```

## Use Docker

If you use Docker, you can simply run the WasmEdge application developer Docker images ([x86](https://hub.docker.com/repository/docker/wasmedge/appdev_x86_64) and [arm64](https://hub.docker.com/repository/docker/wasmedge/appdev_aarch64)). Those images contain all the tooling you need for quick WasmEdge development.

```bash
$ docker pull wasmedge/appdev_x86_64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_x86_64:0.9.0
(docker) #
```

## WebAssembly examples

We have several WebAssembly bytecode program examples for you to try out on your newly installed WasmEdge CLI.

### Hello world

The [hello.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/hello.wasm) WebAssembly program contains a `main()` function.
Checkout its [Rust source code project](https://github.com/second-state/wasm-learning/tree/master/cli/hello).
It prints out `hello` followed by the command line arguments.

```bash
$ wasmedge hello.wasm second state
hello
second
state
```

### Call a function written in Rust

The [add.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/add.wasm) WebAssembly program contains an `add()` function.
Checkout its [Rust source code project](https://github.com/second-state/wasm-learning/tree/master/cli/add).
We use WasmEdge in reactor mode to call the `add()` with two integer input parameters.

```bash
$ wasmedge --reactor add.wasm add 2 2
4
```

### Call a function written in WAT

We created the [fibonacci.wat](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/fibonacci.wat) program by hand and used the [wat2wasm](https://github.com/WebAssembly/wabt) compiler to build the [fibonacci.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/fibonacci.wasm) WebAssembly program.
It contains a `fib()` function which takes a single integer as input parameter. We use wasmedge in reactor mode to call the exported function.

```bash
$ wasmedge --reactor fibonacci.wasm fib 10
89
```

### With Statistics enabled

The CLI supports `--enable-all-statistics` flags for the statistics and gas meter.

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

### With gas-limit enabled

The CLI supports `--gas-limit` flags for controlling the execution costs.

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

## JavaScript examples

It is possible to use WasmEdge as a high-performance, secure, extensible, easy to deploy, and [Kubernetes-compliant](https://github.com/second-state/wasmedge-containers-examples) JavaScript runtime.

The [qjs.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/js/qjs.wasm) program is a JavaScript interpreter compiled into WebAssembly.
The [hello.js](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/js/hello.js) file is a very simple JavaScript program.

```bash
$ wasmedge --dir .:. qjs.wasm hello.js 1 2 3
Hello 1 2 3
```

The [qjs_tf.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/js/qjs_tf.wasm) is a JavaScript interpreter with [WasmEdge Tensorflow extension](https://www.secondstate.io/articles/wasi-tensorflow/) compiled into WebAssembly.
To run [qjs_tf.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/js/qjs_tf.wasm), you must use the `wasmedge-tensorflow-lite` CLI tool, which is a build of WasmEdge with Tensorflow extension built-in.
You can download a full [Tensorflow-based JavaScript example](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/tensorflow_lite_demo) to classify images.

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

Read on and continue your learning of WasmEdge.

* [Install and uninstall WasmEdge](start/install.md)
* [Use the WasmEdge CLI](start/cli.md)
* [WasmEdge use cases](intro/use.md)
* [WasmEdge technical highlights](intro/features.md)
