# Bindgen and rustwasmc


The [rustwasmc](https://github.com/second-state/rustwasmc) tool is inspired by the wasm-pack project but is optimized for edge cloud and device applications. Specifically, it supports the [WasmEdge](https://github.com/WasmEdge/WasmEdge) WebAssembly runtime.

## Prerequisites

The `rustwasmc` depends on the Rust cargo toolchain to compile Rust source code to WebAssembly. You must have [Rust installed](https://www.rust-lang.org/tools/install) on your machine.

```src
$ curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
$ source $HOME/.cargo/env
$ rustup override set 1.50.0
```

## Install

The easiest way to install [rustwasmc](https://github.com/second-state/rustwasmc) is to use its installer.

```src
$ curl https://raw.githubusercontent.com/second-state/rustwasmc/master/installer/init.sh -sSf | sh
```

Alternatively, you can [install using the NPM](https://github.com/second-state/rustwasmc#install) if you'd like.

## Usage

To build [Rust functions for Node.js](/articles/getting-started-with-rust-function) applications, use the following command. See a [template application](https://github.com/second-state/wasmedge-nodejs-starter).

```src
$ rustwasmc build
```

Use the `--enable-ext` flag to compile Rust programs that use WASI extensions, such as WasmEdge's storage and [tensorflow](https://www.secondstate.io/articles/wasi-tensorflow/) APIs. The `rustwasmc` will run the compiled WebAssembly bytecode program in the `wasmedge-extensions` Node.js module instead of `wasmedge-core` in this case.

```src
$ rustwasmc build --enable-ext
```

## Support AOT

A key feature of the WasmEdge runtime is its support for Ahead-of-Time (AOT) compilers. When you run WebAssembly programs in Node.js `wasmedge-core` and `wasmedge-extensions` add-ons, you typically do not need to worry about it as the add-on handles AOT compilation transparently. However, in some cases, you do want the `rustwasmc` to compile and generate native code for the program. 

Then, use the commands below to bring your operating system up to date with the latest developer tools. The commands here are tested on Ubuntu 20.04.

```src
$ sudo apt-get update
$ sudo apt-get -y upgrade
$ sudo apt install build-essential curl wget git vim libboost-all-dev llvm-dev liblld-10-dev
```

Now, you can build the `.so` files for the AOT native target like the following.

```src
$ rustwasmc build --enable-aot
```

Enjoy coding!
