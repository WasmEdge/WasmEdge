# 從原始碼編譯 WasmEdge

請參閱此說明從原始碼編譯和測試 WasmEdge 。
以下指引基於 Linux 發行版，對於 MacOS 作業系統，請參閱[在 MacOS 上編譯](build_on_mac.md)，對於 Windows 作業系統，則請參閱[在 Windows 上編譯](build_on_windows.md)。

> 若您只是需要 `master` 分支的 `HEAD` 最新的編譯版本，並不想自行編譯，您可以直接從 Github Action 的 CI artifact 下載。[請參考此範例](https://github.com/WasmEdge/WasmEdge/actions/runs/1521549504#artifacts)。

## 下載原始碼

```bash
git clone https://github.com/WasmEdge/WasmEdge.git
cd WasmEdge
```

## 檢查相依套件

WasmEdge 會基於最新版本的 LLVM 來編譯我們的最新版本。
如果您想從原始碼開始編譯，必須先安裝以下相依套件，或是直接使用我們提供的 Docker image 來編譯，我們提供了多個 Linux 發行版本。

- LLVM 12.0.0 (>= 10.0.0)
- GCC 11.1.0 (>= 9.4.0)

## 準備編譯環境

### 使用 Docker Images

Dockerhub 上的儲存庫 `wasmedge/wasmedge`

您可以使用下列的指令獲取最新的 docker image ：

```bash
docker pull wasmedge/wasmedge # 等同於 wasmedge/wasmedge:latest
```

#### 可用的標籤

| 標籤名稱                  | CPU 架構 | 基於的作業系統       | LLVM 版本 | 編譯環境               | 相容性                    | 備註                                      |
| ---                     | ---      | ---                | ---       | ---                   | ---                      | ---                                      |
| `latest`                | x86\_64  | Ubuntu 20.04 LTS   | 12.0.0    | CC=clang, CXX=clang++ | Ubuntu 20.04+            | 使用於自動測試，會一直更新到最新的 Ubuntu 版本 |
| `ubuntu-build-gcc`      | x86\_64  | Ubuntu 20.04 LTS   | 12.0.0    | CC=gcc, CXX=g++       | Ubuntu 20.04+            | 使用於自動測試，會一直更新到最新的 Ubuntu 版本 |
| `ubuntu-build-clang`    | x86\_64  | Ubuntu 20.04 LTS   | 12.0.0    | CC=clang, CXX=clang++ | Ubuntu 20.04+            | 使用於自動測試，會一直更新到最新的 Ubuntu 版本 |
| `ubuntu2004_x86_64`     | x86\_64  | Ubuntu 20.04 LTS   | 10.0.0    | CC=gcc, CXX=g++       | Ubuntu 20.04+            | 提供給熟悉 Ubuntu 20.04 LTS 版本的開發者使用 |
| `ubuntu2104_armv7l`     | armhf    | Ubuntu 21.04       | 12.0.0    | CC=gcc, CXX=g++       | Ubuntu 21.04+            | 使用於 armhf 架構的發行                    |
| `manylinux2014_x86_64`  | x86\_64  | CentOS 7, 7.9.2009 | 12.0.0    | CC=gcc, CXX=g++       | Ubuntu 16.04+, CentOS 7+ | 提供給熟悉 CentOS x86\_64 架構的開發者使用  |
| `manylinux2014_aarch64` | aarch64  | CentOS 7, 7.9.2009 | 12.0.0    | CC=gcc, CXX=g++       | Ubuntu 16.04+, CentOS 7+ | 提供給熟悉 CentOS aarch64 架構的開發者使用 |

### 在 Ubuntu 20.04 上手動安裝相依套件

```bash
# 工具和函式庫
sudo apt install -y \
    software-properties-common \
    cmake \
    libboost-all-dev

# 需要 llvm 來支援 wasmedgec 工具
sudo apt install -y \
    llvm-12-dev \
    liblld-12-dev

# WasmEdge 同時支援 clang++ 和 g++ 編譯器
# 您可以選擇其中一個來編譯這個專案
# 如果您傾向使用 gcc
sudo apt install -y gcc g++
# 或者您選擇使用 clang
sudo apt install -y clang
```

### 對舊版作業系統的支援

我們的開發環境需要 `libLLVM-12` 和 `>=GLIBCXX_3.4.33` 。

如果使用者使用比 Ubuntu 20.04 更舊版的作業系統，請使用我們提供的 docker image 來編譯 WasmEdge 。
若您在尋找舊版作業系統上使用的執行檔與函式庫，我們也提供了幾個基於 manylinux\* 發行版的安裝檔。

| 可移植的 Linux 發行版標籤                  | 基礎 image   | 提供的環境需                                                           | Docker image                              |
| ---                                     | ---         | ---                                                                   | ---                                      |
| `manylinux2014`                         | CentOS 7.9  | GLIBC <= 2.17</br>CXXABI <= 1.3.7</br>GLIBCXX <= 3.4.19</br>GCC <= 4.8.0 | wasmedge/wasmedge:manylinux2014\_x86\_64 |
| `manylinux2014`                         | CentOS 7.9  | GLIBC <= 2.17</br>CXXABI <= 1.3.7</br>GLIBCXX <= 3.4.19</br>GCC <= 4.8.0 | wasmedge/wasmedge:manylinux2014\_aarch64 |

## 編譯 WasmEdge

WasmEdge 提供了各種工具來支援更好的性能以及更多樣的執行期環境，
編譯完成後，您可以找到以下幾個 WasmEdge 相關工具：

1. `wasmedge` 是通用的 WASM runtime 。
   - `wasmedge` 可以在直譯器模式下執行一個 `WASM` 檔案，也可以在 Ahead-of-time 模式下執行一個 `so` 檔案。
   - 您可以透過將 CMake 選項 `WASMEDGE_BUILD_TOOLS` 設為 `OFF` 來禁止編譯所有工具。
2. `wasmedgec` 是一個 WASM Ahead-of-time 編譯器。
   - `wasmedgec` 可以將一個通用的 `WASM` 檔案編譯成 `so` 檔案。
   - 您可以透過將 CMake 選項 `WASMEDGE_BUILD_AOT_RUNTIME` 設為 `OFF` 來禁止編譯 Ahead-of-time 編譯器。
3. `libwasmedge.so` 是 WasmEdge C API 的共享函式庫。
   - `libwasmedge.so` 提供了連接 WASM runtime 和 Ahead-of-time 編譯器的 C 語言 API。
   - 如果 `WASMEDGE_BUILD_AOT_RUNTIME` 選項被設為 `OFF` ，與 Ahead-of-time 編譯器相關的 API 都將回傳錯誤。
   - 您可以透過將 CMake 選項 `WASMEDGE_BUILD_SHARED_LIB` 設為 `OFF` 來禁止編譯 WasmEdge C API 的共享函式庫。
4. `ssvm-qitc` 與 AI 應用程式相關，是支援 ONNX 格式的 AI 模型的 ONNC runtime 。
   - 若您想嘗試使用 `ssvm-qitc` ，請參考 [ONNC-Wasm](https://github.com/ONNC/onnc-wasm) 專案來設定工作環境與執行幾個範例。
   - 這是我們的 [ONNC-Wasm Tutorial （ YouTube 影片 ）](https://www.youtube.com/watch?v=cbiPuHMS-iQ) 。

```bash
# 下載 WasmEdge docker image 後
$ docker run -it --rm \
    -v <path/to/your/wasmedge/source/folder>:/root/wasmedge \
    wasmedge/wasmedge:latest
(docker)$ cd /root/wasmedge
(docker)$ mkdir -p build && cd build
(docker)$ cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_TESTS=ON .. && make -j
```

### 如果您不需要編譯 Ahead-of-time 編譯器

如果使用者不需要 Ahead-of-time 編譯器支援，可以將 CMake 選項 `WASMEDGE_BUILD_AOT_RUNTIME` 設為 `OFF` 。

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_AOT_RUNTIME=OFF ..
```

## 執行內建測試

內建的測試僅有編譯選項 `WASMEDGE_BUILD_TESTS` 設為 `ON` 的時候才會啟用。

使用者可以使用這些測試來驗證 WasmEdge 執行檔與函式庫的正確性。

```bash
cd <path/to/wasmedge/build_folder>
LD_LIBRARY_PATH=$(pwd)/lib/api ctest
```

## 執行應用程式

接下來，請參考 [這份文件](../index.md) 在 `wasmedge` 上執行 WebAssembly 應用程式。
