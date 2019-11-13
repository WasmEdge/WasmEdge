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
