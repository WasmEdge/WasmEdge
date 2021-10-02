# Run WasmEdge from CLI

After [installing WasmEdge](install.md) or starting the [WasmEdge app dev Docker container](../utils/docker/build-appdev.md), there are several ways to run compiled WebAssembly programs. In this article, we will cover the most straighforward -- to run a WebAssembly program from the Linux command line (CLI).

* If the WebAssembly program contains a `main()` function, `wasmedge` would execute it as a standalone program in the command mode.
* If the WebAssembly program contains one or more public functions, `wasmedge` could execute individual functions in the reactor mode.

## Command line options

The options and flags for the `wasmedge` command are as follows.

1. (Optional) Reactor mode: use `--reactor` to enable reactor mode. In the reactor mode, `wasmedge` runs a specified function from the WebAssembly program.
	* WasmEdge will execute the function which name should be given in ARG[0].
	* If there's exported function which names `_initialize`, the function will be executed with the empty parameter at first.
2. (Optional) Binding directories into WASI virtual filesystem.
	* Each directory can be specified as `--dir guest_path:host_path`.
3. (Optional) Environ variables.
	* Each variable can be specified as `--env NAME=VALUE`.
4. Wasm file(`/path/to/wasm/file`).
5. (Optional) Arguments.
	* In reactor mode, the first argument will be the function name, and the arguments after ARG[0] will be parameters of wasm function ARG[0].
	* In command mode, the arguments will be parameters of function `_start`. They are also known as command line arguments for a standalone program.

## Examples

Here are some examples on how use the `wasmedge` command to run WebAssembly programs. They are all located in the `tools/wasmedge/examples` folder.

### Example: Hello world

The `hello.wasm` WebAssembly program contains a `main()` function. Checkout its Rust [source code project](https://github.com/second-state/wasm-learning/tree/master/cli/hello). It prints out `hello` followed by the command line arguments. We use `wasmedge` in command mode to run the standalone program.

```bash
# cd <path/to/WasmEdge>
$ cd tools/wasmedge/examples
$ wasmedge hello.wasm second state
hello
second
state
```

### Example: Add

The `add.wasm` WebAssembly program contains a `add()` function. Checkout its Rust [source code project](https://github.com/second-state/wasm-learning/tree/master/cli/add). We use `wasmedge` in reactor mode to call the `add()` with two integer input parameters.

```bash
# cd <path/to/WasmEdge>
$ cd tools/wasmedge/examples
$ wasmedge --reactor add.wasm add 2 2
4
```

> WebAssembly only supports a few simple data types. To call a wasm function with complex input parameters and return values in reactor mode, you will need the [rustwasmc](https://github.com/second-state/rustwasmc) compiler toolchain to generate wasm functions that can be embedded into [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) or [Go](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/) programs.

### Example: Fibonacci

We created the `fibonacci.wat` program by hand and used the [wat2wasm](https://github.com/WebAssembly/wabt) compiler to build the `fibonacci.wasm` WebAssembly program. It contains a `fib()` function which takes a single integer as input parameter. We use `wasmedge` in reactor mode to call the exported function.

```bash
# cd <path/to/WasmEdge>
$ cd tools/wasmedge/examples
# wasmedge [-h|--help] [-v|--version] [--reactor] [--dir PREOPEN_DIRS ...] [--env ENVS ...] [--enable-bulk-memory] [--enable-reference-types] [--enable-simd] [--enable-all] [--allow-command COMMANDS ...] [--allow-command-all] [--] WASM_OR_SO [ARG ...]
$ wasmedge --reactor fibonacci.wasm fib 10
89
```

When wrong number of parameter given, the following error message is printed.

```bash
$ wasmedge --reactor fibonacci.wasm fib 10 10
2020-08-21 06:30:37,304 ERROR [default] execution failed: function signature mismatch, Code: 0x83
2020-08-21 06:30:37,304 ERROR [default]     Mismatched function type. Expected: params{i32} returns{i32} , Got: params{i32 , i32} returns{i32}
2020-08-21 06:30:37,304 ERROR [default]     When executing function name: "fib"
```

When calling unknown exported function, the following error message is printed.

```bash
$ wasmedge --reactor fibonacci.wasm fib2 10
2020-08-21 06:30:56,981 ERROR [default] wasmedge runtime failed: wasm function not found, Code: 0x04
2020-08-21 06:30:56,981 ERROR [default]     When executing function name: "fib2"
```

### Example: Factorial

We created the `factorial.wat` program by hand and used the [wat2wasm](https://github.com/WebAssembly/wabt) compiler to build the `factorial.wasm` WebAssembly program. It contains a `fac()` function which takes a single integer as input parameter. We use `wasmedge` in reactor mode to call the exported function.

```bash
# cd <path/to/WasmEdge>
$ cd tools/wasmedge/examples
# wasmedge [-h|--help] [-v|--version] [--reactor] [--dir PREOPEN_DIRS ...] [--env ENVS ...] [--enable-bulk-memory] [--enable-reference-types] [--enable-simd] [--enable-all] [--allow-command COMMANDS ...] [--allow-command-all] [--] WASM_OR_SO [ARG ...]
$ wasmedge --reactor factorial.wasm fac 5
120
```

### Example: JavaScript

It is possible to use WasmEdge as a high-performance, secure, extensible, easy to deploy, and OCI-compliant JavaScript runtime. The examples are in the `tools/wasmedge/examples/js` folder. The `qjs.wasm` program is a [JavaScript interpreter compiled into WebAssembly](https://github.com/second-state/wasmedge-quickjs). The `hello.js` file is a very simple JavaScript program.

```bash
$ cd tools/wasmedge/examples/js
$ wasmedge --dir .:. qjs.wasm hello.js 1 2 3
Hello 1 2 3
```

You can also run an interactive JavaScript terminal (read-eval-print loop, or REPL) from the WasmEdge CLI.

```bash
$ cd tools/wasmedge/examples/js
$ wasmedge --dir .:. qjs.wasm repl.js
QuickJS - Type "\h" for help
qjs >
```

The `qjs_tf.wasm` is a JavaScript interpreter with WasmEdge Tensorflow extension compiled into WebAssembly. To run `qjs_tf.wasm`, you must use the `wasmedge-tensorflow-lite` CLI tool, which is a build of WasmEdge with Tensorflow extension built-in. You can [download a full Tensorflow-based JavaScript example](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/tensorflow_lite_demo) to classify images.

```bash
$ cd tools/wasmedge/examples/js

# Download the Tensorflow example
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/aiy_food_V1_labelmap.txt
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/food.jpg
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/lite-model_aiy_vision_classifier_food_V1_1.tflite
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/main.js

$ wasmedge-tensorflow-lite --dir .:. qjs_tf.wasm main.js
label: Hot dog
confidence: 0.8941176470588236
```

## More ways to run WasmEdge

We have so far demonstrated that it is easy to run WasmEdge from the CLI. But in most real world applications, you probably want to embed WasmEdge in another application or host platform.

* Embed a standalone program (i.e., with `main()` function) in another platform.
  * [AWS Lambda](https://www.cncf.io/blog/2021/08/25/webassembly-serverless-functions-in-aws-lambda/) [[Template](https://github.com/second-state/aws-lambda-wasm-runtime)]
  * [Vercel Serverless Functions](https://www.secondstate.io/articles/vercel-wasmedge-webassembly-rust/) [[Template](https://github.com/second-state/vercel-wasm-runtime)]
  * [Netlify Functions](https://www.secondstate.io/articles/netlify-wasmedge-webassembly-rust-serverless/) [[Template](https://github.com/second-state/netlify-wasm-runtime)]
  * [Tencent Cloud Functions 腾讯云函数](https://github.com/second-state/tencent-scf-wasm-runtime)
  * [Dapr sidecar microservice](https://github.com/second-state/dapr-wasm)
* Embed a Wasm function in another platform.
  * [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/)
  * [Go](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/)
  * [Dapr sidecar microservice](https://github.com/second-state/dapr-wasm)
  * [Slack bots](http://reactor.secondstate.info/en/docs/user_guideline.html)
  * [飞书机器人](http://reactor.secondstate.info/zh/docs/user_guideline.html)

