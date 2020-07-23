# Introduction

**SSVM** is a high performance and enterprise-ready WebAssembly (WASM) Virtual Machine for cloud, AI, and Blockchain applications. Its use cases include the following.

* A high performance and secure runtime for Rust functions in Node.js applications. [Getting started](https://www.secondstate.io/articles/getting-started-with-rust-function/) | [VSCode Codespaces](https://www.secondstate.io/articles/getting-started-rust-nodejs-vscode/) | [Tensorflow](https://www.secondstate.io/articles/artificial-intelligence/) | [Privacy computing @ Mozilla Open Labs](https://hackernoon.com/second-state-releases-scalable-privacy-service-at-mozilla-open-labs-b15u3wh7)
* A hardware-optimized runtime for ONNX AI models. [ONNC compiler for AI](https://github.com/ONNC/onnc-wasm)
* Smart contract runtime engine for leading blockchain platforms. [Polkadot/Substrate](https://github.com/second-state/substrate-ssvm-node) | [CyberMiles](https://docs.secondstate.io/devchain/getting-started/cybermiles-ewasm-testnet)

![build](https://github.com/second-state/SSVM/workflows/build/badge.svg)


# Getting Started

## Get Source Code

```bash
$ git clone git@github.com:second-state/SSVM.git
$ cd SSVM
$ git checkout 0.6.3
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

# And you will need to install llvm for ssvm-aot tools
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

1. `ssvm` is for general wasm runtime. Interpreter mode.
2. `ssvm-qitc` is for AI application, supporting ONNC runtime for AI model in ONNX format.
	* If you want to try `ssvm-qitc`, please refer to [ONNC-Wasm](https://github.com/ONNC/onnc-wasm) project to set up the working environment and run several examples.
	* And here is our [tutorial for ONNC-Wasm project(YouTube Video)](https://www.youtube.com/watch?v=cbiPuHMS-iQ).
3. `ssvm-aot` is for general wasm runtime. AOT compilation mode.
	* `ssvmc` compiles a general wasm runtime to so file.
	* `ssvmr` execute a general wasm runtime or so file in WASI environment.
	* To disable building the ahead of time compilation runtime, you can set the CMake option `SSVM_DISABLE_AOT_RUNTIME` to `OFF`.

```bash
# After pulling our ssvm docker image
$ docker run -it --rm \
    -v <path/to/your/ssvm/source/folder>:/root/ssvm \
    secondstate/ssvm:latest
(docker)$ cd /root/ssvm
(docker)$ mkdir -p build && cd build
(docker)$ cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON .. && make -j
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

1. Wasm file(`/path/to/wasm/file`)
2. (Optional) Entry function name, the default value is `main`
3. (Optional) Argument List, can be one or more arguments.

### Example: Fibonacci

```bash
# cd <path/to/ssvm/build_folder>
$ cd tools/ssvm
# ./ssvm wasm_file.wasm [exported_func_name] [args...]
$ ./ssvm examples/fibonacci.wasm fib 10
2020-07-22 10:36:27,721 DEBUG [default] [hydai@unknown-host] [SSVM::Expect<void> SSVM::Interpreter::Interpreter::runFunction(SSVM::Runtime::StoreManager&, const SSVM::Runtime::Instance::FunctionInstance&, SSVM::Span<const SSVM::Support::Variant<unsigned int, long unsigned int, float, double> >)] [/home/hydai/workspace/SSVM/lib/interpreter/engine/engine.cpp:104]  Execution succeeded.
2020-07-22 10:36:27,721 DEBUG [default] [hydai@unknown-host] [SSVM::Expect<void> SSVM::Interpreter::Interpreter::runFunction(SSVM::Runtime::StoreManager&, const SSVM::Runtime::Instance::FunctionInstance&, SSVM::Span<const SSVM::Support::Variant<unsigned int, long unsigned int, float, double> >)] [/home/hydai/workspace/SSVM/lib/interpreter/engine/engine.cpp:120]
 ====================  Statistics  ====================
 Total execution time: 101 us
 Wasm instructions execution time: 101 us
 Host functions execution time: 0 us
 Executed wasm instructions count: 1766
 Gas costs: 1766
 Instructions per second: 17485148

 Return value: 89
```

### Example: Factorial

```bash
# ./ssvm wasm_file.wasm [exported_func_name] [args...]
$ ./ssvm examples/factorial.wasm fac 5
2020-07-22 10:37:36,624 DEBUG [default] [hydai@unknown-host] [SSVM::Expect<void> SSVM::Interpreter::Interpreter::runFunction(SSVM::Runtime::StoreManager&, const SSVM::Runtime::Instance::FunctionInstance&, SSVM::Span<const SSVM::Support::Variant<unsigned int, long unsigned int, float, double> >)] [/home/hydai/workspace/SSVM/lib/interpreter/engine/engine.cpp:104]  Execution succeeded.
2020-07-22 10:37:36,624 DEBUG [default] [hydai@unknown-host] [SSVM::Expect<void> SSVM::Interpreter::Interpreter::runFunction(SSVM::Runtime::StoreManager&, const SSVM::Runtime::Instance::FunctionInstance&, SSVM::Span<const SSVM::Support::Variant<unsigned int, long unsigned int, float, double> >)] [/home/hydai/workspace/SSVM/lib/interpreter/engine/engine.cpp:120]
 ====================  Statistics  ====================
 Total execution time: 107 us
 Wasm instructions execution time: 107 us
 Host functions execution time: 0 us
 Executed wasm instructions count: 55
 Gas costs: 55
 Instructions per second: 514018

 Return value: 120
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

## DevChain

[The Second State DevChain](https://github.com/second-state/devchain) features a powerful and easy-to-use virtual machine that can quickly get you started with the smart contract and DApp development.

SSVM-evmc is integrated into our DevChain. [Click here to learn how to run an ewasm smart contract on a real blockchain.](https://docs.secondstate.io/devchain/getting-started/run-an-ewasm-smart-contract?utm_source=github&utm_medium=documents&utm_campaign=Github-ssvm-readme)

## Customized Host Functions

[Design document](https://github.com/second-state/SSVM/tree/master/doc/host_function.md) shows how to register customized host functions into SSVM and execute with wasm files.

