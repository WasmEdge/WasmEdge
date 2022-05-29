# Quick start with JavaScript on WasmEdge

First, let's build a WebAssembly-based JavaScript interpreter program for WasmEdge. It is based on [QuickJS](https://bellard.org/quickjs/) with WasmEdge extensions, such as [network sockets](https://github.com/second-state/wasmedge_wasi_socket) and [Tensorflow inference](https://www.secondstate.io/articles/wasi-tensorflow/), incorporated into the interpreter as JavaScript APIs. You will need to [install Rust](https://www.rust-lang.org/tools/install) to build the interpreter.

> If you just want to use the interpreter to run JavaScript programs, you can skip this section. Make sure you have installed [Rust](https://www.rust-lang.org/tools/install) and [WasmEdge](../../start/install.md).

Fork or clone [the wasmedge-quickjs Github repository](https://github.com/second-state/wasmedge-quickjs) to get started.

```bash
git clone https://github.com/second-state/wasmedge-quickjs
```

Following the instructions from that repo, you will be able to build a JavaScript interpreter for WasmEdge.

```bash
# Install GCC
sudo apt update
sudo apt install build-essential

# Install wasm32-wasi target for Rust
rustup target add wasm32-wasi

# Build the QuickJS JavaScript interpreter
cargo build --target wasm32-wasi --release
```

The WebAssembly-based JavaScript interpreter program is located in the build target directory. You can now try a simple "hello world" JavaScript program ([example_js/hello.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/hello.js)), which prints out the command line arguments to the console.

```javascript
import * as os from 'os';
import * as std from 'std';

args = args.slice(1);
print('Hello', ...args);
setTimeout(() => {
  print('timeout 2s');
}, 2000);
```

Run the `hello.js` file in WasmEdge’s QuickJS runtime as follows.

```bash
$ cd example_js
$ wasmedge --dir .:. ../target/wasm32-wasi/release/wasmedge_quickjs.wasm hello.js WasmEdge Runtime
Hello WasmEdge Runtime
```

> Note: the `--dir .:.` on the command line is to give `wasmedge` permission to read the local directory in the file system for the `hello.js` file. We will use  `--dir .:.` in the following sections.

## Make it faster

WasmEdge provides a `wasmedgec` utility to compile and add a native machine code section to the `wasm` file. You can use `wasmedge` to run the natively instrumented `wasm` file to get much faster performance.

```bash
wasmedgec ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm wasmedge_quickjs.wasm
wasmedge --dir .:. wasmedge_quickjs.wasm hello.js
```

Next, we will discuss more advanced use case for JavaScript in WasmEdge.
