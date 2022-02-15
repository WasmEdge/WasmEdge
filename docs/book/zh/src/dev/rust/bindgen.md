# Bindgen 和 rustwasmc

[rustwasmc](https://github.com/second-state/rustwasmc) 工具的灵感来源于 wasm-pack 工程，并针对边缘计算和设备应用程序进行优化。具体地说，它支持了 [WasmEdge](https://github.com/WasmEdge/WasmEdge) WebAssembly 运行时。

相较于标准 `wasm32-wasi` 编译器目标，`rustwasmc` 的关键特性之一是使用 `wasm-bindgen` 工具处理编译的 Rust 函数。默认情况下，WebAssembly 函数只支持一些简单数据类型作为输入调用参数。而像 `wasm-bindgen` 这样的工具将 WebAssembly 函数参数转换为内存指针，并允许主机程序传递复杂参数给 WebAssembly 函数，例如字符串和数组。WasmEdge 的 [Node.js SDK](../../embed/node.md) 和 [Go SDK](../../embed/go.md) 都支持 `wasm-bindgen`，允许 JavaScript 和 Go 程序用复杂参数调用 WebAssembly 函数。

> 目前我们使用 1.50 或更低版本的 Rust 编译器才能将 WebAssembly 函数与 `wasm-bindgen` 和 `rustwasmc` 一起使用。只要接口类型规范最终确定并的到支持，我们将立刻赶上最新版本的 Rust 编译器。


## 前置条件

`rustwasmc` 需要依靠 Rust cargo 工具链去编译 Rust 源代码为 WebAssembly。所以，你需要安装在你的机器上安装 [Rust](https://www.rust-lang.org/tools/install) 。

```src
$ curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
$ source $HOME/.cargo/env
$ rustup override set 1.50.0
```


## 安装 

安装 [rustwasmc](https://github.com/second-state/rustwasmc) 最简洁的方式是使用它提供的安装程序。

```src
$ curl https://raw.githubusercontent.com/second-state/rustwasmc/master/installer/init.sh -sSf | sh
```

当然，你也可以使用 [NPM](https://github.com/second-state/rustwasmc#install) 去安装。


## 用法

使用下面的命令来构建 [Rust functions for Node.js](../../embed/node.md) 应用程序。请看一下 [模板程序](https://github.com/second-state/wasmedge-nodejs-starter) 。

```src
$ rustwasmc build
```

使用 `--enable-ext` 标志着编译使用 WASI 扩展的 Rust 程序，例如 WasmEdge 存储 API 和  [Tensorflow](tensorflow.md) API。在这种情况下，`rustwasmc` 将为 `wasmedge-extensions` Node.js 模块而不是 `wasmedge-core` 模块生成已编译的 WebAssembly 字节码程序。

```src
$ rustwasmc build --enable-ext
```

## 支持 AOT

WasmEdge 运行时的一个关键特性是对提前编译(AOT)编译器的支持。当你在 Node.js `wasmedge-core` 和 `wasmedge-extensions` 插件中运行 WebAssembly 程序时，你通常不需要担心它，因为插件会透明地处理 AOT 编译。然而，在一些情况下，你只希望使用 `rustwasmc` 编译程序并生成本地代码。

那么就需要使用以下命令是你的操作系统和开发者工具保持同步。此处命令在 Ubuntu 20.04 上测试。

```src
$ sudo apt-get update
$ sudo apt-get -y upgrade
$ sudo apt install build-essential curl wget git vim libboost-all-dev llvm-dev liblld-10-dev
```

现在，你可以按下面的命令为 AOT 本地目标构建 `.so` 文件了。

```src
$ rustwasmc build --enable-aot
```

享受编程吧！
