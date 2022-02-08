# Bindgen and rustwasmc

**NOTE: This has been deprecated. We recommend using [wasmedge-bindgen](https://github.com/second-state/wasmedge-bindgen) when calling WebAssembly functions from another application (e.g., from [Go](https://wasmedge.org/book/en/embed/go/function.html)).**

---

The [rustwasmc](https://github.com/second-state/rustwasmc) tool is inspired by the wasm-pack project but is optimized for edge cloud and device applications. Specifically, it supports the [WasmEdge](https://github.com/WasmEdge/WasmEdge) WebAssembly runtime.

One of the key features of `rustwasmc` over the standard `wasm32-wasi` compiler target is that `rustwasmc` processes compiled Rust functions using the `wasm-bindgen` tool.
By default, WebAssembly functions only support a few simple data types as input call arguments. Tools like `wasm-bindgen` turn WebAssembly function arguments into memory pointers, and allow host applications to pass complex arguments, such as strings and arrays, to WebAssembly functions.
WasmEdge's [Node.js SDK](../../embed/node.md) and [Go SDK](../../embed/go.md) both support `wasm-bindgen`, allowing JavaScript and Go programs to call WebAssembly function with complex call arguments.

> At this time, we require Rust compiler version 1.50 or less in order for WebAssembly functions to work with `wasm-bindgen` and `rustwasmc`. We will [catch up to the latest Rust compiler](https://github.com/WasmEdge/WasmEdge/issues/264) version once the Interface Types spec is finalized and supported.

## Prerequisites

The `rustwasmc` depends on the Rust cargo toolchain to compile Rust source code to WebAssembly. You must have [Rust installed](https://www.rust-lang.org/tools/install) on your machine.

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env
rustup override set 1.50.0
```

## Install

The easiest way to install [rustwasmc](https://github.com/second-state/rustwasmc) is to use its installer.

```bash
curl https://raw.githubusercontent.com/second-state/rustwasmc/master/installer/init.sh -sSf | sh
```

Alternatively, you can [install using the NPM](https://github.com/second-state/rustwasmc#install) if you'd like.

## Usage

To build [Rust functions for Node.js](../../embed/node.md) applications, use the following command. See a [template application](https://github.com/second-state/wasmedge-nodejs-starter).

```bash
rustwasmc build
```

Use the `--enable-ext` flag to compile Rust programs that use WASI extensions, such as WasmEdge's storage and [Tensorflow](tensorflow.md) APIs. The `rustwasmc` will generates the compiled WebAssembly bytecode program for the `wasmedge-extensions` Node.js module instead of the `wasmedge-core` module in this case.

```bash
rustwasmc build --enable-ext
```

## Support AOT

A key feature of the WasmEdge runtime is its support for Ahead-of-Time (AOT) compilers. When you run WebAssembly programs in Node.js `wasmedge-core` and `wasmedge-extensions` add-ons, you typically do not need to worry about it as the add-on handles AOT compilation transparently. However, in some cases, you do want the `rustwasmc` to compile and generate native code for the program.
Then, use the commands below to bring your operating system up to date with the latest developer tools. The commands here are tested on Ubuntu 20.04.

```bash
sudo apt-get update
sudo apt-get -y upgrade
sudo apt install build-essential curl wget git vim libboost-all-dev llvm-dev liblld-10-dev
```

Now, you can build the `.so` files for the AOT native target like the following.

```bash
rustwasmc build --enable-aot
```

Enjoy coding!
