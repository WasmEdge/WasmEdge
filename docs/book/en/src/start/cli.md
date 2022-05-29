# WasmEdge command line tool (CLI)

After [installing WasmEdge](install.md) or starting the [WasmEdge app dev Docker container](docker.md), there are several ways to run compiled WebAssembly programs.

## wasmedge

The `wasmedge` binary file is a command line interface (CLI) program that runs WebAssembly bytecode programs.

* If the WebAssembly program contains a `main()` function, `wasmedge` would execute it as a standalone program in the command mode.
* If the WebAssembly program contains one or more public functions, `wasmedge` could execute individual functions in the reactor mode.

### Command line options

The options and flags for the `wasmedge` command are as follows.

1. (Optional) Statistics information:
   * Use `--enable-time-measuring` to show the execution time.
   * Use `--enable-gas-measuring` to show the amount of used gas.
   * Use `--enable-instruction-count` to display the number of executed instructions.
   * Or use `--enable-all-statistics` to enable all of the statistics options.
2. (Optional) Resource limitation:
   * Use `--gas-limit` to limit the execution cost.
   * Use `--memory-page-limit` to set the limitation of pages(as size of 64 KiB) in every memory instance.
3. (Optional) Reactor mode: use `--reactor` to enable reactor mode. In the reactor mode, `wasmedge` runs a specified function from the WebAssembly program.
   * WasmEdge will execute the function which name should be given in `ARG[0]`.
   * If there's exported function which names `_initialize`, the function will be executed with the empty parameter at first.
4. (Optional) Binding directories into WASI virtual filesystem.
   * Each directory can be specified as `--dir guest_path:host_path`.
5. (Optional) Environ variables.
   * Each variable can be specified as `--env NAME=VALUE`.
6. Wasm file (`/path/to/wasm/file`).
7. (Optional) Arguments.
   * In reactor mode, the first argument will be the function name, and the arguments after `ARG[0]` will be parameters of wasm function `ARG[0]`.
   * In command mode, the arguments will be parameters of function `_start`. They are also known as command line arguments for a standalone program.

Once installed, you can [review and run our examples](../index.md).

## wasmedgec

The `wasmedgec` binary file is a program to compile WebAssembly bytecode programs into native machine code (i.e., the AOT compiler).
The compiled machine code could be [stored in the original `wasm` file](universal.md), and the `wasmedge` CLI will automatically choose to execute the native machine code whenever it is available.

The options and flags for the `wasmedgec` are as follows.

1. Input Wasm file(`/path/to/input/wasm/file`).
2. Output file name(`/path/to/output/file`).
   * By default, it will generate the [universal Wasm binary format](universal.md).
   * Users can still generate native binary only by specifying the `.so`, `.dylib`, or `.dll` extensions.

```bash
# This is slow
wasmedge app.wasm

# AOT compile
wasmedgec app.wasm app.wasm

# This is now MUCH faster
wasmedge app.wasm
```

On Linux systems, it could generate a `so` shared library file, which is then executed by the `wasmedge` CLI.

```bash
wasmedgec app.wasm app.so
wasmedge app.so
```
