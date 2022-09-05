# Rust

Rust is one of the "first-class citizen" programming languages in the WebAssembly ecosystem. All WasmEdge extensions to WebAssembly also come with Rust APIs for developers.
In this chapter, we will show you how to compile your Rust applications to WebAssembly and to run in the WasmEdge runtime.

## Prerequisites

You need to install [Rust](https://www.rust-lang.org/tools/install) and [WasmEdge](../quick_start/install.md) in order to get started.
You should also install the `wasm32-wasi` target of the Rust toolchain.

```bash
rustup target add wasm32-wasi
```

## Hello world

The Hello world example is a standalone Rust application that can be executed by the [WasmEdge CLI](../cli.md). Its [source code is available here](https://github.com/second-state/wasm-learning/tree/master/cli/hello).

The full source code for the Rust [main.rs](https://github.com/second-state/wasm-learning/blob/master/cli/hello/src/main.rs) file is as follows.
It echoes the command line arguments passed to this program at runtime.

```rust
use std::env;

fn main() {
  println!("hello");
  for argument in env::args().skip(1) {
    println!("{}", argument);
  }
}
```

### Hello world: Build the WASM bytecode

```bash
cargo build --target wasm32-wasi
```

### Hello world: Run the application from command line

We will use the `wasmedge` command to run the program.

```bash
$ wasmedge target/wasm32-wasi/debug/hello.wasm second state
hello
second
state
```

## A simple function

The [add example](https://github.com/second-state/wasm-learning/tree/master/cli/add) is a Rust library function that can be executed by the [WasmEdge CLI](../cli/wasmedge.md) in the reactor mode.

The full source code for the Rust [lib.rs](https://github.com/second-state/wasm-learning/blob/master/cli/add/src/lib.rs) file is as follows.
It provides a simple `add()` function.

```rust
#[no_mangle]
pub fn add(a: i32, b: i32) -> i32 {
  return a + b;
}
```

### A simple function: Build the WASM bytecode

```bash
cargo build --target wasm32-wasi
```

### A simple function: Run the application from command line

We will use `wasmedge` in reactor mode to run the program. We pass the function name and its input parameters as command line arguments.

```bash
$ wasmedge --reactor target/wasm32-wasi/debug/add.wasm add 2 2
4
```

## Pass Parameters with Complex Data Types

Of course, in most cases, you will not call functions using CLI arguments.
Instead, you will probably need to use a [language SDK from WasmEdge](../sdk.md) to call the function, pass call parameters, and receive return values.
Below are some SDK examples for complex call parameters and return values.

* [Use wasmedge-bindgen in a Go host app](../sdk/go/function.md)
* [Use direct memory passing in a Go host app](../sdk/go/memory.md)

## Improve the Performance

To achieve native Rust performance for those applications, you could use the `wasmedgec` command to AOT compile the `wasm` program, and then run it with the `wasmedge` command.

```bash
$ wasmedgec hello.wasm hello_aot.wasm

$ wasmedge hello_aot.wasm second state
hello
second
state
```

For the `--reactor` mode,

```bash
$ wasmedgec add.wasm add_aot.wasm

$ wasmedge --reactor add_aot.wasm add 2 2
4
```

## Further readings

* [Bindgen](rust/bindgen.md) can helper developers to create the WebAssembly library from Rust.
* [Access OS services via WASI](rust/wasi.md) shows how the WebAssembly program can access the underlying OS services, such as file system and environment variables.
* [Tensorflow](rust/tensorflow.md) shows how to create Tensorflow-based AI inference applications for WebAssembly using the WasmEdge TensorFlow Rust SDK.
* [Neural Network for WASI](rust/wasinn.md) shows how the WebAssembly program can leverage OpenVINO model to access the Machine Learning (ML) functions.
* [Crypto for WASI](rust/wasicrypto.md) shows how the WebAssembly program can access the crypto functions.
* [Simple networking socket](rust/networking.md) shows how to create simple HTTP client and server applications using the WasmEdge networking socket Rust SDK.
* [Simple networking socket in https](rust/networking-https.md) shows how to create simple HTTPS client and server applications using the WasmEdge networking socket Rust SDK.
* [Non-blocking networking socket](rust/networking-nonblocking.md) shows how to create a high-performance non-blocking networking applications with concurrent open connections using the WasmEdge networking socket Rust SDK.
* [Server-side rendering](rust/ssr.md) shows how to build an interactive web app with Rust, and then render the HTML DOM UI on the server using WasmEdge. The Rust source code is compiled to WebAssembly to render the HTML DOM in the browser or on the server.
* [Command interface](rust/command.md) shows how to create native command applications for WebAssembly using the Wasmedge command interface Rust SDK.
