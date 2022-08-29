# Build from source on Windows 10

WasmEdge supports Windows 10. We also provide pre-built binaries and libraries for Windows. (Those using Windows 11 can use the same instructions specified here)

You can find the details [here](https://github.com/WasmEdge/WasmEdge/blob/master/.github/workflows/build.yml#L266-L322)

If you would like to develop WasmEdge on Windows 10, please follow this guide to build and test from source code.

## Get Source Code

```bash
git clone https://github.com/WasmEdge/WasmEdge.git
cd WasmEdge
```

## Requirements and Dependencies

WasmEdge will require LLVM 13 to build our nightly build and you may need to install these dependencies by yourself.

- Chocolatey, we use it to install cmake, ninja, and vswhere
- Windows SDK 19041
- LLVM 13.0.1, you can find the pre-built files [here](https://github.com/WasmEdge/llvm-windows/releases) or you can just follow the instructions/commands to download automatically.

### Prepare the environment

```powershell
# Install the required tools
choco install cmake ninja vswhere

$vsPath = (vswhere -latest -property installationPath)
Import-Module (Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64 -winsdk=10.0.19041.0"

# Download our pre-built LLVM 13 binary
$llvm = "LLVM-13.0.1-win64.zip"
curl -sLO https://github.com/WasmEdge/llvm-windows/releases/download/llvmorg-13.0.1/LLVM-13.0.1-win64.zip -o $llvm
Expand-Archive -Path $llvm

# Set LLVM environment
$llvm_dir = "$pwd\\LLVM-13.0.1-win64\\LLVM-13.0.1-win64\\lib\\cmake\\llvm"
$Env:CC = "clang-cl"
$Env:CXX = "clang-cl"
```

### If you don't need the Ahead-of-Time runtime/compiler

If you don't need Ahead-of-Time runtime/compiler support, then set the CMake option `WASMEDGE_BUILD_AOT_RUNTIME` to `OFF`.

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_AOT_RUNTIME=OFF ..
```

## Build WasmEdge

WasmEdge provides various tools for enabling different runtime environments for optimal performance.
After the build is finished, you can find there are several wasmedge related tools:

1. `wasmedge` is the general wasm runtime.
   - `wasmedge` executes a `WASM` file in the interpreter mode or a compiled WASM `dll` file in the ahead-of-time compilation mode.
   - To disable building all tools, set the CMake option `WASMEDGE_BUILD_TOOLS` to `OFF`.
2. `wasmedgec` is the ahead-of-time `WASM` compiler.
   - `wasmedgec` compiles a general `WASM` file into a `dll` file.
   - To disable building the ahead-of-time compiler, set the CMake option `WASMEDGE_BUILD_AOT_RUNTIME` to `OFF`.
3. `wasmedge.dll` is the WasmEdge C API shared library.
   - `wasmedge.dll` provides the C API for the ahead-of-time compiler and the WASM runtime.
   - The APIs related to the ahead-of-time compiler will always fail if the CMake option `WASMEDGE_BUILD_AOT_RUNTIME` is set to `OFF`.
   - To disable building the shared library, set the CMake option `WASMEDGE_BUILD_SHARED_LIB` to `OFF`.

```bash
$vsPath = (vswhere -latest -property installationPath)
Import-Module (Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64 -winsdk=10.0.19041.0"

cmake -Bbuild -GNinja -DCMAKE_SYSTEM_VERSION=10.0.19041.0 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL "-DLLVM_DIR=$llvm_dir" -DWASMEDGE_BUILD_TESTS=ON -DWASMEDGE_BUILD_PACKAGE="ZIP" .
cmake --build build
```

> If you encounter any issues in building, please check the [issues](https://github.com/WasmEdge/WasmEdge/issues/) section of the repository for help.

## Run built-in tests

The following built-in tests are available only when the build flag `WASMEDGE_BUILD_TESTS` was set to `ON`.

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

Next, you can follow [this guide](run.md) to run WebAssembly bytecode programs in `wasmedge`.
