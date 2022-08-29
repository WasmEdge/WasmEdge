# 用 Mac 编译

目前，WasmEdge 项目同时支持 Intel 和 M1 型号的 MacOS。然而，我们只在 Big Sur 和 Catalina 上进行测试和开发。

* 型号
  * Intel
  * M1
* 操作系统
  * Big Sur
  * Catalina

如果您想在 MacOS 上开发 WasmEdge，请按照这个教程从源码进行构建和测试。

## 获取源码

```bash
git clone https://github.com/WasmEdge/WasmEdge.git
cd WasmEdge
```

## 依赖组件

WasmEdge 会基于最新版本的 LLVM 来创建我们的每日构建。如果你想从源码构建的话，需要自己手动来安装下面的这些依赖。

* LLVM 12.0.0 (>= 10.0.0)，使用 brew 安装，请勿使用系统自带的 LLVM。
* 因为最新版本 brew 中的 LLVM 默认版本是 13。请使用 `llvm@12` 来调整 LLVM 的版本。

### 环境准备

```bash
# 工具和库
$ brew install boost cmake ninja llvm@12
# 使用 brew 版本的 llvm，而不是系统自带的 LLVM。
$ export PATH="/usr/local/opt/llvm@12/bin:$PATH"
$ export LDFLAGS="-L/usr/local/opt/llvm@12/lib -Wl，-rpath，/usr/local/opt/llvm@12/lib"
$ export CPPFLAGS="-I/usr/local/opt/llvm@12/include"
```

### 如果你不需要预编译的运行时/编译器

如果你不需要预编译运行时和编译器特性的话，你可以将 CMake 配置项 `WASMEDGE_BUILD_AOT_RUNTIME` 设置成 `OFF`。

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_AOT_RUNTIME=OFF ..
```

## 构建 WasmEdge

WasmEdge 提供了丰富的工具来支撑更好的性能以及更多样的运行时环境，编译完成后，你可以找到以下几个 WasmEdge 相关的工具：

1. `wasmedge` 是 wasm 的通用运行时。
   * `wasmedge` 可以在解释器模式下执行一个 `WASM` 文件，也可以在预编译模式下执行一个 WASM `dyld` 文件。
   * 你可以通过将 CMake 配置项 `WASMEDGE_BUILD_TOOLS` 设置成 `OFF` 来禁止构建所有工具。
2. `wasmedgec` 是一个 `WASM` 预编译器。
   * `wasmedgec` 将一个通用的 `WASM` 文件编译成 `dyld` 文件。
   * 你可以通过将 CMake 配置项 `WASMEDGE_BUILD_AOT_RUNTIME` 设置成 `OFF` 来禁止构建预编译器。
3. `libwasmedge.dyld` 是 WasmEdge C API 的共享库。
   * `libwasmedge.dyld` 提供了访问预编译器和 WASM 运行时的 C 语言 API。
   * 如果 `WASMEDGE_BUILD_AOT_RUNTIME` 配置项被设置成 `OFF` ，那么与预编译器相关的 API 都将会返回错误。
   * 你可以通过将 CMake 配置项 `WASMEDGE_BUILD_SHARED_LIB` 设置成 `OFF` 来禁止构建共享库。

```bash
cmake -Bbuild -GNinja -DWASMEDGE_BUILD_PACKAGE="TGZ" -DWASMEDGE_BUILD_TESTS=ON .
cmake --build build
```

## 运行内置测试

以下内置测试只有在构建标志 `WASMEDGE_BUILD_TESTS` 设置为 `ON` 时才可用。

用户可以通过这些测试来验证 WasmEdge 二进制包的正确性。

```bash
export DYLD_LIBRARY_PATH="$(pwd)/build/lib/api:$DYLD_LIBRARY_PATH"
cmake --build build --target test
```

## 运行应用程序

接下来，参考[这个文档](../index.md)在 `wasmedge` 上运行 WebAssembly 字节码程序。

## 已知问题

以下测试无法在 MacOS 上通过，我们正在调查这些问题：

* wasmedgeAPIVMCoreTests
* wasmedgeAPIStepsCoreTests
* wasmedgeAPIAOTCoreTests
* wasmedgeProcessTests
