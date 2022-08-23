# 从源码构建和测试 WasmEdge

请参考这个教程来从源码构建和测试 WasmEdge

## 获取源码

```bash
git clone https://github.com/WasmEdge/WasmEdge.git
cd WasmEdge
```

## 检查依赖

WasmEdge 会基于最新版本的 LLVM 来创建我们的每日构建。
如果你想从源码构建的话，需要自己手动来安装下面的这些依赖。你也可以直接使用我们提供的 Docker 镜像来构建， 它支持多个发行版本的 Linux 。

- LLVM 12.0.0 (>= 10.0.0)
- GCC 11.1.0 (>= 9.4.0)

## 环境准备

### Docker 镜像

Dockerhub 上的仓库 `wasmedge/wasmedge`

你可以使用下面的命令来获取我们最新的镜像：

```bash
docker pull wasmedge/wasmedge # 等同于 wasmedge/wasmedge:latest
```

#### 可用的标签

| 标签名                  | 体系结构 | 基于的操作系统     | LLVM 版本 | 环境配置              | 兼容性                   | 备注                                                                |
| ---                     | ---      | ---                | ---       | ---                   | ---                      | ---                                                                 |
| `latest`                | x86\_64  | Ubuntu 20.04 LTS   | 12.0.0    | CC=clang, CXX=clang++ | Ubuntu 20.04+            | 这个是用于持续集成的，会一直使用最新的 Ubuntu 版本                  |
| `ubuntu-build-gcc`      | x86\_64  | Ubuntu 20.04 LTS   | 12.0.0    | CC=gcc, CXX=g++       | Ubuntu 20.04+            | 这个是用于持续集成的，会一直使用最新的 Ubuntu 版本                  |
| `ubuntu-build-clang`    | x86\_64  | Ubuntu 20.04 LTS   | 12.0.0    | CC=clang, CXX=clang++ | Ubuntu 20.04+            | 这个是用于持续集成的，会一直使用最新的 Ubuntu 版本                  |
| `ubuntu2004_x86_64`     | x86\_64  | Ubuntu 20.04 LTS   | 10.0.0    | CC=gcc, CXX=g++       | Ubuntu 20.04+            | 这个提供给熟悉 Ubuntu 20.04 LTS 版本的开发者使用                    |
| `manylinux2014_x86_64`  | x86\_64  | CentOS 7, 7.9.2009 | 12.0.0    | CC=gcc, CXX=g++       | Ubuntu 16.04+, CentOS 7+ | 这个提供给熟悉 CentOS x86\_64 架构的开发者使用                      |
| `manylinux2014_aarch64` | aarch64  | CentOS 7, 7.9.2009 | 12.0.0    | CC=gcc, CXX=g++       | Ubuntu 16.04+, CentOS 7+ | 这个提供给熟悉 CentOS aarch64 架构的开发者使用                      |

### 在 Ubuntu 20.04 上手动安装依赖

```bash
# 工具和库
sudo apt install -y \
    software-properties-common \
    cmake \
    libboost-all-dev

# 你需要 llvm 来支持 wasmedgec 工具
sudo apt install -y \
    llvm-12-dev \
    liblld-12-dev

# WasmEdge 支持 clang++ 和 g++ 编译器
# 你可以选择其中任一个用来编译这个项目
# 如果你倾向于 GCC
sudo apt install -y gcc g++
# 或者你选择 clang
sudo apt install -y clang
```

### 对过时操作系统的支持

我们的开发环境依赖于 `libLLVM-12` 和 `>=GLIBCXX_3.4.33`。

如果用户的系统是比 Ubuntu 20.04 还早的版本，请使用我们定制的 docker 镜像来构建 WasmEdge 。
如果你需要的是在过时版本的操作系统上使用的二进制包，我们也提供了几个基于 manylinux\* 发行版本的安装包。

| 可移植的 Linux 发行版标签                  | 基础镜像      | 提供的依赖                                                              | Docker 镜像                              |
| ---                                     | ---         | ---                                                                   | ---                                      |
| `manylinux2014`                         | CentOS 7.9  | GLIBC <= 2.17</br>CXXABI <= 1.3.7</br>GLIBCXX <= 3.4.19</br>GCC <= 4.8.0 | wasmedge/wasmedge:manylinux2014\_x86\_64 |
| `manylinux2014`                         | CentOS 7.9  | GLIBC <= 2.17</br>CXXABI <= 1.3.7</br>GLIBCXX <= 3.4.19</br>GCC <= 4.8.0 | wasmedge/wasmedge:manylinux2014\_aarch64 |

## 构建 WasmEdge

WasmEdge 提供了丰富的工具来支撑更好的性能以及更多样的运行时环境，
编译完成后，你可以找到以下几个 WasmEdge 相关的工具：

1. `wasmedge` 是 wasm 的通用运行时。
   - `wasmedge` 可以在解释器模式下执行一个 `WASM` 文件， 也可以在预编译模式下执行一个 WASM `so` 文件。
   - 你可以通过将 CMAKE 配置项 `WASMEDGE_BUILD_TOOLS` 设置成 `OFF`来禁止所有工具的构建。
2. `wasmedgec` 是一个 `WASM` 预编译器。
   - `wasmedgec` 将一个通用的 `WASM` 文件编译成 `so` 文件。
   - 你可以通过将 CMAKE 配置项 `WASMEDGE_BUILD_AOT_RUNTIME` 设置成 `OFF`来禁止构建预编译器。
3. `libwasmedge.so` 是 WasmEdge C API 的共享库。
   - `libwasmedge.so` 提供了访问预编译器和 WASM 运行时的 C 语言 API。
   - 如果 `WASMEDGE_BUILD_AOT_RUNTIME` 配置项被设置成 `OFF`， 那么与预编译器相关的 API 都将会返回错误。
4. `ssvm-qitc` 是用来支持 AI 应用的，它支持基于 ONNX 格式的 AI 模型的 ONNC 运行时。
   - 如果你想尝试使用 `ssvm-qitc`，请参考 [ONNC-Wasm](https://github.com/ONNC/onnc-wasm) 项目来设置运行环境，并且运行几个示例。
   - 这里是我们的 [ONNC-Wasm 项目教程 （ YouTube 视频）](https://www.youtube.com/watch?v=cbiPuHMS-iQ) 。

```bash
# 获取到 wasm docker 镜像后
docker run -it --rm \
    -v <path/to/your/wasmedge/source/folder>:/root/wasmedge \
    wasmedge/wasmedge:latest
# In docker
cd /root/wasmedge
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_TESTS=ON .. && make -j
```

### 如果你不需要预编译运行时/编译器

如果用户不需要预编译运行时和编译器特性的话，可以将 CMAKE 配置项 `WASMEDGE_BUILD_AOT_RUNTIME` 设置成 `OFF`。

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_AOT_RUNTIME=OFF ..
```

## 执行内置的测试

下面所提到的测试只有在构建配置 `WASMEDGE_BUILD_TESTS` 设置为 `ON` 的时候才有效。

用户可以通过这些测试来验证 WasmEdge 二进制包的正确性。

```bash
cd <path/to/wasmedge/build_folder>
LD_LIBRARY_PATH=$(pwd)/lib/api ctest
```

## 运行应用

接下来，参考 [这个文档](../index.md) 在 `wasmedge` 上运行 WebAssembly 字节码程序。
