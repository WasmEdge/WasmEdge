# Execution in AOT Mode

The `wasmedge` command line tool will execute the original WASM files in interpreter mode. For the much better performance, we recommand users to compile the WASM with the `wasmedgec` AOT compiler to execute the WASM in AOT mode. There are 2 output formats of the AOT compiler:

## Output Format: Universal WASM

By default, the `wasmedgec` AOT compiler tool could wrap the AOT-compiled native binary into a custom section in the origin WASM file. We call this the universal WASM binary format.

This AOT-compiled WASM file is compatible with any WebAssembly runtime. However, when this WASM file is executed by the WasmEdge runtime, WasmEdge will extract the native binary from the custom section and execute it in AOT mode.

> Note: On MacOS platforms, the universal WASM format will `bus error` in execution. It's because the `wasmedgec` tool optimizes the WASM in `O2` level by default. We are trying to fix this issue. For working around, please use the shared library output format instead.

```bash
wasmedgec app.wasm app_aot.wasm
wasmedge app_aot.wasm
```

## Output Format: Shared Library

Users can assign the shared library extension for the output files (`.so` on Linux, `.dylib` on MacOS, and `.dll` on Windows) to generate the shared library output format output.

This AOT-compiled WASM file is only for WasmEdge use, and cannot be used by other WebAssembly runtimes.

```bash
wasmedgec app.wasm app_aot.so
wasmedge app_aot.so
```
