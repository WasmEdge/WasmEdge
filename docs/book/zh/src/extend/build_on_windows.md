# 在Windows 10由源码构建

WasmEdge可以在Windows 10上由源码构建。我们也同时提供了二进制安装包和库文件。

可以在[这里](https://github.com/WasmEdge/WasmEdge/blob/master/.github/workflows/build.yml#L266-L322)查看详情。

如果想要在Windows10上开发WasmEdge，请继续阅读本文以进行从源码构建和测试。

下文中将以`AOT`代替`ahead-of-time`，`ahead-of-time`的含义是将WASM文件提前编译为机器码。

## 下载源代码

```bash
$ git clone https://github.com/WasmEdge/WasmEdge.git
$ cd WasmEdge
```

## 依赖说明

WasmEdge会尝试用最新的LLVM发行版本来创建我们的nightly版本。

编译源码前需要安装以下依赖：

- Chocolatey，用来安装cmake，ninja和vswhere
- Windows SDK 19041
- LLVM 13.0.0，预编译版本在下节中提供

### 下载依赖

```powershell
# 下载工具
choco install cmake ninja vswhere

$vsPath = (vswhere -latest -property installationPath)
Import-Module (Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64 -winsdk=10.0.19041.0"

# 下载我们提供的预编译好的LLVM 13
$llvm = "LLVM-13.0.0-win64.zip"
curl -sLO https://github.com/WasmEdge/llvm-windows/releases/download/llvmorg-13.0.0/LLVM-13.0.0-win64.zip -o $llvm
Expand-Archive -Path $llvm

# 设置LLVM环境变量
$llvm_dir = "$pwd\\LLVM-13.0.0-win64\\LLVM-13.0.0-win64\\lib\\cmake\\llvm"
$Env:CC = "clang-cl"
$Env:CXX = "clang-cl"
```

### 如果不需要构建AOT运行时或者编译器

如果不需要AOT运行时或者编译器，可以将CMake选项`WASMEDGE_BUILD_AOT_RUNTIME` 设置为`OFF`。

```bash
$ cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_AOT_RUNTIME=OFF ..
```

## 构建WasmEdge

为应对不同的性能需求，WasmEdge提供了多种运行时环境和工具。

构建成功后，你可以找到这些wasmedge相关工具：

1. `wasmedge` 是一个通用的wasm运行时。
   * `wasmedge` 可以解释执行`WASM` 文件或者以AOT模式执行一个编译为`dyld`的WASM文件。
   * 在构建WasmEdge时，可以设置CMake选项`WASMEDGE_BUILD_TOOLS`为`OFF`来不构建所有工具。
2. `wasmedgec`是AOT`WASM`编译器。
   * `wasmedgec`将一个`WASM` 格式的文件编译为`dyld`格式文件。
   * 想要只不构建AOT编译器，可以设置CMake选项`WASMEDGE_BUILD_AOT_RUNTIME`为`OFF`。
3. `libwasmedge_c.dyld`是WasmEdge C API共享库.
   * `libwasmedge_c.dyld`为AOT编译器和WASM运行时提供C API。
   * CMake选项`WASMEDGE_BUILD_AOT_RUNTIME`设置为`OFF`后，调用与AOT编译器相关的API只会返回失败值。
   * 想要只不构建共享库，你可以将CMake选项`WASMEDGE_BUILD_SHARED_LIB`设置为`OFF`。

```bash
$vsPath = (vswhere -latest -property installationPath)
Import-Module (Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64 -winsdk=10.0.19041.0"

cmake -Bbuild -GNinja -DCMAKE_SYSTEM_VERSION=10.0.19041.0 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL "-DLLVM_DIR=$llvm_dir" -DWASMEDGE_BUILD_TESTS=ON -DWASMEDGE_BUILD_PACKAGE="ZIP" .
cmake --build build
```

## 运行内置测试

以下内置测试仅在CMake编译选项`WASMEDGE_BUILD_TESTS`为`ON`时可用。

用户可以用这些测试来验证自己构建的WasmEdge二进制文件的正确性。

```bash
$vsPath = (vswhere -latest -property installationPath)
Import-Module (Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64 -winsdk=10.0.19041.0"

$Env:PATH += ";$pwd\\build\\lib\\api"
cd build
ctest --output-on-failure
cd -
```

## 运行应用

下一步，请按照[该指导](run.md)在`wasmedge`上运行WebAssembly字节码程序。
