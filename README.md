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
    -v <path/to/your/ssvm/source/folder>:/root/ssvm hydai/ssvm-dev:0.1.0

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
# Usage: ./ssvm-evm wasm_file.wasm call_data_in_string_format
$ ./ssvm-evm ethereum/erc20.wasm 4e6ec24700000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000064
# Expect output:
 !!! start running...
 Host func ethereum::getCallDataSize cost 1 us
 Host func ethereum::callDataCopy cost 0 us
 Host func ethereum::callDataCopy cost 0 us
 Host func ethereum::storageLoad cost 13 us
 Host func ethereum::storageStore cost 9 us
 Host func ethereum::callStatic cost 3 us
 Host func ethereum::returnDataCopy cost 0 us
 Host func ethereum::callStatic cost 2 us
 Host func ethereum::returnDataCopy cost 0 us
 Host func ethereum::storageLoad cost 5 us
 Host func ethereum::storageStore cost 9 us
 Host func ethereum::finish cost 1 us
 --- exec success
 Instructions execution cost 273 us
 Host functions cost 50 us
 Total executed instructions: 1041
 Instructions per second: 3813186
    --- result storage:
         0000000000000000000000000000000000000000000000000000000000000000 0000000000000000000000000000000000000000000000000000000000000064
         f5b24dcea0e9381721a8c72784d30cfe64c11b4591226269f839d095b3e9cf10 0000000000000000000000000000000000000000000000000000000000000064
    --- return data:
```

## Run ssvm (SSVM with general wasm runtime)
```bash
$ cd <path/to/ssvm/build_folder>
$ cd tools/ssvm
# ./ssvm wasm_file.wasm [exported_func_name]
```
