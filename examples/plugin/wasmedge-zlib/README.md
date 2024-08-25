# WasmEdge WASMEDGE-Zlib example

This is an example to demonstrate how to use the `wasmedge-zlib` plugin of WasmEdge with C++.

## Prerequisites

### Install Emscripten SDK (C++ Compiler Toolkit for wasm targets)

```bash
cd ~/
mkdir lib; cd lib
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh # Only this shell will be able to use emscripten.
```

### Install WasmEdge and WASMEDGE-zlib plugin

Note that if you install WasmEdge using the install script, you need to download `wasmedge-zlib` plugin from the [release page](https://github.com/WasmEdge/WasmEdge/releases/) and put it into `$HOME/.wasmedge/plugin/`.

Or you can build Wasmedge from scratch with `wasmedge-zlib` plugin enabled.

```bash
git clone https://github.com/WasmEdge/WasmEdge.git --depth 1
cd WasmEdge
export WASMEDGE_PATH=$PWD
# To tell Wasmedge where to find wasmedge-zlib plugin.
export WASMEDGE_PLUGIN_PATH=$WASMEDGE_PATH/build/plugins/wasmedge_zlib
mkdir build; cd build
cmake .. -DWASMEDGE_PLUGIN_WASMEDGE_ZLIB=ON
# In case you don't want `AOT` support, try the variant below
# cmake .. -DWASMEDGE_PLUGIN_ZLIB=ON -DWASMEDGE_USE_LLVM=OFF
cmake --build . -j
# Compiled Wasmedge is located in ./tools/wasmedge/wasmedge
```

## Build and Run the example as a WASM Module

```bash
cd ../examples/plugin/wasmedge-zlib/
mkdir build
em++ main.cpp -O2 -o build/main.wasm -sSTANDALONE_WASM -sWARN_ON_UNDEFINED_SYMBOLS=0
```

Then we get `build/main.wasm`.

We can run this example with `Wasmedge` with the following command

```bash
../../../build/tools/wasmedge/wasmedge build/main.wasm
```

## Build and Run the example as a Native executable

```bash
apt install zlib1g-dev # For Ubuntu / Debian distros | Try zlib-devel for fedora
cd ../examples/plugin/wasmedge-zlib/
mkdir build
g++ main.cpp -o build/main -lz
./build/main
```

## Expected Output

The WASM example should run successfully and print out the following messages.

```bash
Compressing Buffer of size : 1048576B
Decompressing Buffer of size : 785071B
Success
```

## Difference between Native & WASM Module

- Since `wasmedge-zlib` ignores the custom memory allocators provided by the program to the zlib library, any code on the custom allocator function won't run.
- This will not affect the actual zlib compression, decompression, or any library usage, and will therefore be almost transparent to the program/module using the zlib library (`wasmedge-zlib`).
