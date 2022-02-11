# Build from source on Windows 10

WasmEdge supports Windows 10. Currently, we also provide pre-built binaries and libraries for Windows.

You can find the details [here](https://github.com/WasmEdge/WasmEdge/blob/master/.github/workflows/build.yml#L266-L322)

If you would like to develop WasmEdge on Windows 10, please follow this guide to build and test from source code.

## Get Source Code

```bash
git clone https://github.com/WasmEdge/WasmEdge.git
cd WasmEdge
```

## Requirements and Dependencies

WasmEdge will try to use the latest LLVM release to create our nightly build.
If you want to build from source, you may need to install these dependencies by yourself.

- Chocolatey, we use it to install cmake, ninja, and vswhere
- Windows SDK 19041
- LLVM 13.0.0, you can find the pre-built files in the following section.

### Prepare the environment

```powershell
# Instal tools
choco install cmake ninja vswhere

$vsPath = (vswhere -latest -property installationPath)
Import-Module (Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64 -winsdk=10.0.19041.0"

# Download our pre-built LLVM 13
$llvm = "LLVM-13.0.0-win64.zip"
curl -sLO https://github.com/WasmEdge/llvm-windows/releases/download/llvmorg-13.0.0/LLVM-13.0.0-win64.zip -o $llvm
Expand-Archive -Path $llvm

# Set LLVM environment
$llvm_dir = "$pwd\\LLVM-13.0.0-win64\\LLVM-13.0.0-win64\\lib\\cmake\\llvm"
$Env:CC = "clang-cl"
$Env:CXX = "clang-cl"
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
   - `wasmedge` executes a `WASM` file in interpreter mode or a compiled WASM `dll` file in ahead-of-time compilation mode.
   - To disable building all tools, you can set the CMake option `WASMEDGE_BUILD_TOOLS` to `OFF`.
2. `wasmedgec` is for ahead-of-time `WASM` compiler.
   - `wasmedgec` compiles a general `WASM` file into a `dll` file.
   - To disable building the ahead-of-time compiler only, you can set the CMake option `WASMEDGE_BUILD_AOT_RUNTIME` to `OFF`.
3. `libwasmedge_c.dll` is the WasmEdge C API shared library.
   - `libwasmedge_c.dll` provides C API for the ahead-of-time compiler and the WASM runtime.
   - The APIs about the ahead-of-time compiler will always return failed if the CMake option `WASMEDGE_BUILD_AOT_RUNTIME` is set as `OFF`.
   - To disable building the shared library only, you can set the CMake option `WASMEDGE_BUILD_SHARED_LIB` to `OFF`.

```bash
$vsPath = (vswhere -latest -property installationPath)
Import-Module (Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64 -winsdk=10.0.19041.0"

cmake -Bbuild -GNinja -DCMAKE_SYSTEM_VERSION=10.0.19041.0 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL "-DLLVM_DIR=$llvm_dir" -DWASMEDGE_BUILD_TESTS=ON -DWASMEDGE_BUILD_PACKAGE="ZIP" .
cmake --build build
```

## Run built-in tests

The following built-in tests are only available when the build flag `WASMEDGE_BUILD_TESTS` sets to `ON`.

Users can use these tests to verify the correctness of WasmEdge binaries.

```bash
$vsPath = (vswhere -latest -property installationPath)
Import-Module (Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64 -winsdk=10.0.19041.0"

$Env:PATH += ";$pwd\\build\\lib\\api"
cd build
ctest --output-on-failure
cd -
```

## Run applications

Next, follow [this guide](run.md) to run WebAssembly bytecode programs in `wasmedge`.
