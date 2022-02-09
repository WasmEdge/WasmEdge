# Build from source on MacOS

Currently, WasmEdge project on MacOS supports both Intel and M1 models. However, we only test and develop on Big Sur and Catalina.

* Model:
  * Intel
  * M1
* Operating System
  * Big Sur
  * Catalina

If you would like to develop WasmEdge on MacOS, please follow this guide to build and test from source code.

## Get Source Code

```bash
git clone https://github.com/WasmEdge/WasmEdge.git
cd WasmEdge
```

## Requirements and Dependencies

WasmEdge will try to use the latest LLVM release to create our nightly build.
If you want to build from source, you may need to install these dependencies by yourself.

* LLVM 12.0.0 (>= 10.0.0), installed via brew, please don't use the built-in one.
* Because the default version of LLVM on the latest brew is 13. Please use `llvm@12` to fix the LLVM version.

### Prepare the environment

```bash
# Tools and libraries
brew install boost cmake ninja llvm@12
# Use brew version of llvm, not the built-in one.
export PATH="/usr/local/opt/llvm@12/bin:$PATH"
export LDFLAGS="-L/usr/local/opt/llvm@12/lib -Wl,-rpath,/usr/local/opt/llvm@12/lib"
export CPPFLAGS="-I/usr/local/opt/llvm@12/include"
```

### If you don't want to build Ahead-of-Time runtime/compiler

If you don't need Ahead-of-Time runtime/compiler support, you can set the CMake option `WASMEDGE_BUILD_AOT_RUNTIME` to `OFF`.

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_AOT_RUNTIME=OFF ..
```

## Build WasmEdge

WasmEdge provides various tools for enabling different runtime environments for optimal performance.
After the build is finished, you can find there are several wasmedge related tools:

1. `wasmedge` is for general wasm runtime.
   * `wasmedge` executes a `WASM` file in interpreter mode or a compiled WASM `dyld` file in ahead-of-time compilation mode.
   * To disable building all tools, you can set the CMake option `WASMEDGE_BUILD_TOOLS` to `OFF`.
2. `wasmedgec` is for ahead-of-time `WASM` compiler.
   * `wasmedgec` compiles a general `WASM` file into a `dyld` file.
   * To disable building the ahead-of-time compiler only, you can set the CMake option `WASMEDGE_BUILD_AOT_RUNTIME` to `OFF`.
3. `libwasmedge_c.dyld` is the WasmEdge C API shared library.
   * `libwasmedge_c.dyld` provides C API for the ahead-of-time compiler and the WASM runtime.
   * The APIs about the ahead-of-time compiler will always return failed if the CMake option `WASMEDGE_BUILD_AOT_RUNTIME` is set as `OFF`.
   * To disable building the shared library only, you can set the CMake option `WASMEDGE_BUILD_SHARED_LIB` to `OFF`.

```bash
cmake -Bbuild -GNinja -DWASMEDGE_BUILD_PACKAGE="TGZ" -DWASMEDGE_BUILD_TESTS=ON .
cmake --build build
```

## Run built-in tests

The following built-in tests are only available when the build flag `WASMEDGE_BUILD_TESTS` sets to `ON`.

Users can use these tests to verify the correctness of WasmEdge binaries.

```bash
export DYLD_LIBRARY_PATH="$(pwd)/build/lib/api:$DYLD_LIBRARY_PATH"
cmake --build build --target test
```

## Run applications

Next, follow [this guide](../index.md) to run WebAssembly bytecode programs in `wasmedge`.

## Known issues

The following tests can not pass on macos, we are investigating these issues:

* wasmedgeAPIVMCoreTests
* wasmedgeAPIStepsCoreTests
* wasmedgeAPIAOTCoreTests
* wasmedgeProcessTests
