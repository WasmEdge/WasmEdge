# Rust

在 Webassembly 生态体系中，Rust 是其中一个一等公民。有关 Webassembly 的所有 WasmEdge 扩展程序都为开发者提供了相应的 Rust API 。在这一章节中，我们会为你展示如何将你的Rust应用程序编译为 wasm 字节码，并在 WasmEdge Runtime 中运行你的程序。

## 前置要求

在学习这节内容前，你需要安装 [Rust](https://www.rust-lang.org/tools/install) 以及 [WasmEdge](../start/install.md)。同时，你也需要在Rust工具链中安装 `wasm32-wasi` 。命令如下所示：

```bash
$ rustup target add wasm32-wasi
```

## Hello world

Hello world 示例是一个可以通过 [WasmEdge CLI](../start/cli.md)执行的Rust应用程序。如果需要阅读源码，可以通过这个链接查看 [源码](https://github.com/second-state/wasm-learning/tree/master/cli/hello)。

这个 Rust 程序的 [main.rs](https://github.com/second-state/wasm-learning/blob/master/cli/hello/src/main.rs) 文件中的完整代码如下所示。这个程序会在运行时回显我们传入的命令行参数。

```rust
use std::env;

fn main() {
  println!("hello");
  for argument in env::args().skip(1) {
    println!("{}", argument);
  }
}
```

### 构建WASM字节码

```bash
$ cargo build --target wasm32-wasi
```

### 通过命令行运行应用程序

我们会使用 `wasmedge` 命令来运行这个程序。

```bash
$ wasmedge target/wasm32-wasi/debug/hello.wasm second state
hello
second
state
```

## 一个简单的函数

[add示例](https://github.com/second-state/wasm-learning/tree/master/cli/add) 是一个可以通过[WasmEdge CLI](../start/cli.md)以reactor模式执行的 Rust library 函数。

Rust [lib.rs](https://github.com/second-state/wasm-learning/blob/master/cli/add/src/lib.rs) 文件的完整代码如下所示。
它提供了一个简单的 `add()` 函数。


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

### 通过命令行运行应用程序

我们会使用 `wasmedge` 以 reactor 模式来运行这个程序. 我们会以命令行参数的形式传入我们要调用的方法名以及对应的入参。

```bash
$ wasmedge --reactor target/wasm32-wasi/debug/add.wasm add 2 2
4
```

### 传入复杂调用参数

当然，在大多数情况下，你们不会通过传入CLI参数来调用函数。相反，你们可能会需要使用 [WasmEdge提供的那些编程语言对应的SDK](../../embed.md) 来调用函数。传入参数，然后就能得到返回结果。下方展示的这些SDK案例就是告诉你们如何传入复杂参数并获取返回值。

* [在 Node.js 应用程序中使用 wasm-bindgen](../embed/node.md#more-examples)
* [在 Go 应用程序中使用 wasm-bindgen](../embed/go/bindgen.md)
* [在 Go 应用程序中使用直接内存传递]()

## 提升性能

为了让这些应用程序能获得如 Rust 原生那样的性能，你可以使用 `wasmedgec` 使用 AOT 编译来编译这个 `wasm` 程序，接着使用 `wasmedge` 命令来运行这个程序。

```bash
$ wasmedgec hello.wasm hello.wasm

$ wasmedge hello.wasm second state
hello
second
state
```

如何启用 `--reactor` 模式,

```bash
$ wasmedgec add.wasm add.wasm

$ wasmedge --reactor add.wasm add 2 2
4
```

## 拓展阅读

* [Access OS services via WASI](rust/wasi.md) 这篇文章展示了 WebAssembly 程序是如何访问底层的OS服务，例如：访问文件系统和环境变量。
* [Tensorflow](rust/tensorflow.md) 展示了如何使用 WasmEdge TensorFlow Rust SDK 为 WebAssembly 创建基于 TensorFlow 的 AI 推理应用程序。
* [Networking socket](rust/networking.md) 展示了如何使用 WasmEdge 网络 socket Rust SDK 来为 WebAssembly 创建网络应用程序。
* [Command interface](rust/command.md) 展示了如何通过 Wasmedge command interface Rust SDK 来创建原生的 Webassembly 命令程序。
* [Bindgen and rustwasmc](rust/bindgen.md) 展示了通过 `rustwasmc` 工具链将 Rust 函数编译为 WebAssembly ，然后从外部应用程序向该函数传入复杂调用参数。
