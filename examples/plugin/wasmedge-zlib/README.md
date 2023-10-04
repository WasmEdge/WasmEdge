# WasmEdge WASMEDGE-Zlib example.

This is a example to demonstrate how to use wasmedge-zlib plugin of WasmEdge with C++.

## Prerequisites

### Install Emscripten SDK (C++ Compiler Toolkit for wasm targets).

Follow the instructions below to install rust and wasm32-wasi target.

```bash
cd ~/
mkdir lib; cd lib
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh # Only this shell will be able to use emscripten.
```

### Install WasmEdge and WASMEDGE-zlib plugin.

Note that if you install WasmEdge using install script, you need to download `wasmedge-zlib` plugin from [release page](https://github.com/WasmEdge/WasmEdge/releases/) and put it into `$HOME/.wasmedge/plugin/`.

Or you can build wasmedge from scratch with wasmedge-zlib plugin enabled.

```sh
git clone https://github.com/WasmEdge/WasmEdge.git --depth 1
cd WasmEdge
export WASMEDGE_PATH=$PWD
# to tell wasmedge where to find wasmedge-zlib plugin.
export WASMEDGE_PLUGIN_PATH=$WASMEDGE_PATH/build/plugins/wasmedge_zlib
mkdir build; cd build
cmake .. -DWASMEDGE_PLUGIN_WASMEDGE_ZLIB=ON
# incase you don't wan't lld & AOT, try the variant below
# cmake .. -DWASMEDGE_PLUGIN_ZLIB=ON -DWASMEDGE_BUILD_AOT_RUNTIME=OFF
cmake --build . -j
# compiled wasmedge is located in: ./tools/wasmedge/wasmedge
```

## Build & Run the example as WASM Module

```sh
cd ../examples/plugin/wasmedge-zlib/
mkdir build
em++ main.cpp -O2 -o build/main.wasm -sSTANDALONE_WASM -sWARN_ON_UNDEFINED_SYMBOLS=0
```

Then we get `build/main.wasm`.

We can run this example with `wasmedge` like

```sh
../../../build/tools/wasmedge/wasmedge build/main.wasm
```

## Build & Run the example as Native executable

```sh
apt install zlib1g-dev # for ubuntu / debian distros | try zlib-devel for fedora
cd ../examples/plugin/wasmedge-zlib/
mkdir build
g++ main.cpp -o build/main -lz
./build/main
```

## Expected Output

The WASM example should run successfully and print out the following messages.

```
Compressing Buffer of size : 1048576B
Decompressing Buffer of size : 785071B
Success
```

## Difference between Native & WASM Module

- Since wasmedge-zlib ignores the custom memory allocators provided by the program for zlib library, any code on the allocator (including log statements) won't run.
- This will have no effect on the actual zlib compression & decompression & library usage, and will therefore be almost transparent to the program/module using the zlib library (wasmedge-zlib).
