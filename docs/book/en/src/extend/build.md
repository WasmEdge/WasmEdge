# Build WasmEdge from source

Please follow this guide to build and test WasmEdge from the source code.

The following guide is for Linux distributions. For MacOS, please refer to [Build for macOS](build_on_mac.md). For Windows, please refer to [Build for Windows](build_on_windows.md). For Android, please refer to [Build for Android](build_for_android.md).

> If you just want the latest builds from the `HEAD` of the `master` branch, and do not want to build it yourself, you can download the release package directly from our Github Action's CI artifact. [Here is a build](https://github.com/WasmEdge/WasmEdge/actions/runs/1521549504#artifacts).

## Get Source Code

```bash
git clone https://github.com/WasmEdge/WasmEdge.git
cd WasmEdge
```

## Check Dependencies

WasmEdge will try to use the latest LLVM release to build the nightly build.
If you want to build from source, you may need to install these dependencies by yourself or using our docker images which support several Linux distributions.

- LLVM 12.0.0 (>= 10.0.0)
- GCC 11.1.0 (>= 9.4.0)

## Prepare the Environment

### Docker Images

Repository on [dockerhub](https://hub.docker.com/search?q=wasmedge) `wasmedge/wasmedge`

You can use the following commands to get our latest docker image:

```bash
docker pull wasmedge/wasmedge # Pulls the latest - wasmedge/wasmedge:latest
```

#### Available Tags

| tag name                | arch    | based operating system | LLVM version | ENVs                  | compatibility            | comments                                                                            |
| ---                     | ---     | ---                    | ---          | ---                   | ---                      | ---                                                                                 |
| `latest`                | x86\_64 | Ubuntu 20.04 LTS       | 12.0.0       | CC=clang, CXX=clang++ | Ubuntu 20.04+            | This is for CI, will always use the latest Ubuntu release                           |
| `ubuntu-build-gcc`      | x86\_64 | Ubuntu 20.04 LTS       | 12.0.0       | CC=gcc, CXX=g++       | Ubuntu 20.04+            | This is for CI, will always use the latest Ubuntu release                           |
| `ubuntu-build-clang`    | x86\_64 | Ubuntu 20.04 LTS       | 12.0.0       | CC=clang, CXX=clang++ | Ubuntu 20.04+            | This is for CI, will always use the latest Ubuntu release                           |
| `ubuntu2004_x86_64`     | x86\_64 | Ubuntu 20.04 LTS       | 10.0.0       | CC=gcc, CXX=g++       | Ubuntu 20.04+            | This is for developers who familiar with Ubuntu 20.04 LTS release                   |
| `ubuntu2104_armv7l`     | armhf   | Ubuntu 21.04           | 12.0.0       | CC=gcc, CXX=g++       | Ubuntu 21.04+            | This is for armhf release                                                           |
| `manylinux2014_x86_64`  | x86\_64 | CentOS 7, 7.9.2009     | 12.0.0       | CC=gcc, CXX=g++       | Ubuntu 16.04+, CentOS 7+ | This is for developers who familiar with CentOS on x86\_64 architecture             |
| `manylinux2014_aarch64` | aarch64 | CentOS 7, 7.9.2009     | 12.0.0       | CC=gcc, CXX=g++       | Ubuntu 16.04+, CentOS 7+ | This is for developers who familiar with CentOS on aarch64 architecture             |

### Install dependencies on Ubuntu 20.04 manually

```bash
# Tools and libraries
sudo apt install -y \
   software-properties-common \
   cmake \
   libboost-all-dev

# And you will need to install llvm for wasmedgec tool
sudo apt install -y \
   llvm-12-dev \
   liblld-12-dev

# WasmEdge supports both clang++ and g++ compilers
# You can choose one of them for building this project
# If you prefer GCC then
sudo apt install -y gcc g++

# Or if you prefer clang then
sudo apt install -y clang-12
```

### Support for legacy operating systems

Our development environment requires `libLLVM-12` and `>=GLIBCXX_3.4.33`.

If users are using operating systems older than Ubuntu 20.04, please use our special docker image to build WasmEdge.
If you are looking for the pre-built binaries for the older operating system, we also provide several pre-built binaries based on manylinux\* distribution.

| Portable Linux Built Distributions Tags | Base Image  | Provided Requirements                                                 | Docker Image                             |
| ---                                     | ---         | ---                                                                   | ---                                      |
| `manylinux2014`                         | CentOS 7.9  | GLIBC <= 2.17</br>CXXABI <= 1.3.7</br>GLIBCXX <= 3.4.19</br>GCC <= 4.8.0 | wasmedge/wasmedge:manylinux2014\_x86\_64 |
| `manylinux2014`                         | CentOS 7.9  | GLIBC <= 2.17</br>CXXABI <= 1.3.7</br>GLIBCXX <= 3.4.19</br>GCC <= 4.8.0 | wasmedge/wasmedge:manylinux2014\_aarch64 |

## Build WasmEdge

WasmEdge provides various tools for enabling different runtime environments for optimal performance.
You can find that there are several wasmedge related tools:

1. `wasmedge` is the general wasm runtime.
   - `wasmedge` executes a `WASM` file in the interpreter mode or a compiled WASM `so` (shared object) file in the ahead-of-time compilation mode.
   - To disable building all tools, you can set the CMake option `WASMEDGE_BUILD_TOOLS` to `OFF`.
2. `wasmedgec` is the ahead-of-time `WASM` compiler.
   - `wasmedgec` compiles a general `WASM` file into a `so` (shared object) file.
   - To disable building the ahead-of-time compiler only, you can set the CMake option `WASMEDGE_BUILD_AOT_RUNTIME` to `OFF`.
3. `libwasmedge.so` is the WasmEdge C API shared library.
   - `libwasmedge.so` provides the C API for the ahead-of-time compiler and the WASM runtime.
   - The APIs related to the ahead-of-time compiler will always fail if the CMake option `WASMEDGE_BUILD_AOT_RUNTIME` is set as `OFF`.
   - To disable building just the shared library, you can set the CMake option `WASMEDGE_BUILD_SHARED_LIB` to `OFF`.
4. `ssvm-qitc` is for AI applications and supports the ONNC runtime for AI models in the ONNX format.
   - If you want to try `ssvm-qitc`, please refer to [ONNC-Wasm](https://github.com/ONNC/onnc-wasm) project to set up the working environment and tryout several examples.
   - And here is our [tutorial for ONNC-Wasm project(YouTube Video)](https://www.youtube.com/watch?v=cbiPuHMS-iQ).

```bash
# After pulling our wasmedge docker image
docker run -it --rm \
    -v <path/to/your/wasmedge/source/folder>:/root/wasmedge \
    wasmedge/wasmedge:latest

# In docker
cd /root/wasmedge

# If you don't use docker then you need to run only the following commands in the cloned repository root
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_TESTS=ON .. && make -j
```

### Building Options

Developers can set the CMake options to customize the WasmEdge building.

1. `WASMEDGE_BUILD_TESTS`: build the WasmEdge tests. Default is `OFF`.
2. `WASMEDGE_BUILD_AOT_RUNTIME`: build with the Ahead-of-Time compiler supporting. Default is `ON`.
3. `WASMEDGE_BUILD_SHARED_LIB`: build the WasmEdge shared library (`libwasmedge.so`, `libwasmedge.dylib`, or `wasmedge.dll`). Default is `ON`.
    - By default, the WasmEdge shared library will link to the LLVM shared library.
4. `WASMEDGE_BUILD_STATIC_LIB`: build the WasmEdge static library (`libwasmedge.a`, Linux and MacOS platforms, experimental). Default is `OFF`.
    - If this option is set as `ON`, the option `WASMEDGE_FORCE_DISABLE_LTO` will forcefully be set as `ON`.
    - If this option is set as `ON`, the `libz` and `libtinfo` on Linux platforms will be statically linked.
5. `WASMEDGE_BUILD_TOOLS`: build the `wasmedge` and `wasmedgec` tools. Default is `ON`.
    - The `wasmedge` and `wasmedgec` tools will link to the WasmEdge shared library by default.
    - If this option is set as `ON` and `WASMEDGE_BUILD_AOT_RUNTIME` is set as `OFF`, the `wasmedgec` tool for the AOT compiler will not be built.
    - If this option is set as `ON` but the option `WASMEDGE_LINK_TOOLS_STATIC` is set as `OFF`, the option `WASMEDGE_BUILD_SHARED_LIB` will forcefully be set as `ON`.
    - If this option and the option `WASMEDGE_LINK_TOOLS_STATIC` are both set as `ON`, the `WASMEDGE_LINK_LLVM_STATIC` and `WASMEDGE_BUILD_STATIC_LIB` will both be set as `ON`, and the `wasmedge` and `wasmedgec` tools will link to the WasmEdge static library instead. In this case, the plugins will not work in tools.
6. `WASMEDGE_BUILD_PLUGINS`: build the WasmEdge plugins. Default is `ON`.
7. `WASMEDGE_BUILD_EXAMPLE`: build the WasmEdge examples. Default is `OFF`.
8. `WASMEDGE_PLUGIN_WASI_NN_BACKEND`: build the WasmEdge WASI-NN plugin (Linux platforms only). Default is empty.
    - This option is useless if the option `WASMEDGE_BUILD_PLUGINS` is set as `OFF`.
    - To build the WASI-NN plugin with backend, please use `-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=<backend_name>`.
    - To build the WASI-NN plugin with multiple backends, please use `-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=<backend_name1>,<backend_name2>`.
9. `WASMEDGE_PLUGIN_WASI_CRYPTO`: build the WasmEdge WASI-Crypto plugin (Linux platforms only). Default is `OFF`.
    - This option is useless if the option `WASMEDGE_BUILD_PLUGINS` is set as `OFF`.
10. `WASMEDGE_FORCE_DISABLE_LTO`: forcefully turn off the link time optimization. Default is `OFF`.
11. `WASMEDGE_LINK_LLVM_STATIC`: link the LLVM and lld libraries statically (Linux and MacOS platforms only, experimental). Default is `OFF`.
12. `WASMEDGE_LINK_TOOLS_STATIC`: make the `wasmedge` and `wasmedgec` tools to link the WasmEdge library and LLVM libraries statically (Linux and MacOS platforms only, experimental). Default is `OFF`.
    - If the option `WASMEDGE_BUILD_TOOLS` and this option are both set as `ON`, the `WASMEDGE_LINK_LLVM_STATIC` will be set as `ON`.

## Run built-in tests

The following built-in tests are only available when the build flag `WASMEDGE_BUILD_TESTS` is set to `ON`.

Users can use these tests to verify the correctness of WasmEdge binaries built.

```bash
cd <path/to/wasmedge/build_folder>
LD_LIBRARY_PATH=$(pwd)/lib/api ctest
```

## Run applications

Next, follow [this guide](../index.md) to run WebAssembly bytecode programs in `wasmedge`.
