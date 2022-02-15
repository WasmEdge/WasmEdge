# Rust

Rust 是 WebAssembly 生态系统中位居“一等公民”的编程语言之一。 WebAssembly 全部的 WasmEdge 扩展都为开发者提供 Rust APIs。在本篇中，我们将展示如何编译你的 Rust 应用到 wasm 字节码并在 WasmEdge 运行时上运行。

## 先决条件

你需要先安装 [Rust](https://www.rust-lang.org/tools/install) 和 [WasmEdge](../start/install.md)。同时也需要安装 `wasm32-wasi` 目标到 Rust 工具链中。

```bash
rustup target add wasm32-wasi
```

## Hello world

Hello world 示例是一个可以由 [WasmEdge CLI](../start/cli.md) 执行的独立 Rust 应用程序。[源代码在这里](https://github.com/second-state/wasm-learning/tree/master/cli/hello) 。

Rust [main.rs](https://github.com/second-state/wasm-learning/blob/master/cli/hello/src/main.rs) 文件源代码如下。它会输出运行时传递到这个程序的命令行参数。

```rust
use std::env;

fn main() {
  println!("hello");
  for argument in env::args().skip(1) {
    println!("{}", argument);
  }
}
```

### 构建 WASM 字节码

```bash
$ cargo build --target wasm32-wasi
```

### 从命令行运行应用

我们将使用 `wasmedge` 命令去运行程序。

```bash
$ wasmedge target/wasm32-wasi/debug/hello.wasm second state
hello
second
state
```

## 一个简单的函数

[add 示例](https://github.com/second-state/wasm-learning/tree/master/cli/add) 是能被 [WasmEdge CLI](../start/cli.md) 在 reactor 模式中执行的 Rust 库函数。

Rust  [lib.rs](https://github.com/second-state/wasm-learning/blob/master/cli/add/src/lib.rs) 文件源代码如下所示。它仅仅提供了 `add()` 函数。

```rust
#[no_mangle]
pub fn add(a: i32, b: i32) -> i32 {
  return a + b;
}
```

### 构建 WASM 字节码

```bash
$ cargo build --target wasm32-wasi
```

###  从命令行运行应用

我们使用 `wasmdege` 在 reactor 模型下运行程序。
我们将函数名和它的输入参数作为命令行参数传入。

```bash
$ wasmedge --reactor target/wasm32-wasi/debug/add.wasm add 2 2
4
```

### 传递复杂的调用参数

当然在大多数情况下，你不会使用命令行参数。相反，你可能需要使用 [来自 WasmEdge 的语言 SDK ](../../embed.md) 调用函数、传递调用参数并接收返回值。 接下来是一些包含复杂调用参数和返回值的 SDK 示例。

* [Use wasm-bindgen in a Node.js host app](../embed/node.md#more-examples)
* [Use wasm-bindgen in a Go host app](../embed/go/bindgen.md)
* [Use direct memory passing in a Go host app]()

## 提升性能

为了把这些程序提升到本地 Rust 性能， 你可以使用 `wasmedgec` 命令去 AOT 编译 `wasm` 程序，并用 `wasmedge` 运行它。

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

## 延伸阅读

* [通过 WASI 访问操作系统服务](rust/wasi.md) 展示了 WebAssembly 程序怎样访问底层操作系统服务，例如文件系统和环境变量。
* [Tensorflow](rust/tensorflow.md) 展示了如何使用 WasmEdge Tensorflow Rust SDK 为 WebAssembly 创建基于 Tensorflow 的人工智能推理应用。
* [Networking socket](rust/networking.md) 展示了如何使用 WasmEdge 网络套接字 Rust SDK 为 WebAssembly 创建网络应用。
* [Command interface](rust/command.md) 展示了使用 WasmEdge 命令行接口 Rust SDK 为  WebAssembly 创建本地命令行应用。
* [Bindgen and rustwasmc](rust/bindgen.md) 展示了使用 `rustwasmc` 工具链去编译 Rust 函数到 WebAssembly，并从外部主机程序传递复杂的调用参数给函数。 
