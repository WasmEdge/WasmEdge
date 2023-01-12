# Running WASM with WasmEdge CLI

After [installing WasmEdge](install.md) or starting the [WasmEdge app dev Docker container](use_docker.md), there are several ways to run WebAssembly programs.

## `wasmedge` CLI

The `wasmedge` binary is a command line interface (CLI) program that runs WebAssembly programs.

* If the WebAssembly program contains a `main()` function, `wasmedge` would execute it as a standalone program in the command mode.
* If the WebAssembly program contains one or more exported public functions, `wasmedge` could invoke individual functions in the reactor mode.

By default, the `wasmedge` will execute WebAssembly programs in interpreter mode, and [execute the AOT-compiled `.so`, `.dylib`, `.dll`, or `.wasm` (universal output format) in AOT mode](run_in_aot_mode.md). If you want to accelerate the WASM execution, we recommend to [compile the WebAssembly with the AOT compiler](#wasmedgec-cli) first.

Users can run the `wasmedge -h` for realizing the command line options quickly, or [refer to the detailed `wasmedge` CLI options here](../cli/wasmedge.md).

### Call A WebAssembly Function Written in WAT

We created the hand-written [fibonacci.wat](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wat) and used the [wat2wasm](https://webassembly.github.io/wabt/demo/wat2wasm/) tool to convert it into the [fibonacci.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wasm) WebAssembly program.
It exported a `fib()` function which takes a single `i32` integer as the input parameter. We can execute `wasmedge` in reactor mode to invoke the exported function.

You can run:

```bash
wasmedge --reactor fibonacci.wasm fib 10
```

The output will be:

```bash
89
```

### Call A WebAssembly Function Compiled From Rust

The [add.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/add.wasm) WebAssembly program contains an exported `add()` function, which is compiled from Rust.
Checkout its [Rust source code project here](https://github.com/second-state/wasm-learning/tree/master/cli/add).
We can execute `wasmedge` in reactor mode to invoke the `add()` function with two `i32` integer input parameters.

You can run:

```bash
wasmedge --reactor add.wasm add 2 2
```

The output will be:

```bash
4
```

### Execute A Standalone WebAssembly Program: Hello world

The [hello.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/hello.wasm) WebAssembly program contains a `main()` function.
Checkout its [Rust source code project here](https://github.com/second-state/wasm-learning/tree/master/cli/hello).
It prints out `hello` followed by the command line arguments.

You can run:

```bash
wasmedge hello.wasm second state
```

The output will be:

```bash
hello
second
state
```

## `wasmedgec` CLI

The `wasmedgec` binary is a CLI program to compile WebAssembly into native machine code (i.e., the AOT compiler). For the pure WebAssembly, the `wasmedge` tool will execute the WASM in interpreter mode. After compiling with the AOT compiler, the `wasmedge` tool can execute the WASM in AOT mode which is much faster.

The options and flags for the `wasmedgec` are as follows.

1. Input Wasm file(`/path/to/input/wasm/file`).
2. Output file name(`/path/to/output/file`).
   * By default, it will generate the [universal Wasm binary format](run_in_aot_mode.md#output-format-universal-wasm).
   * Users can still generate native binary only by specifying the `.so`, `.dylib`, or `.dll` extensions.

Users can run the `wasmedgec -h` for realizing the command line options quickly, or [refer to the detailed `wasmedgec` CLI options here](../cli/wasmedgec.md).

```bash
# This is slow in interpreter mode.
wasmedge app.wasm

# AOT compilation.
wasmedgec app.wasm app_aot.wasm

# This is now MUCH faster in AOT mode.
wasmedge app_aot.wasm
```
