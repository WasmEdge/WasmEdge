# Introduction
**SSVM** is a high performance, hardware optimal Wasm Virtual Machine for AI and Blockchain applications.


# Getting Started

## Get Source Code

```bash
$ git clone git@github.com:second-state/SSVM.git
$ cd SSVM
$ git checkout 0.2.0
```

## Prepare environment

### Use our docker image

Our docker image use `ubuntu 18.04` as base.

```bash
$ docker pull hydai/ssvm-dev:0.2.0
```

### Or setup the environment manually

```bash
$ sudo apt install -y \
	cmake \
	g++ \
	libboost-all-dev
```

## Build SSVM

SSVM provides various tools to enabling different runtime environment for optimal performance.
After the build is finished, you can find there are two ssvm binaries:

1. `ssvm` is for general wasm runtime.
2. `ssvm-evm` is for Ewasm runtime.
3. `ssvm-qitc` is for AI application, supporting ONNC runtime for AI model in ONNX format.

```bash
# After pulling our ssvm-dev docker image
$ docker run -it --rm \
    -v <path/to/your/ssvm/source/folder>:/root/ssvm \
    hydai/ssvm-dev:0.2.0
(docker)$ cd /root/ssvm
(docker)$ mkdir -p build && cd build
(docker)$ cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON .. && make
```

## Run built-in tests

The following built-in tests are only avaliable when the build flag `BUILD_TESTS` sets to `ON`.

Users can use these tests to verify the correctness of SSVM binaries.

```bash
$ cd <path/to/ssvm/build_folder>
$ cd test/loader
$ ./ssvmLoaderEthereumTests
$ ./ssvmLoaderFileMgrTests
$ ./ssvmLoaderWagonTests
$ cd ../ast/load
$ ./ssvmASTLoadTests
$ cd ../../evm
$ ./ssvmEVMTests
$ cd ../proxy
$ ./ssvmProxyTests
```

## Run ssvm-evm (SSVM with Ewasm runtime)

To run SSVM with Ewasm bytecode, you will need to provide a valid Ewasm bytecode and the calldata.
Currently, SSVM doesn’t support ABI encoding for Ethereum, so users need to compose the calldata by theirselves.

SSVM-EVM will take 3 parameters:

1. Ewasm bytecode file (`/path/to/your/ewasm/file`)
2. Call data in hex string format (`4e6ec2..000054`)
3. Gas limit (`100000`)

### Example: ERC20 token contract

In this example, we create an ERC20 token contract and compile it into Ewasm bytecode by SecondState [SOLL](https://github.com/second-state/soll) compiler.
You can find this Ewasm file(`ethereum/erc20.wasm`) in the same folder of ssvm-evm.

#### With enough gas limit (Expect execution succeeded)

```bash
# cd <path/to/ssvm/build_folder>
$ cd tools/ssvm-evm
# Usage: ./ssvm-evm wasm_file.wasm call_data_in_string_format gas_limit
$ ./ssvm-evm ethereum/erc20.wasm 4e6ec24700000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000064 100000
# Expect output:
 Info: Start running...
 Info: Worker execution succeeded.
 =================  Statistics  =================
 Total execution time: 243 us
 Wasm instructions execution time: 190 us
 Host functions execution time: 53 us
 Executed wasm instructions count: 1041
 Gas costs: 71308
 Instructions per second: 5478947
    --- result storage:
         0000000000000000000000000000000000000000000000000000000000000000 0000000000000000000000000000000000000000000000000000000000000064
         f5b24dcea0e9381721a8c72784d30cfe64c11b4591226269f839d095b3e9cf10 0000000000000000000000000000000000000000000000000000000000000064
    --- return data:
```

#### Without enough gas limit (Expect execution reverted)

```bash
# cd <path/to/ssvm/build_folder>
$ cd tools/ssvm-evm
# Usage: ./ssvm-evm wasm_file.wasm call_data_in_string_format gas_limit
$ ./ssvm-evm ethereum/erc20.wasm 4e6ec24700000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000064 10000
# Expect output:
 Info: Start running...
 Error: Reverted.
 =================  Statistics  =================
 Total execution time: 152 us
 Wasm instructions execution time: 152 us
 Host functions execution time: 0 us
 Executed wasm instructions count: 188
 Gas costs: 9900
 Instructions per second: 1236842
    --- result storage:
    --- return data:
```

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
Parsing arguments...
Input JSON file locates in inputJSON/input-mplus-sample.json
Output JSON file locates in outputJSON/output-mplus-sample.json
Wasm file locates in wasm/calc.wasm
 Info: Start running...
 Info: Worker execution succeeded.
 =================  Statistics  =================
 Total execution time: 23 us
 Wasm instructions execution time: 23 us
 Host functions execution time: 0 us
 Executed wasm instructions count: 9
 Gas costs: 9
 Instructions per second: 391304
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
 Info: Start running...
 Info: Worker execution succeeded.
 =================  Statistics  =================
 Total execution time: 60 us
 Wasm instructions execution time: 60 us
 Host functions execution time: 0 us
 Executed wasm instructions count: 1766
 Gas costs: 1855
 Instructions per second: 29433333
 Return value: 89
```

### Example: Factorial

```bash
# ./ssvm wasm_file.wasm [exported_func_name] [args...]
$ ./ssvm examples/factorial.wasm fac 5
 Info: Start running...
 Info: Worker execution succeeded.
 =================  Statistics  =================
 Total execution time: 33 us
 Wasm instructions execution time: 33 us
 Host functions execution time: 0 us
 Executed wasm instructions count: 55
 Gas costs: 61
 Instructions per second: 1666666
 Return value: 120
```
