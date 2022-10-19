# `wasmedgec` CLI

After [installation](../quick_start/install.md), users can execute the `wasmedgec` tool with commands.

```bash
$ wasmedgec -v
wasmedge version {{ wasmedge_version }}
```

The usage of the `wasmedge` tool will be:

```bash
$ wasmedge -h
USAGE
   wasmedgec [OPTIONS] [--] WASM WASM_SO

...
```

The `wasmedgec` can compile WebAssembly into native machine code (i.e., the AOT compiler).
For the pure WebAssembly, the `wasmedge` tool will execute the WASM in interpreter mode.
After compiling with the `wasmedgec` AOT compiler, the `wasmedge` tool can execute the WASM in AOT mode which is much faster.

## Options

The options of the `wasmedgec` CLI tool are as follows.

1. `-v|--version`: Show the version information. Will ignore other arguments below.
2. `-h|--help`: Show the help messages. Will ignore other arguments below.
3. (Optional) `--dump`: Dump the LLVM IR to `wasm.ll` and `wasm-opt.ll`.
4. (Optional) `--interruptible`: Generate the binary which supports interruptible execution.
    * By default, the AOT-compiled WASM not supports [interruptions in asynchronous executions](../sdk/c/ref.md#async).
5. (Optional) Statistics informations:
    * By default, the AOT-compiled WASM not supports all statistics even if the options are turned on when running the `wasmedge` tool.
    * Use `--enable-time-measuring` to generate code for enabling the statistics of time measuring in execution.
    * Use `--enable-gas-measuring` to generate code for enabling the statistics of gas measuring in execution.
    * Use `--enable-instruction-count` to generate code for enabling the statistics of counting WebAssembly instructions.
    * Or use `--enable-all-statistics` to generate code for enabling all of the statistics.
6. (Optional) `--generic-binary`: Generate the generic binary of the current host CPU architecture.
7. (Optional) WebAssembly proposals:
    * Use `--disable-import-export-mut-globals` to disable the [Import/Export of Mutable Globals](https://github.com/WebAssembly/mutable-global) proposal (Default `ON`).
    * Use `--disable-non-trap-float-to-int` to disable the [Non-Trapping Float-to-Int Conversions](https://github.com/WebAssembly/nontrapping-float-to-int-conversions) proposal (Default `ON`).
    * Use `--disable-sign-extension-operators` to disable the [Sign-Extension Operators](https://github.com/WebAssembly/sign-extension-ops) proposal (Default `ON`).
    * Use `--disable-multi-value` to disable the [Multi-value](https://github.com/WebAssembly/multi-value) proposal (Default `ON`).
    * Use `--disable-bulk-memory` to disable the [Bulk Memory Operations](https://github.com/WebAssembly/bulk-memory-operations) proposal (Default `ON`).
    * Use `--disable-reference-types` to disable the [Reference Types](https://github.com/WebAssembly/reference-types) proposal (Default `ON`).
    * Use `--disable-simd` to disable the [Fixed-width SIMD](https://github.com/webassembly/simd) proposal (Default `ON`).
    * Use `--enable-multi-memory` to enable the [Multiple Memories](https://github.com/WebAssembly/multi-memory) proposal (Default `OFF`).
    * Use `--enable-tail-call` to enable the [Tail call](https://github.com/WebAssembly/tail-call) proposal (Default `OFF`).
    * Use `--enable-extended-const` to enable the [Extended Constant Expressions](https://github.com/WebAssembly/extended-const) proposal (Default `OFF`).
    * Use `--enable-threads` to enable the [Threads](https://github.com/webassembly/threads) proposal (Default `OFF`).
    * Use `--enable-all` to enable ALL proposals above.
8. (Optional) `--optimize`: Select the LLVM optimization level.
    * Use `--optimize LEVEL` to set the optimization level. The `LEVEL` should be one of `0`, `1`, `2`, `3`, `s`, or `z`.
    * The default value will be `2`, which means `O2`.
9. Input WASM file (`/path/to/wasm/file`).
10. Output path (`/path/to/output/file`).
    * By default, the `wasmedgec` tool will output the [universal WASM format](../quick_start/run_in_aot_mode.md#output-format-universal-wasm).
    * If the specific file extension (`.so` on Linux, `.dylib` on MacOS, and `.dll` on Windows) is assigned in the output path, the `wasmedgec` tool will output the [shared library format](../quick_start/run_in_aot_mode.md#output-format-shared-library).

## Example

Take the [fibonacci.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wasm) for example.
It exported a `fib()` function which takes a single `i32` integer as the input parameter.

You can run:

```bash
wasmedgec fibonacci.wasm fibonacci_aot.wasm
```

or:

```bash
wasmedgec fibonacci.wasm fibonacci_aot.so # On Linux.
```

The output will be:

```bash
[2022-09-09 14:22:10.540] [info] compile start
[2022-09-09 14:22:10.541] [info] verify start
[2022-09-09 14:22:10.542] [info] optimize start
[2022-09-09 14:22:10.547] [info] codegen start
[2022-09-09 14:22:10.552] [info] output start
[2022-09-09 14:22:10.600] [info] compile done
```

Then you can execute the output file with `wasmedge` and measure the execution time:

```bash
time wasmedge --reactor fibonacci_aot.wasm fib 30
```

The output will be:

```bash
1346269

real    0m0.029s
user    0m0.012s
sys     0m0.014s
```

Then you can compare it with the interpreter mode:

```bash
time wasmedge --reactor fibonacci.wasm fib 30
```

The output shows that the AOT-compiled WASM is much faster than the interpreter mode:

```bash
1346269

real    0m0.442s
user    0m0.427s
sys     0m0.012s
```
