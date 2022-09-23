# Build WasmEdge on Windows 10

You can also find the details [here](https://github.com/WasmEdge/WasmEdge/blob/master/.github/workflows/reusable-build-on-windows.yml#L37-L48).

## Get Source Code

```bash
git clone https://github.com/WasmEdge/WasmEdge.git
cd WasmEdge
```

## Requirements and Dependencies

WasmEdge requires LLVM 13 and you may need to install these following dependencies by yourself.

- Chocolatey, we use it to install `cmake`, `ninja`, and `vswhere`.
- Windows SDK 19041
- LLVM 13.0.1, you can find the pre-built files [here](https://github.com/WasmEdge/llvm-windows/releases) or you can just follow the `instructions/commands` to download automatically.

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

## Build WasmEdge

```bash
$vsPath = (vswhere -latest -property installationPath)
Import-Module (Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64 -winsdk=10.0.19041.0"

cmake -Bbuild -GNinja -DCMAKE_SYSTEM_VERSION=10.0.19041.0 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL "-DLLVM_DIR=$llvm_dir" -DWASMEDGE_BUILD_TESTS=ON -DWASMEDGE_BUILD_PACKAGE="ZIP" .
cmake --build build
```

## Run Tests

The following tests are available only when the build option `WASMEDGE_BUILD_TESTS` was set to `ON`.

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
