# Run WasmEdge from CLI

There are several ways to run compiled WebAssembly programs in the WasmEdge VM. In this article, we will cover the most straighforward -- to run a WebAssembly program from the Linux command line (CLI).

* If the WebAssembly program contains a `main()` function, `wasmedge` would execute it as a standalone program in the command mode. 
* If the WebAssembly program contains one or more public functions, `wasmedge` could execute individual functions in the reactor mode.

## Command line options

The options and flags for the `wasmedge` command are as follows.

1. (Optional) Reactor mode: use `--reactor` to enable reactor mode. In the reactor mode, `wasmedge` runs a specified function from the WebAssembly program.
	* WasmEdge will execute the function which name should be given in ARG[0].
	* If there's exported function which names `_initialize`, the function will be executed with the empty parameter at first.
2. (Optional) Binding directories into WASI virtual filesystem.
	* Each directory can be specified as `--dir host_path:guest_path`.
3. (Optional) Environ variables.
	* Each variable can be specified as `--env NAME=VALUE`.
4. Wasm file(`/path/to/wasm/file`).
5. (Optional) Arguments.
	* In reactor mode, the first argument will be the function name, and the arguments after ARG[0] will be parameters of wasm function ARG[0].
	* In command mode, the arguments will be parameters of function `_start`. They are also known as command line arguments for a standalone program.

## Examples

Here are some examples on how use the `wasmedge` command to run WebAssembly programs.

### Example: Hello world

The `hello.wasm` WebAssembly program contains a `main()` function. Checkout its Rust [source code project](https://github.com/second-state/wasm-learning/tree/master/cli/hello). It prints out `hello` followed by the command line arguments. We use `wasmedge` in command mode to run the standalone program.

```bash
# cd <path/to/wasmedge/build_folder>
$ cd tools/wasmedge
$ ./wasmedge examples/hello.wasm second state
hello
second
state
```

### Example: Add

The `add.wasm` WebAssembly program contains a `add()` function. Checkout its Rust [source code project](https://github.com/second-state/wasm-learning/tree/master/cli/add). We use `wasmedge` in reactor mode to call the `add()` with two integer input parameters.

```bash
# cd <path/to/wasmedge/build_folder>
$ cd tools/wasmedge
$ ./wasmedge --reactor examples/add.wasm add 2 2
4
```

> WebAssembly only supports a few simple data types. To call a wasm function with complex input parameters and return values in reactor mode, you will need the [rustwasmc](https://github.com/second-state/rustwasmc) compiler toolchain to generate wasm functions that can be embedded into [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) or [Golang](https://github.com/second-state/yomo-flow-ssvm-example) programs.

### Example: Fibonacci

The `fibonacci.wasm` WebAssembly program contains a `fib()` function which takes a single integer as input parameter. We use `wasmedge` in reactor mode to call the exported function.

```bash
# cd <path/to/wasmedge/build_folder>
$ cd tools/wasmedge
# ./wasmedge [-h|--help] [-v|--version] [--reactor] [--dir PREOPEN_DIRS ...] [--env ENVS ...] [--enable-bulk-memory] [--enable-reference-types] [--enable-simd] [--enable-all] [--allow-command COMMANDS ...] [--allow-command-all] [--] WASM_OR_SO [ARG ...]
$ ./wasmedge --reactor examples/fibonacci.wasm fib 10
89
```

When wrong number of parameter given, the following error message is printed.

```bash
$ ./wasmedge --reactor examples/fibonacci.wasm fib 10 10
2020-08-21 06:30:37,304 ERROR [default] execution failed: function signature mismatch, Code: 0x83
2020-08-21 06:30:37,304 ERROR [default]     Mismatched function type. Expected: params{i32} returns{i32} , Got: params{i32 , i32} returns{i32}
2020-08-21 06:30:37,304 ERROR [default]     When executing function name: "fib"
```

When calling unknown exported function, the following error message is printed.

```bash
$ ./wasmedge --reactor examples/fibonacci.wasm fib2 10
2020-08-21 06:30:56,981 ERROR [default] wasmedge runtime failed: wasm function not found, Code: 0x04
2020-08-21 06:30:56,981 ERROR [default]     When executing function name: "fib2"
```

### Example: Factorial

The `factorial.wasm` WebAssembly program contains a `fac()` function which takes a single integer as input parameter. We use `wasmedge` in reactor mode to call the exported function.

```bash
# ./wasmedge [-h|--help] [-v|--version] [--reactor] [--dir PREOPEN_DIRS ...] [--env ENVS ...] [--enable-bulk-memory] [--enable-reference-types] [--enable-simd] [--enable-all] [--allow-command COMMANDS ...] [--allow-command-all] [--] WASM_OR_SO [ARG ...]
$ ./wasmedge --reactor examples/factorial.wasm fac 5
120
```

## More ways to run WasmEdge

We have so far demonstrated that it is easy to run WasmEdge from the CLI. But in most real world applications, you probably want to embed WasmEdge in another application or host platform. 

* Embed a standalone program (i.e., with `main()` function) in another platform.
  * [Node.js](https://github.com/second-state/wasm-learning/tree/master/ssvm/file-example)
  * [Golang app](https://github.com/second-state/yomo-flow-ssvm-example)
  * [Tencent Serverless container](https://github.com/second-state/tencent-tensorflow-scf/blob/main/README-en.md)
* Embed a Wasm function in another platform.
  * [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/)
  * [Feishu chatbot platform in Chinese](http://reactor.secondstate.info/docs/user-create-a-bot.html)



