# Rust

Rust is one of the "first-class citizen" programming languages in the WebAssembly ecosystem. All WasmEdge extensions to WebAssembly also come with Rust APIs
for developers.
In this chapter, we will show you how to compile your Rust applications to wasm bytecode and to run in the WasmEdege runtime.

## Prerequisites

You need to install [Rust](https://www.rust-lang.org/tools/install) and [WasmEdge](../start/install.md) in order to get started.
You should also install the `wasm32-wasi` target to the Rust toolchain.

```bash
rustup target add wasm32-wasi
```

## Hello world

The Hello world example is a standalone Rust application that can be executed
by the [WasmEdge CLI](../start/cli.md). Its [source code is available](https://github.com/second-state/wasm-learning/tree/master/cli/hello).

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

### Build the WASM bytecode

```bash
$ cargo build --target wasm32-wasi
```

### Run the application from command line

We will use the `wasmedge` command to run the program.

```bash
$ wasmedge target/wasm32-wasi/debug/hello.wasm second state
hello
second
state
```

## A simple function

The [add example](https://github.com/second-state/wasm-learning/tree/master/cli/add) is a Rust library function that can be executed
by the [WasmEdge CLI](../start/cli.md) in the reactor mode.

The full source code for the Rust [lib.rs](https://github.com/second-state/wasm-learning/blob/master/cli/add/src/lib.rs) file is as follows.
It provides a simple `add()` function.

```rust
#[no_mangle]
pub fn add(a: i32, b: i32) -> i32 {
  return a + b;
}
```

### Build the WASM bytecode

```bash
$ cargo build --target wasm32-wasi
```

### Run the application from command line

We will use `wasmedge` in reactor mode to run the program. We pass the function name and its input parameters as command line arguments.

```bash
$ wasmedge --reactor target/wasm32-wasi/debug/add.wasm add 2 2
4
```

## Improve performance

To achieve native Rust performance for those applications, you
could use the `wasmedgec` command to AOT compile the `wasm` program,
and then run it with the `wasmedge` command.

```bash
$ wasmedgec hello.wasm hello.wasm

$ wasmedge hello.wasm second state
hello
second
state
```

For the `--reactor` mode,

```bash
$ wasmedgec add.wasm add.wasm

$ wasmedge --reactor add.wasm add 2 2
4
```

## Further readings

* [Bindgen and rustwasmc](rust/bindgen.md) shows how to use the rustwasmc toolchain to compile Rust functions into WebAssembly bytecode programs that can be easily called by external applications.
* [Tensorflow](rust/tensorflow.md) shows how to create Tensorflow-based AI inference applications for WebAssembly using the WasmEdge TensorFlow Rust SDK.
* [Networking socket](rust/networking.md) shows how to create networking applications for WebAssembly using the WasmEdge networking socket Rust SDK.
* [Command interface](rust/command.md) shows how to create native command applications for WebAssembly using the Wasmedge command interface Rust SDK.


