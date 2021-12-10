# Getting started

The easiest way to get started with WasmEdge is to use its command line tools (CLI). 
You can then run our example WebAssembly and JavaScript programs in the WasmEdge CLI.
After that, you can create new programs for WasmEdge and run them in different
host applications or frameworks.

## Install

You can install WasmEdge using our one-line installer. 
Your system should have `git` and `wget` as prerequisites.

```
wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash
```

If you would like to install WasmEdge with its [Tensorflow and image processing extensions](https://www.secondstate.io/articles/wasi-tensorflow/), 
please run the following command. It will attempt to install 
Tensorflow and image shared libraries on your system.

```
wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -e all
```

Run the following command to make the installed binary available 
in the current session source `$HOME/.wasmedge/env`.

## Use Docker

If you use Docker, you can simply run the WasmEdge application developer 
Docker images ([x86](https://hub.docker.com/repository/docker/wasmedge/appdev_x86_64) and [arm64](https://hub.docker.com/repository/docker/wasmedge/appdev_aarch64)). Those images contain all the tooling you need for quick
WasmEdge development.

```
docker pull wasmedge/appdev_x86_64:0.8.2
docker run --rm -v $(pwd):/app -it wasmedge/appdev_x86_64:0.8.2
(docker) #
```

## WebAssembly examples

We have several WebAssembly bytecode program examples for you to try out on
your newly installed WasmEdge CLI! Those files are available in the following
directory.

```
https://github.com/WasmEdge/WasmEdge/tree/master/tools/wasmedge/examples
```

### Hello world

The [hello.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/hello.wasm) WebAssembly program contains a `main()` function. 
Checkout its [Rust source code project](https://github.com/second-state/wasm-learning/tree/master/cli/hello). 
It prints out `hello` followed by the command line arguments.

```
$ wasmedge hello.wasm second state
hello
second
state
```

### Call a function written in Rust

The [add.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/add.wasm) WebAssembly program contains an `add()` function. 
Checkout its [Rust source code project](https://github.com/second-state/wasm-learning/tree/master/cli/add). 
We use WasmEdge in reactor mode to call the `add()` with two integer input parameters.

```
$ wasmedge --reactor add.wasm add 2 2
4
```

### Call a function written in WAT

We created the [fibonacci.wat](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/fibonacci.wat) program by hand and used 
the [wat2wasm](https://github.com/WebAssembly/wabt) compiler to build the [fibonacci.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/fibonacci.wasm) WebAssembly program. 
It contains a `fib()` function which takes a single integer as input parameter. We use wasmedge in reactor mode to call the exported function.

```
$ wasmedge --reactor fibonacci.wasm fib 10
89
```
### With Statistics enabled

The CLI now supports `--enable-all-statistics` flags for the statistics and gas meter. 

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


## JavaScript examples

It is possible to use WasmEdge as a high-performance, secure, extensible, easy to deploy, and [Kubernetes-compliant](https://github.com/second-state/wasmedge-containers-examples) JavaScript runtime. 
The examples are in the following folder. 

```
https://github.com/WasmEdge/WasmEdge/tree/master/tools/wasmedge/examples/js
```

The [qjs.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/js/qjs.wasm) program is a JavaScript interpreter compiled into WebAssembly. 
The [hello.js](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/js/hello.js) file is a very simple JavaScript program.

```
$ wasmedge --dir .:. qjs.wasm hello.js 1 2 3
Hello 1 2 3
```

The [qjs_tf.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/js/qjs_tf.wasm) is a JavaScript interpreter with 
[WasmEdge Tensorflow extension](https://www.secondstate.io/articles/wasi-tensorflow/) compiled into WebAssembly. 
To run [qjs_tf.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/tools/wasmedge/examples/js/qjs_tf.wasm), you must use the `wasmedge-tensorflow-lite` CLI tool, which is a build of WasmEdge with Tensorflow extension built-in. 
You can download a full [Tensorflow-based JavaScript example](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/tensorflow_lite_demo) to classify images.

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



Read on and continue your learning of WasmEdge.

* [Install and uninstall WasmEdge](start/install.md)
* [Use the WasmEdge CLI](start/cli.md)
* [WasmEdge use cases](intro/use.md)
* [WasmEdge technical highlights](intro/features.md)


