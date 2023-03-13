# Build WasmEdge on Linux

## Get the Source Code

```bash
git clone https://github.com/WasmEdge/WasmEdge.git
cd WasmEdge
```

## Prepare the Environment

### Docker Images

The easiest way to setup the environment is using the WasmEdge docker images.

You can use the following commands to get our latest docker image [from dockerhub](https://hub.docker.com/search?q=wasmedge):

```bash
docker pull wasmedge/wasmedge # Pulls the latest - wasmedge/wasmedge:latest
```

Or you can pull with the [available tags](../../quick_start/use_docker.md#docker-images-for-building-wasmedge).

### Install Dependencies on Ubuntu Manually

For the developers who don't want to use docker, they can setup the environment on Ubuntu Manually.

Please check that these dependencies are satisfied.

- LLVM 12.0.0 (>= 10.0.0)
- (Optional) GCC 11.1.0 (>= 9.4.0), install it if you prefer to use GCC toolchain.

#### For Ubuntu 22.04

```bash
# Tools and libraries
sudo apt install -y \
   software-properties-common \
   cmake \
   libboost-all-dev

# And you will need to install llvm for the AOT runtime
sudo apt install -y \
   llvm-14-dev \
   liblld-14-dev

# WasmEdge supports both clang++ and g++ compilers.
# You can choose one of them to build this project.
# If you prefer GCC, then:
sudo apt install -y gcc g++
# Or if you prefer clang, then:
sudo apt install -y clang-14
```

#### For Ubuntu 20.04

```bash
# Tools and libraries
sudo apt install -y \
   software-properties-common \
   cmake \
   libboost-all-dev

# And you will need to install llvm for the AOT runtime
sudo apt install -y \
   llvm-12-dev \
   liblld-12-dev

# WasmEdge supports both clang++ and g++ compilers.
# You can choose one of them to build this project.
# If you prefer GCC, then:
sudo apt install -y gcc g++
# Or if you prefer clang, then:
sudo apt install -y clang-12
```

### Support for Legacy Operating Systems

Our development environment requires `libLLVM-12` and `>=GLIBCXX_3.4.33`.

If users are using operating systems older than Ubuntu 20.04, please use our special docker image to build WasmEdge.
If you are looking for the pre-built binaries for the older operating system, we also provide several pre-built binaries based on `manylinux*` distributions.

| Docker Image                              | Base Image  | Provided Requirements                                                    |
| ---                                       | ---         | ---                                                                      |
| `wasmedge/wasmedge:manylinux2014_x86_64`  | CentOS 7.9  | GLIBC <= 2.17</br>CXXABI <= 1.3.7</br>GLIBCXX <= 3.4.19</br>GCC <= 4.8.0 |
| `wasmedge/wasmedge:manylinux2014_aarch64` | CentOS 7.9  | GLIBC <= 2.17</br>CXXABI <= 1.3.7</br>GLIBCXX <= 3.4.19</br>GCC <= 4.8.0 |

## Build WasmEdge

Please refer to [here](../build_from_src.md#cmake-building-options) for the descriptions of all CMake options.

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

## Run Tests

The following tests are available only when the build option `WASMEDGE_BUILD_TESTS` is set to `ON`.

Users can use these tests to verify the correctness of WasmEdge binaries.

```bash
# In docker
cd <path/to/wasmedge/build_folder>
LD_LIBRARY_PATH=$(pwd)/lib/api ctest
```
