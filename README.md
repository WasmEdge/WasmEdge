# Introduction

**SSVM** is a high performance and enterprise-ready WebAssembly (WASM) Virtual Machine for cloud, AI and Blockchain applications. Its use cases include the following.

* A high performance and secure runtime for Rust functions in Node.js applications. [Getting started](https://cloud.secondstate.io/server-side-webassembly/getting-started) | [VSCode Codespaces](https://github.com/second-state/ssvm-nodejs-starter) | [Tensorflow](https://github.com/second-state/rust-wasm-ai-demo) | [Privacy computing @ Mozilla Open Labs](https://hackernoon.com/second-state-releases-scalable-privacy-service-at-mozilla-open-labs-b15u3wh7)
* A hardware-optimized runtime for ONNX AI models. [ONNC compiler for AI](https://github.com/ONNC/onnc-wasm)
* Smart contract runtime engine for leading blockchain platforms. [Polkadot/Substrate](https://github.com/second-state/substrate-ssvm-node) | [CyberMiles](https://docs.secondstate.io/devchain/getting-started/cybermiles-ewasm-testnet)

![build](https://github.com/second-state/SSVM/workflows/build/badge.svg)


# Getting Started

## Get Source Code

```bash
$ git clone git@github.com:second-state/SSVM.git
$ cd SSVM
$ git checkout 0.6.1
```

## Prepare environment

### Use our docker image

Our docker image use `ubuntu 18.04` as base.

```bash
$ docker pull secondstate/ssvm
```

### Or setup the environment manually

```bash
$ sudo apt install -y \
	cmake \
	gcc-8 \
	g++-8
	libboost-all-dev
# And you will need to install llvm-9 for ssvm-aot tools
$ wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
$ sudo apt update && apt install -y \
	libllvm9 \
	llvm-9 \
	llvm-9-dev \
	llvm-9-runtime \
	libclang-common-9-dev # for yaml-bench

```

## Build SSVM

SSVM provides various tools to enabling different runtime environment for optimal performance.
After the build is finished, you can find there are several ssvm related tools:

1. `ssvm` is for general wasm runtime. Interpreter mode.
2. `ssvm-qitc` is for AI application, supporting ONNC runtime for AI model in ONNX format.
	* If you want to try `ssvm-qitc`, please refer to [ONNC-Wasm](https://github.com/ONNC/onnc-wasm) project to setup the working environment and run several examples.
	* And here is our [tutorial for ONNC-Wasm project(YouTube Video)](https://www.youtube.com/watch?v=cbiPuHMS-iQ).
3. `ssvm-aot` is for general wasm runtime. AOT compilation mode.
	* `ssvmc` compiles a general wasm runtime to so file.
	* `ssvmr` execute a general wasm runtime or so file in WASI environment.
	* To disable building the ahead of time compilation runtime, you can set the cmake option `SSVM_DISABLE_AOT_RUNTIME` to `OFF`.

```bash
# After pulling our ssvm docker image
$ docker run -it --rm \
    -v <path/to/your/ssvm/source/folder>:/root/ssvm \
    secondstate/ssvm:latest
(docker)$ cd /root/ssvm
(docker)$ mkdir -p build && cd build
(docker)$ cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON .. && make
```

## Run built-in tests

The following built-in tests are only avaliable when the build flag `BUILD_TESTS` sets to `ON`.

Users can use these tests to verify the correctness of SSVM binaries.

```bash
$ cd <path/to/ssvm/build_folder>
$ ctest
```

## Run ssvm (SSVM with general wasm runtime)

To run SSVM with general wasm runtime, users will need to provide the following parameters:

1. Wasm file(`/path/to/wasm/file`)
2. (Optional) Entry function name, default value is `main`
3. (Optional) Argument List, can be one or more arguments.

### Example: Fibonacci

```bash
# cd <path/to/ssvm/build_folder>
$ cd tools/ssvm
# ./ssvm wasm_file.wasm [exported_func_name] [args...]
$ ./ssvm examples/fibonacci.wasm fib 10
2020-06-23 21:53:56,703 DEBUG [default] [ubuntu@unknown-host] [Expect<void> SSVM::Interpreter::Interpreter::runFunction(Runtime::StoreManager &, const Runtime::Instance::FunctionInstance &, Span<const SSVM::ValVariant>)] [/root/ssvm/lib/interpreter/engine/engine.cpp:100]  Execution succeeded.
2020-06-23 21:53:56,703 DEBUG [default] [ubuntu@unknown-host] [Expect<void> SSVM::Interpreter::Interpreter::runFunction(Runtime::StoreManager &, const Runtime::Instance::FunctionInstance &, Span<const SSVM::ValVariant>)] [/root/ssvm/lib/interpreter/engine/engine.cpp:111]
 ====================  Statistics  ====================
 Total execution time: 52 us
 Wasm instructions execution time: 52 us
 Host functions execution time: 0 us
 Executed wasm instructions count: 1766
 Gas costs: 1855
 Instructions per second: 33961538

 Return value: 89
```

### Example: Factorial

```bash
# ./ssvm wasm_file.wasm [exported_func_name] [args...]
$ ./ssvm examples/factorial.wasm fac 5
2020-06-23 21:56:14,706 DEBUG [default] [ubuntu@unknown-host] [Expect<void> SSVM::Interpreter::Interpreter::runFunction(Runtime::StoreManager &, const Runtime::Instance::FunctionInstance &, Span<const SSVM::ValVariant>)] [/root/ssvm/lib/interpreter/engine/engine.cpp:100]  Execution succeeded.
2020-06-23 21:56:14,707 DEBUG [default] [ubuntu@unknown-host] [Expect<void> SSVM::Interpreter::Interpreter::runFunction(Runtime::StoreManager &, const Runtime::Instance::FunctionInstance &, Span<const SSVM::ValVariant>)] [/root/ssvm/lib/interpreter/engine/engine.cpp:111]
 ====================  Statistics  ====================
 Total execution time: 49 us
 Wasm instructions execution time: 49 us
 Host functions execution time: 0 us
 Executed wasm instructions count: 55
 Gas costs: 61
 Instructions per second: 1122448

 Return value: 120
```

# Related tools

## SSVM-EVMC

[SSVM-EVMC](https://github.com/second-state/ssvm-evmc) provides support for Ewasm runtime which is compatible with [EVMC](https://github.com/ethereum/evmc).

This project provides a shared library that can initialize and execute by the EVMC interface.

## SSVM nodejs addon

[SSVM-napi](https://github.com/second-state/SSVM-napi) provides support for accessing SSVM as a Node.js addon.

It allows Node.js applications to call WebAssembly functions written in Rust or other high performance languages.

[Why do you want to run WebAssembly on the server-side?](https://cloud.secondstate.io/server-side-webassembly/why?utm_source=github&utm_medium=documents&utm_campaign=Github-ssvm-readme)

The SSVM addon could interact with the wasm files generated by the [ssvmup](https://github.com/second-state/ssvmup) compiler tool.

## DevChain

[The Second State DevChain](https://github.com/second-state/devchain) features a powerful and easy-to-use virtual machine that can quickly get you started with smart contract and DApp development.

SSVM-evmc is integrated into our DevChain. [Click here to learn how to run an ewasm smart contrat on a real blockchain.](https://docs.secondstate.io/devchain/getting-started/run-an-ewasm-smart-contract?utm_source=github&utm_medium=documents&utm_campaign=Github-ssvm-readme)
