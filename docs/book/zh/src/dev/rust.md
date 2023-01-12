# Rust

在 WebAssembly 生态中，Rust 是一等公民编程语言之一。对开发者来说，所有编译为 WebAssembly 的 WasmEdge 拓展都有相对应的 Rust 的开发接口。
在这一章节中，我们将会向你展示如何将你的 Rust 应用程序编译为 WASM 字节码，并且在 WasmEdge Runtime 中运行它。

## 前置条件

开始前，你需要安装 [Rust](https://www.rust-lang.org/tools/install) 以及  [WasmEdge](../start/install.md)。
同时你也应该将 `wasm32-wasi` 添加到 Rust 工具链中。

```bash
rustup target add wasm32-wasi
```

## Hello world

Hello world 示例是一个独立的 Rust 应用程序，可以被 [WasmEdge 命令行接口](../start/cli.md) 执行。它的[源代码在这里](https://github.com/second-state/wasm-learning/tree/master/cli/hello)。

如下是 [main.rs](https://github.com/second-state/wasm-learning/blob/master/cli/hello/src/main.rs) 的完整代码，它将输出在运行的时候接收到的命令行参数。

```rust
use std::env;

fn main() {
  println!("hello");
  for argument in env::args().skip(1) {
    println!("{}", argument);
  }
}
```

### Hello world： 构建 WASM 字节码

```bash
cargo build --target wasm32-wasi
```

### Hello world： 在命令行中运行应用程序

我们将使用 `wasmedge` 命令来运行这个程序。

```bash
$ wasmedge target/wasm32-wasi/debug/hello.wasm second state
hello
second
state
```

## 一个简单的函数

[add 示例](https://github.com/second-state/wasm-learning/tree/master/cli/add)是一个 Rust 库函数，可以被 [WasmEdge 命令行接口](../start/cli.md) 接口在 `--reactor` 模式下执行。

如下是 [lib.rs](https://github.com/second-state/wasm-learning/blob/master/cli/add/src/lib.rs) 的完整代码，它将输出在运行的时候接收到的命令行参数。
它提供了一个简单的 `add()` 函数。

```rust
#[no_mangle]
pub fn add(a: i32, b: i32) -> i32 {
  return a + b;
}
```

### 一个简单的函数： 构建 WASM 字节码

```bash
cargo build --target wasm32-wasi
```

### 一个简单的函数： 在命令行中运行应用程序

我们将使用 WasmEdge 的 `--reactor` 模式来运行这个程序。我们将函数的名字以及它的输入参数作为命令行参数。

```bash
$ wasmedge --reactor target/wasm32-wasi/debug/add.wasm add 2 2
4
```

## 传递复杂的参数

当然，在大多数情况下，你不会使用命令行参数来调用函数。
相反地，你可能会需要使用一个 [WasmEdge 提供的语言 SDK](../../embed.md) 来调用函数、传递参数以及接收返回值。
以下是一些关于复杂参数和返回值的 SDK 示例。

* [在 GO 程序中使用 wasmedge-bindgen](../embed/go/function.md)
* [在 GO 程序中直接传递内存](../embed/go/memory.md)

## 提升性能

如果要让这些应用程序达到原生 Rust 的性能，你可以使用 `wasmedgec` 命令对 `wasm` 程序进行提前编译（AOT），然后使用 `wasmedge` 命令运行它。

```bash
$ wasmedgec hello.wasm hello.wasm

$ wasmedge hello.wasm second state
hello
second
state
```

对于 `--reactor` 模式，

```bash
$ wasmedgec add.wasm add.wasm

$ wasmedge --reactor add.wasm add 2 2
4
```

## 更多资料

* [通过 WASI 访问系统服务](rust/wasi.md) 展示了 WebAssembly 程序如何访问底层系统服务，比如文件系统和环境变量。
* [Tensorflow](rust/tensorflow.md) 展示了如何使用 WasmEdge Tensorflow Rust SDK 来为 WebAssembly 创建基于 Tensorflow 的 AI 推理应用程序。
* [简单的网络通信](rust/networking.md) 展示了如何使用 WasmEdge 网络通信 Rust SDK 来创建简单的 HTTP 客户端以及服务端应用程序。
* [非阻塞的网络通信](rust/networking-nonblocking.md) 展示了如何使用 WasmEdge 网络通信 Rust SDK 来创建一个高性能、非阻塞、并发连接的网络应用程序。
* [服务器端渲染](rust/ssr.md) 展示了如何使用 Rust 构建一个可交互的 Web 程序，并在服务器上使用 WasmEdge 来渲染 HTML DOM UI。用来渲染 HTML DOM 的 Rust 源代码会被编译为 WebAssembly，在浏览器中或是服务器上运行。
* [命令接口](rust/command.md) 展示了如何使用 WasmEdge 命令行接口 Rust SDK 来为 WebAssembly 创建原生的命令行应用程序。
