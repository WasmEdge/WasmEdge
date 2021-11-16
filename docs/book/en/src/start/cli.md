# WasmEdge command line tool (CLI)

After [installing WasmEdge](install.md) or starting the [WasmEdge app dev Docker container](docker.md), there are several ways to run compiled WebAssembly programs. In this article, we will cover the most straighforward -- to run a WebAssembly program from the Linux command line (CLI).

* If the WebAssembly program contains a `main()` function, `wasmedge` would execute it as a standalone program in the command mode.
* If the WebAssembly program contains one or more public functions, `wasmedge` could execute individual functions in the reactor mode.

## Command line options

The options and flags for the `wasmedge` command are as follows.

1. (Optional) Reactor mode: use `--reactor` to enable reactor mode. In the reactor mode, `wasmedge` runs a specified function from the WebAssembly program.
	* WasmEdge will execute the function which name should be given in `ARG[0]`.
	* If there's exported function which names `_initialize`, the function will be executed with the empty parameter at first.
2. (Optional) Binding directories into WASI virtual filesystem.
	* Each directory can be specified as `--dir guest_path:host_path`.
3. (Optional) Environ variables.
	* Each variable can be specified as `--env NAME=VALUE`.
4. Wasm file (`/path/to/wasm/file`).
5. (Optional) Arguments.
	* In reactor mode, the first argument will be the function name, and the arguments after `ARG[0]` will be parameters of wasm function `ARG[0]`.
	* In command mode, the arguments will be parameters of function `_start`. They are also known as command line arguments for a standalone program.

Once installed, you can [review and run our examples](../index.md).
