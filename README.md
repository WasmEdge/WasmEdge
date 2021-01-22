# Introduction

**SSVM** is a high performance and enterprise-ready WebAssembly (WASM) Virtual Machine for cloud, AI, and Blockchain applications. Its use cases include the following.

* A high performance and secure runtime for Rust functions in Node.js applications. [Getting started](https://www.secondstate.io/articles/getting-started-with-rust-function/) | [VSCode Codespaces](https://www.secondstate.io/articles/getting-started-rust-nodejs-vscode/) | [Tensorflow](https://www.secondstate.io/articles/artificial-intelligence/) | [Privacy computing @ Mozilla Open Labs](https://hackernoon.com/second-state-releases-scalable-privacy-service-at-mozilla-open-labs-b15u3wh7)
* A hardware-optimized runtime for ONNX AI models. [ONNC compiler for AI](https://github.com/ONNC/onnc-wasm)
* Smart contract runtime engine for leading blockchain platforms. [Polkadot/Substrate](https://github.com/second-state/substrate-ssvm-node) | [CyberMiles](https://docs.secondstate.io/devchain/getting-started/cybermiles-ewasm-testnet)

For the information on related tools and the `SSVM` ecosystem, please refer to the [SSVM ecosystem documentation](https://github.com/second-state/SSVM/blob/master/doc/ecosystem.md).

![build](https://github.com/second-state/SSVM/workflows/build/badge.svg)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/second-state/SSVM.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/second-state/SSVM/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/second-state/SSVM.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/second-state/SSVM/context:cpp)
[![codecov](https://codecov.io/gh/second-state/SSVM/branch/master/graph/badge.svg)](https://codecov.io/gh/second-state/SSVM)


# Getting Started

## Get Source Code

```bash
$ git clone git@github.com:second-state/SSVM.git
$ cd SSVM
$ git checkout 0.7.3
```

## Prepare the environment

### Use our docker image

Our docker image use `ubuntu 20.04` as the base.

```bash
$ docker pull secondstate/ssvm
```

### Or setup the environment manually

```bash
# Tools and libraries
$ sudo apt install -y \
	software-properties-common \
	cmake \
	libboost-all-dev

# And you will need to install llvm for ssvmc tool
$ sudo apt install -y \
	llvm-dev \
	liblld-10-dev

# SSVM supports both clang++ and g++ compilers
# You can choose one of them for building this project
$ sudo apt install -y gcc g++
$ sudo apt install -y clang
```

## Build SSVM

SSVM provides various tools for enabling different runtime environments for optimal performance.
After the build is finished, you can find there are several ssvm related tools:

1. `ssvm` is for general wasm runtime.
	* `ssvm` executes a `WASM` file in interpreter mode or a compiled WASM `so` file in ahead-of-time compilation mode.
	* To disable building all tools, you can set the CMake option `BUILD_TOOLS` to `OFF`.
2. `ssvmc` is for ahead-of-time `WASM` compiler.
	* `ssvmc` compiles a general `WASM` file into a `so` file.
	* To disable building the ahead-of-time compiler only, you can set the CMake option `SSVM_DISABLE_AOT_RUNTIME` to `ON`.
	* To disable building all tools, you can set the CMake option `BUILD_TOOLS` to `OFF`.
3. `ssvm-qitc` is for AI application, supporting ONNC runtime for AI model in ONNX format.
	* If you want to try `ssvm-qitc`, please refer to [ONNC-Wasm](https://github.com/ONNC/onnc-wasm) project to set up the working environment and run several examples.
	* And here is our [tutorial for ONNC-Wasm project(YouTube Video)](https://www.youtube.com/watch?v=cbiPuHMS-iQ).

```bash
# After pulling our ssvm docker image
$ docker run -it --rm \
    -v <path/to/your/ssvm/source/folder>:/root/ssvm \
    secondstate/ssvm:latest
(docker)$ cd /root/ssvm
(docker)$ mkdir -p build && cd build
(docker)$ cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON .. && make -j
```

SSVM requires `libLLVM-10` and `GLIBCXX_3.4.26` or after.
If users want to build and execute the `ssvm` runner tool without these dependencies, they can set the CMake option `SSVM_DISABLE_AOT_RUNTIME` and `STATIC_BUILD` to `ON`.

```bash
$ cmake -DCMAKE_BUILD_TYPE=Release -DSSVM_DISABLE_AOT_RUNTIME=ON -DSTATIC_BUILD=ON ..
```

## Run built-in tests

The following built-in tests are only available when the build flag `BUILD_TESTS` sets to `ON`.

Users can use these tests to verify the correctness of SSVM binaries.

```bash
$ cd <path/to/ssvm/build_folder>
$ ctest
```

## Run ssvm (SSVM with general wasm runtime)

To run SSVM with general wasm runtime, users will need to provide the following parameters:

1. (Optional) Reactor mode: use `--reactor` to enable reactor mode.
	* SSVM will execute the function which name should be given in ARG[0].
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
# cd <path/to/ssvm/build_folder>
$ cd tools/ssvm
# ./ssvm [-h|--help] [-v|--version] [--reactor] [--dir PREOPEN_DIRS ...] [--env ENVS ...] [--enable-bulk-memory] [--enable-reference-types] [--enable-simd] [--enable-all] [--allow-command COMMANDS ...] [--allow-command-all] [--] WASM_OR_SO [ARG ...]
$ ./ssvm --reactor examples/fibonacci.wasm fib 10
89
```

When wrong number of parameter given, the following error message is printed.

```bash
$ ./ssvm --reactor examples/fibonacci.wasm fib 10 10
2020-08-21 06:30:37,304 ERROR [default] execution failed: function signature mismatch, Code: 0x83
2020-08-21 06:30:37,304 ERROR [default]     Mismatched function type. Expected: params{i32} returns{i32} , Got: params{i32 , i32} returns{i32}
2020-08-21 06:30:37,304 ERROR [default]     When executing function name: "fib"
```

When calling unknown exported function, the following error message is printed.

```bash
$ ./ssvm --reactor examples/fibonacci.wasm fib2 10
2020-08-21 06:30:56,981 ERROR [default] ssvm runtime failed: wasm function not found, Code: 0x04
2020-08-21 06:30:56,981 ERROR [default]     When executing function name: "fib2"
```

### Example: Factorial

```bash
# ./ssvm [-h|--help] [-v|--version] [--reactor] [--dir PREOPEN_DIRS ...] [--env ENVS ...] [--enable-bulk-memory] [--enable-reference-types] [--enable-simd] [--enable-all] [--allow-command COMMANDS ...] [--allow-command-all] [--] WASM_OR_SO [ARG ...]
$ ./ssvm --reactor examples/factorial.wasm fac 5
120
```

# Related tools

## SSVM-EVMC

[SSVM-EVMC](https://github.com/second-state/ssvm-evmc) provides support for Ewasm runtime which is compatible with [EVMC](https://github.com/ethereum/evmc).

This project provides a shared library that can initialize and execute by the EVMC interface.

## SSVM nodejs addon

[SSVM-napi](https://github.com/second-state/SSVM-napi) provides support for accessing SSVM as a Node.js addon.

It allows Node.js applications to call WebAssembly functions written in Rust or other high-performance languages.

[Why do you want to run WebAssembly on the server-side?](https://www.secondstate.io/articles/why-webassembly-server/?utm_source=github&utm_medium=documents&utm_campaign=Github-ssvm-readme)

The SSVM addon could interact with the wasm files generated by the [ssvmup](https://www.secondstate.io/articles/ssvmup/) compiler tool.

## SSVM-TensorFlow

[SSVM-TensorFlow](https://github.com/second-state/ssvm-tensorflow) provides support for accessing with [TensorFlow C library](https://www.tensorflow.org/install/lang_c).

This project provides a tool that can execute `WASM` with TensorFlow extension compiled from [Rust ssvm_tensorflow_interface](https://crates.io/crates/ssvm_tensorflow_interface).

## DevChain

[The Second State DevChain](https://github.com/second-state/devchain) features a powerful and easy-to-use virtual machine that can quickly get you started with the smart contract and DApp development.

SSVM-evmc is integrated into our DevChain. [Click here to learn how to run an ewasm smart contract on a real blockchain.](https://docs.secondstate.io/devchain/getting-started/run-an-ewasm-smart-contract?utm_source=github&utm_medium=documents&utm_campaign=Github-ssvm-readme)

## Customized Host Functions

[Design document](https://github.com/second-state/SSVM/tree/master/doc/host_function.md) shows how to register customized host functions into SSVM and execute with wasm files.

