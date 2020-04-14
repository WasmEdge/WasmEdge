# Introduction
**SSVM** is a high performance, hardware optimal Wasm Virtual Machine for AI and Blockchain applications.

# Getting Started

## Get Source Code

```bash
$ git clone git@github.com:second-state/SSVM.git
$ cd SSVM
$ git checkout 0.5.1
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
2. `ssvm-evmc` is an Ewasm runtime which is compatible with EVMC.
3. `ssvm-qitc` is for AI application, supporting ONNC runtime for AI model in ONNX format.
4. `ssvm-proxy` is for SSVMRPC service, which allows users to deploy and execute Wasm applications via Web interface.
5. `ssvm-aot` is for general wasm runtime. AOT compilation mode.

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
$ cd test
$ cd loader
$ ./ssvmLoaderEthereumTests
$ ./ssvmLoaderFileMgrTests
$ ./ssvmLoaderWagonTests
$ cd ../ast/load
$ ./ssvmASTLoadTests
$ cd ../../evmc
$ ./ssvmEVMCTests
$ cd ../proxy
$ ./ssvmProxyTests
$ cd ../expected
$ ./expectedTests
```

## ssvm-evmc (SSVM with Ewasm runtime with EVMC integration)

SSVM-EVMC is a Ewasm runtime which is compatible with [EVMC](https://github.com/ethereum/evmc).
Please notice that SSVM-EVMC is not a standalone tool but a shared library which can initialize and execute by EVMC interface.
The built library will be placed at `<your/build/folder>/tools/ssvm-evmc/libssvmEVMC.so` on Linux or `<your/build/folder>/tools/ssvm-evmc/libssvmEVMC.dylib` on MacOS.

## Run ssvm-proxy (SSVM with general wasm runtime and JSON input/output)

To run SSVM-PROXY, users will need to provide the following parameters:

1. Wasm file(`/path/to/wasm/file`)
2. Input JSON file(`/path/to/input/json/file`)
3. (Optional) Output JSON file(`/path/to/output/json/file`).

Examples of input and output JSON files are in `doc/ssvm-proxy/design_document.md`.

### Example wasm file

`tools/ssvm-proxy/wasm/calc.wasm` is a wasm providing m+, m-, and mrc function on calculator.
1. Global variable 64-bit integer to store.
2. `int64_t mplus(int64_t)` wasm function to add number to store and return.
3. `int64_t mminus(int64_t)` wasm function to sub number to store and return.
4. `int64_t mrc()` wasm function to return the stored number and clear.

### Example JSON file

`tools/ssvm-proxy/inputJSON/` contains input examples for ssvm-proxy.
1. `inputJSON/input-mplus-sample.json` is a JSON file call `mplus` function.
  * The `VMSnapshot` contains `9` stored in calculator.
  * Arguments of `255` is set.
  * `outputJSON/output-mplus-sample.json` is the result.
  * The return value will be `264` (i.e. `0x0000000000000108` in 64-bit number).
2. `inputJSON/input-mminus-sample.json` is a JSON file call `mminus` function.
  * The `VMSnapshot` contains `9` stored in calculator.
  * Arguments of `160` is set.
  * `outputJSON/output-mplus-sample.json` is the result.
  * The return value will be `-151` (i.e. `0xffffffffffffff69` in 64-bit number).
3. `inputJSON/input-mrc-sample.json` is a JSON file call `mrc` function.
  * The `VMSnapshot` contains `238` stored in calculator.
  * `outputJSON/output-mrc-sample.json` is the result.
  * The return value will be `238` (i.e. `0x00000000000000ee` in 64-bit number).

### Example: Calculator M+

```bash
# cd <path/to/ssvm/build_folder>
$ cd tools/ssvm-proxy
# ./ssvm-proxy --input_file=input.json --output_file=output.json --wasm_file=wasm_file.wasm
$ ./ssvm-proxy --input_file=inputJSON/input-mplus-sample.json --output_file=outputJSON/output-mplus-sample.json --wasm_file=wasm/calc.wasm
2020-04-01 12:26:20,417 INFO [default] Start running...
2020-04-01 12:26:20,418 INFO [default] Execution succeeded.
2020-04-01 12:26:20,418 INFO [default] Done.
2020-04-01 12:26:20,418 INFO [default]
 =================  Statistics  =================
 Total execution time: 47 us
 Wasm instructions execution time: 47 us
 Host functions execution time: 0 us
 Executed wasm instructions count: 9
 Gas costs: 9
 Instructions per second: 191489

# The output json should be logically the same with SSVM/tools/ssvm-proxy/outputJSON/output-mplus-sample.json
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
2020-04-01 12:31:56,042 INFO [default] Start running...
2020-04-01 12:31:56,042 INFO [default] Execution succeeded.
2020-04-01 12:31:56,042 INFO [default] Done.
2020-04-01 12:31:56,042 INFO [default]
 =================  Statistics  =================
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
2020-04-01 12:32:33,153 INFO [default] Start running...
2020-04-01 12:32:33,153 INFO [default] Execution succeeded.
2020-04-01 12:32:33,153 INFO [default] Done.
2020-04-01 12:32:33,153 INFO [default]
 =================  Statistics  =================
 Total execution time: 49 us
 Wasm instructions execution time: 49 us
 Host functions execution time: 0 us
 Executed wasm instructions count: 55
 Gas costs: 61
 Instructions per second: 1122448

 Return value: 120
```

# Related tools

## SSVM nodejs addon

[SSVM-napi](https://github.com/second-state/SSVM-napi) provides support for accessing SSVM as a Node.js addon.

It allows Node.js applications to call WebAssembly functions written in Rust or other high performance languages.

[Why do you want to run WebAssembly on the server-side?](https://cloud.secondstate.io/server-side-webassembly/why?utm_source=github&utm_medium=documents&utm_campaign=Github-ssvm-readme)

The SSVM addon could interact with the wasm files generated by the [ssvmup](https://github.com/second-state/ssvmup) compiler tool.

## DevChain

[The Second State DevChain](https://github.com/second-state/devchain) features a powerful and easy-to-use virtual machine that can quickly get you started with smart contract and DApp development.

SSVM-evmc is integrated into our DevChain. [Click here to learn how to run an ewasm smart contrat on a real blockchain.](https://docs.secondstate.io/devchain/getting-started/run-an-ewasm-smart-contract?utm_source=github&utm_medium=documents&utm_campaign=Github-ssvm-readme)
