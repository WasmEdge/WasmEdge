# Introduction
**SSVM** is a high performance, hardware optimal Wasm Virtual Machine for AI and Blockchain applications.

# Getting Started

## Prepare environment

### Use our docker image

```Shell
$ docker pull hydai/ssvm-dev:alpha
```

### Manual

```Shell
$ sudo apt install -y \
	cmake \
	g++ \
	libboost-all-dev

```

## Build SSVM
```Shell
$ mkdir -p build && cd build
$ cmake -DBUILD_TESTS=ON .. && make
```

## Run tests
```Shell
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

## Run ssvm-evm
```Shell
$ cd tools/ssvm-evm
# ./ssvm-evm wasm_file.wasm call_value_string
$ ./ssvm-evm ethereum/erc20.wasm 4e6ec24700000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000064
```

## Run ssvm
```Shell
$ cd tools/ssvm
# ./ssvm wasm_file.wasm [exported_func_name]
```