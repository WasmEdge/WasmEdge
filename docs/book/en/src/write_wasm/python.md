# Python

There are already several different language implementations of the Python runtime, and some of them support WebAssembly. This document will describe how to run [RustPython](https://github.com/RustPython/RustPython) on WasmEdge to execute Python programs.

## Compile RustPython

To compile RustPython, you should have the Rust toolchain installed on your machine. And `wasm32-wasi` platform support should be enabled.

```bash
rustup target add wasm32-wasi
```

Then you could use the following command to clone and compile RustPython:

```bash
git clone https://github.com/RustPython/RustPython.git
cd RustPython
cargo build --release --target wasm32-wasi --features="freeze-stdlib"
```

`freeze-stdlib` feature is enabled for including Python standard library inside the binary file. The output file should be able at `target/wasm32-wasi/release/rustpython.wasm`.

## AOT Compile

WasmEdge supports compiling WebAssembly bytecode programs into native machine code for better performance. It is highly recommended to compile the RustPython to native machine code before running.

```bash
wasmedgec ./target/wasm32-wasi/release/rustpython.wasm ./target/wasm32-wasi/release/rustpython.wasm
```

## Run

```bash
wasmedge ./target/wasm32-wasi/release/rustpython.wasm
```

Then you could get a Python shell in WebAssembly!

## Grant file system access

You can pre-open directories to let WASI programs have permission to read and write files stored on the real machine. The following command mounted the current working directory to the WASI virtual file system.

```bash
wasmedge --dir .:. ./target/wasm32-wasi/release/rustpython.wasm
```
