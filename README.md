# Introduction
**SSVM** is a high performance, hardware optimal Wasm Virtual Machine for AI and Blockchain applications.


# Getting Started

## Get Source Code

```bash
$ git clone git@github.com:second-state/SSVM.git
$ cd SSVM
$ git checkout 0.1.0
```

## Prepare environment

### Use our docker image

Our docker image use `ubuntu 18.04` as base.

```bash
$ docker pull hydai/ssvm-dev:0.1.0
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

1. `ssvm` is for general wasm runtime (Still in the very early stage).
2. `ssvm-evm` is for Ewasm runtime.

```bash
# After pulling our ssvm-dev docker image
$ docker run -it --rm \
    -v <path/to/your/ssvm/source/folder>:/root/ssvm
(docker)$ cd /root/ssvm
(docker)$ mkdir -p build && cd build
(docker)$ cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON .. && make
```

## Run built-in tests

The following built-in tests are only avaliable when the build flag `BUILD_TESTS` sets to `ON`.

Users can use these tests to verify the correctness of SSVM binaries.

```bash
$ cd <path/to/ssvm/build_folder>
$ cd test/regression
$ ./ssvmRegressionTests
$ cd ../loader
$ ./ssvmLoaderEthereumTests
$ ./ssvmLoaderFileMgrTests
$ ./ssvmLoaderWagonTests
$ cd ../ast/load
$ ./ssvmASTLoadTests
$ cd ../../evm
$ ./ssvmEVMTests
```

## Run ssvm-evm (SSVM with Ewasm runtime)

To run SSVM with Ewasm bytecode, you will need to provide a valid Ewasm bytecode and the calldata.
Currently, SSVM doesn’t support ABI encoding for Ethereum, so users need to compose the calldata by theirselves.

SSVM will dump the log information of EEI function calls and print the state of storage in the end.

SSVM provides an ERC20 token contract example for the demo purpose. See the example below.

```bash
$ cd <path/to/ssvm/build_folder>
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

## Run ssvm (SSVM with general wasm runtime)
```bash
$ cd <path/to/ssvm/build_folder>
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
