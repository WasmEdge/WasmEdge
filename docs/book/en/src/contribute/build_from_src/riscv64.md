# Build and test WasmEdge on RISC-V 64 arch

Please follow this tutorial to build and test WasmEdge on the RISC-V64 system.

## Prepare the Environment

This tutorial is based on Ubuntu 22.04 host, and WasmEdge uses the [RISCV-Lab](https://gitee.com/tinylab/riscv-lab) which provide Ubuntu 22.04 system with riscv64 architecture. Here users can use their own riscv64 environment.

### Install and run RISCV-Lab

```bash
git clone https://gitee.com/tinylab/cloud-lab.git
cd cloud-lab
LOGIN=bash tools/docker/run riscv-lab
```

Note that it will take a long time to pull the image here.

## Build WasmEdge

### Get Source code

```bash
ubuntu@riscv-lab:/labs/riscv-lab$ git clone https://github.com/WasmEdge/WasmEdge.git
ubuntu@riscv-lab:/labs/riscv-lab$ cd WasmEdge
```

### Dependencies

WasmEdge requires LLVM 12 at least and you may need to install these following dependencies by yourself.

```bash
ubuntu@riscv-lab:/labs/riscv-lab$ sudo apt-get update
ubuntu@riscv-lab:/labs/riscv-lab$ sudo apt install -y software-properties-common cmake libboost-all-dev
ubuntu@riscv-lab:/labs/riscv-lab$ sudo apt install -y llvm-12-dev liblld-12-dev
```

### Compile

Please refer to [here](../contribute/build_from_src.md#cmake-building-options) for the descriptions of all CMake options.

```bash
ubuntu@riscv-lab:/labs/riscv-lab$ cd WasmEdge
ubuntu@riscv-lab:/labs/riscv-lab/WasmEdge$ mkdir -p build && cd build
ubuntu@riscv-lab:/labs/riscv-lab/WasmEdge/build$ cmake -DCMAKE_BUILD_TYPE=Release .. && make -j
```

## Test

### Execute the wasmedge tool

For the pure WebAssembly, the `wasmedge` CLI tool will execute it in interpreter mode.

```bash
ubuntu@riscv-lab:/labs/riscv-lab/WasmEdge/build$ sudo make install
ubuntu@riscv-lab:/labs/riscv-lab/WasmEdge/build$ cd ../examples/wasm
ubuntu@riscv-lab:/labs/riscv-lab/WasmEdge/examples/wasm$ wasmedge -v
wasmedge version 0.12.0-alpha.1-13-g610cc21f
ubuntu@riscv-lab:/labs/riscv-lab/WasmEdge/examples/wasm$ wasmedge --reactor fibonacci.wasm fib 10
89
ubuntu@riscv-lab:/labs/riscv-lab/WasmEdge/examples/wasm$ wasmedge --reactor add.wasm add 2 2
4
```

### Execute the wasmedgec tool

To improve the performance, the `wasmedgec` can compile WebAssembly into native machine code. After compiling with the `wasmedgec` AOT compiler, the wasmedge tool can execute the WASM in AOT mode which is much faster.

```bash
ubuntu@riscv-lab:/labs/riscv-lab/WasmEdge/examples/wasm$ wasmedgec fibonacci.wasm fibonacci_aot.wasm
[2023-02-01 22:39:15.807] [info] compile start
[2023-02-01 22:39:15.857] [info] verify start
[2023-02-01 22:39:15.866] [info] optimize start
[2023-02-01 22:39:16.188] [info] codegen start
[2023-02-01 22:39:16.403] [info] output start
[2023-02-01 22:39:16.559] [info] compile done
[2023-02-01 22:39:16.565] [info] output start
ubuntu@riscv-lab:/labs/riscv-lab/WasmEdge/examples/wasm$ time wasmedge --reactor fibonacci_aot.wasm fib 30
1346269
real    0m0.284s
user    0m0.282s
sys     0m0.005s
ubuntu@riscv-lab:/labs/riscv-lab/WasmEdge/examples/wasm$ time wasmedge --reactor fibonacci.wasm fib 30
1346269
real    0m1.814s
user    0m1.776s
sys     0m0.016s
```
