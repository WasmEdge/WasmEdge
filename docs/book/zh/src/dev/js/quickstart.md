# 快速开始

首先，让我们为 WasmEdge 构建一个基于 WebAssembly 的 JavaScript 解释器程序。这个程序基于 [QuickJS](https://bellard.org/quickjs/) 和 WasmEdge 的一些扩展程序，比如 [network sockets](https://github.com/second-state/wasmedge_wasi_socket) 和 [Tensorflow 推理](https://www.secondstate.io/articles/wasi-tensorflow/)，并且这些扩展都是作为 JavaScript API 被合并到解释器中的。首先你需要[安装 rust](https://www.rust-lang.org/tools/install) 来构建这个解释器。

> 如果你只是想要使用这个解释器来运行 JavaScript 程序，那就可以跳过这部分了。否则，先确保你已经安装了 [Rust](https://www.rust-lang.org/tools/install) 和 [WasmEdge](../../start/install.md)。

Fork 或者克隆 [wasmedge-quickjs](https://github.com/second-state/wasmedge-quickjs) 这个 Github repo 开始教程。

```bash
git clone https://github.com/second-state/wasmedge-quickjs
```

按照这个 repo 中的介绍，你将能够为 WasmEdge 构建一个 JavaScript 解释器。

```bash
# Install GCC
sudo apt update
sudo apt install build-essential

# Install wasm32-wasi target for Rust
rustup target add wasm32-wasi

# Build the QuickJS JavaScript interpreter
cargo build --target wasm32-wasi --release
```

构建成功后，基于 WebAssembly 的解释器程序位于 build target 目录中，你现在可以尝试执行一个简单的 "hello world" JavaScript 程序 ([example_js/hello.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/hello.js))，它会在终端中打印出命令行参数。

```javascript
import * as os from 'os';
import * as std from 'std';

args = args.slice(1);
print('Hello', ...args);
setTimeout(() => {
  print('timeout 2s');
}, 2000);
```

接下来输入下面的命令在 WasmEdge 的 QuickJS runtime 运行 `hello.js` 文件。

```bash
$ cd example_js
$ wasmedge --dir .:. ../target/wasm32-wasi/release/wasmedge_quickjs.wasm hello.js WasmEdge Runtime
Hello WasmEdge Runtime
```

> 注意：命令行里的 `--dir .:.` 这个参数是为了让 `wasmedge` 命令有权限在文件系统中读取本地目录里的 `hello.js` 文件。我们将在下面的章节中继续用到 `--dir .:.` 这个参数。

## 更快地执行

WasmEdge 提供了一个 `wasmedgec` 实用程序来编绎并且添加一个本地机器代码部分到 `wasm` 文件中。你可以使用 `wasmedge` 来运行本地检测到的 `wasm` 文件，从而获得更快的性能。

```bash
wasmedgec ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm wasmedge_quickjs.wasm
wasmedge --dir .:. wasmedge_quickjs.wasm hello.js
```

接下来，我们将讨论关于在 WasmEdger 中使用 JavaScript 的更多高级案例。
