# Introduction

**WasmEdge** (formerly SSVM) is a high performance and enterprise-ready WebAssembly (WASM) Virtual Machine for cloud, AI, and Blockchain applications. Its use cases include the following.

* A high performance and secure runtime for Rust functions in Node.js applications. [Getting started](https://www.secondstate.io/articles/getting-started-with-rust-function/) | [VSCode Codespaces](https://www.secondstate.io/articles/getting-started-rust-nodejs-vscode/) | [Tensorflow](https://www.secondstate.io/articles/artificial-intelligence/) | [Privacy computing @ Mozilla Open Labs](https://hackernoon.com/second-state-releases-scalable-privacy-service-at-mozilla-open-labs-b15u3wh7)
* A hardware-optimized runtime for ONNX AI models. [ONNC compiler for AI](https://github.com/ONNC/onnc-wasm)
* Smart contract runtime engine for leading blockchain platforms. [Polkadot/Substrate](https://github.com/second-state/substrate-ssvm-node) | [CyberMiles](https://docs.secondstate.io/devchain/getting-started/cybermiles-ewasm-testnet)

For the information on related tools and the `WasmEdge` ecosystem, please refer to the [WasmEdge ecosystem documentation](https://github.com/WasmEdge/WasmEdge/blob/master/doc/ecosystem.md).

![build](https://github.com/WasmEdge/WasmEdge/workflows/build/badge.svg)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/WasmEdge/WasmEdge.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/WasmEdge/WasmEdge/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/WasmEdge/WasmEdge.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/WasmEdge/WasmEdge/context:cpp)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)


# Getting Started

## Get Source Code

```bash
$ git clone git@github.com:WasmEdge/WasmEdge.git
$ cd WasmEdge
$ git checkout 0.7.3
```

## Prepare the environment

### Use our docker image

Our docker image use `ubuntu 20.04` as the base.

```bash
$ docker pull wasmedge/wasmedge
```

### Or setup the environment manually

```bash
# Tools and libraries
$ sudo apt install -y \
	software-properties-common \
	cmake \
	libboost-all-dev

# And you will need to install llvm for wasmedgec tool
$ sudo apt install -y \
	llvm-dev \
	liblld-10-dev

# WasmEdge supports both clang++ and g++ compilers
# You can choose one of them for building this project
$ sudo apt install -y gcc g++
$ sudo apt install -y clang
```

### Support for legacy operating systems

Our development environment requires `libLLVM-10` and `>=GLIBCXX_3.4.26`.

If users are using the older operating system than Ubuntu 20.04, please use our special docker image to build WasmEdge.
If you are looking for the pre-built binaries for the older operatoring system, we also provide several pre-built binaries based on manylinux\* distribution.



| Portable Linux Built Distributions Tags | Base Image  | Provided Requirements                                                 | Docker Image                            |
| ---                                     | ---         | ---                                                                   | ---                                     |
| `manylinux1`                            | CentOS 5.11 | GLIBC <= 2.5<br>CXXABI <= 3.4.8<br>GLIBCXX <= 3.4.9<br>GCC <= 4.2.0   | wasmedge/wasmedge:manylinux1\_x86\_64    |
| `manylinux2010`                         | CentOS 6    | GLIBC <= 2.12<br>CXXABI <= 1.3.3<br>GLIBCXX <= 3.4.13<br>GCC <= 4.5.0 | wasmedge/wasmedge:manylinux2010\_x86\_64 |
| `manylinux2014`                         | CentOS 7    | GLIBC <= 2.17<br>CXXABI <= 1.3.7<br>GLIBCXX <= 3.4.19<br>GCC <= 4.8.0 | wasmedge/wasmedge:manylinux2014\_x86\_64 |

### If you don't want to build Ahead-of-Time runtime/compiler

If users don't need Ahead-of-Time runtime/compiler support, they can set the CMake option `WASMEDGE_DISABLE_AOT_RUNTIME` to `ON`.

```bash
$ cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_DISABLE_AOT_RUNTIME=ON ..
```

## Build WasmEdge

WasmEdge provides various tools for enabling different runtime environments for optimal performance.
After the build is finished, you can find there are several wasmedge related tools:

1. `wasmedge` is for general wasm runtime.
	* `wasmedge` executes a `WASM` file in interpreter mode or a compiled WASM `so` file in ahead-of-time compilation mode.
	* To disable building all tools, you can set the CMake option `BUILD_TOOLS` to `OFF`.
2. `wasmedgec` is for ahead-of-time `WASM` compiler.
	* `wasmedgec` compiles a general `WASM` file into a `so` file.
	* To disable building the ahead-of-time compiler only, you can set the CMake option `WASMEDGE_DISABLE_AOT_RUNTIME` to `ON`.
	* To disable building all tools, you can set the CMake option `BUILD_TOOLS` to `OFF`.
3. `ssvm-qitc` is for AI application, supporting ONNC runtime for AI model in ONNX format.
	* If you want to try `ssvm-qitc`, please refer to [ONNC-Wasm](https://github.com/ONNC/onnc-wasm) project to set up the working environment and run several examples.
	* And here is our [tutorial for ONNC-Wasm project(YouTube Video)](https://www.youtube.com/watch?v=cbiPuHMS-iQ).

```bash
# After pulling our wasmedge docker image
$ docker run -it --rm \
    -v <path/to/your/wasmedge/source/folder>:/root/wasmedge \
    wasmedge/wasmedge:latest
(docker)$ cd /root/wasmedge
(docker)$ mkdir -p build && cd build
(docker)$ cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON .. && make -j
```

## Run built-in tests

The following built-in tests are only available when the build flag `BUILD_TESTS` sets to `ON`.

Users can use these tests to verify the correctness of WasmEdge binaries.

```bash
$ cd <path/to/wasmedge/build_folder>
$ ctest
```

## Run wasmedge (WasmEdge with general wasm runtime)

To run WasmEdge with general wasm runtime, users will need to provide the following parameters:

1. (Optional) Reactor mode: use `--reactor` to enable reactor mode.
	* WasmEdge will execute the function which name should be given in ARG[0].
	* If there's exported function which names `_initialize`, the function will be executed with the empty parameter at first.
2. (Optional) Binding directories into WASI virtual filesystem.
	* Each directory can be specified as `--dir host_path:guest_path`.
3. (Optional) Environ variables.
	* Each variable can be specified as `--env NAME=VALUE`.
4. Wasm file(`/path/to/wasm/file`).
5. (Optional) Arguments.
	* In reactor mode, the first argument will be the function name, and the arguments after ARG[0] will be parameters of wasm function ARG[0].
	* In command mode, the arguments will be parameters of function `_start`.

### Example: Fibonacci

```bash
# cd <path/to/wasmedge/build_folder>
$ cd tools/wasmedge
# ./wasmedge [-h|--help] [-v|--version] [--reactor] [--dir PREOPEN_DIRS ...] [--env ENVS ...] [--enable-bulk-memory] [--enable-reference-types] [--enable-simd] [--enable-all] [--allow-command COMMANDS ...] [--allow-command-all] [--] WASM_OR_SO [ARG ...]
$ ./wasmedge --reactor examples/fibonacci.wasm fib 10
89
```

When wrong number of parameter given, the following error message is printed.

```bash
$ ./wasmedge --reactor examples/fibonacci.wasm fib 10 10
2020-08-21 06:30:37,304 ERROR [default] execution failed: function signature mismatch, Code: 0x83
2020-08-21 06:30:37,304 ERROR [default]     Mismatched function type. Expected: params{i32} returns{i32} , Got: params{i32 , i32} returns{i32}
2020-08-21 06:30:37,304 ERROR [default]     When executing function name: "fib"
```

When calling unknown exported function, the following error message is printed.

```bash
$ ./wasmedge --reactor examples/fibonacci.wasm fib2 10
2020-08-21 06:30:56,981 ERROR [default] wasmedge runtime failed: wasm function not found, Code: 0x04
2020-08-21 06:30:56,981 ERROR [default]     When executing function name: "fib2"
```

### Example: Factorial

```bash
# ./wasmedge [-h|--help] [-v|--version] [--reactor] [--dir PREOPEN_DIRS ...] [--env ENVS ...] [--enable-bulk-memory] [--enable-reference-types] [--enable-simd] [--enable-all] [--allow-command COMMANDS ...] [--allow-command-all] [--] WASM_OR_SO [ARG ...]
$ ./wasmedge --reactor examples/factorial.wasm fac 5
120
```

# Related tools

## SSVM-EVMC

[SSVM-EVMC](https://github.com/second-state/ssvm-evmc) provides support for Ewasm runtime which is compatible with [EVMC](https://github.com/ethereum/evmc).

This project provides a shared library that can initialize and execute by the EVMC interface.

## SSVM nodejs addon

[SSVM-napi](https://github.com/second-state/ssvm-napi) provides support for accessing WasmEdge as a Node.js addon.

It allows Node.js applications to call WebAssembly functions written in Rust or other high-performance languages.

[Why do you want to run WebAssembly on the server-side?](https://www.secondstate.io/articles/why-webassembly-server/?utm_source=github&utm_medium=documents&utm_campaign=Github-ssvm-readme)

The WasmEdge addon could interact with the wasm files generated by the [ssvmup](https://www.secondstate.io/articles/ssvmup/) compiler tool.

## SSVM-TensorFlow

[SSVM-TensorFlow](https://github.com/second-state/ssvm-tensorflow) provides support for accessing with [TensorFlow C library](https://www.tensorflow.org/install/lang_c).

This project provides a tool that can execute `WASM` with TensorFlow extension compiled from [Rust ssvm_tensorflow_interface](https://crates.io/crates/ssvm_tensorflow_interface).

## DevChain

[The Second State DevChain](https://github.com/second-state/devchain) features a powerful and easy-to-use virtual machine that can quickly get you started with the smart contract and DApp development.

SSVM-evmc is integrated into our DevChain. [Click here to learn how to run an ewasm smart contract on a real blockchain.](https://docs.secondstate.io/devchain/getting-started/run-an-ewasm-smart-contract?utm_source=github&utm_medium=documents&utm_campaign=Github-ssvm-readme)

## Customized Host Functions

[Design document](https://github.com/WasmEdge/WasmEdge/tree/master/doc/host_function.md) shows how to register customized host functions into WasmEdge and execute with wasm files.

