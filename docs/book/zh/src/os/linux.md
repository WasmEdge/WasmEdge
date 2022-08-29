# Linux

WasmEdge 支持广泛的2007年之后的 Linux 发行版。官方发布包包含静态链接的二进制文件和库，适用于旧的 Linux 系统。
下表展示了 WasmEdge 官方发布包中的构建目标。

| 镜像 tag   | 指令集架构  | 基础操作系统 | LLVM 版本 | 环境变量   | 兼容性 | 说明  |
| ---                     | ---     | ---                    | ---          | ---                   | ---                      | ---                                                                                 |
| `latest`                | x86\_64 | Ubuntu 20.04 LTS       | 12.0.0       | CC=clang, CXX=clang++ | Ubuntu 20.04+            | 始终使用最新的 Ubuntu 版本提供给 CI                          |
| `ubuntu-build-gcc`      | x86\_64 | Ubuntu 20.04 LTS       | 12.0.0       | CC=gcc, CXX=g++       | Ubuntu 20.04+            | 始终使用最新的 Ubuntu 版本提供给 CI                            |
| `ubuntu-build-clang`    | x86\_64 | Ubuntu 20.04 LTS       | 12.0.0       | CC=clang, CXX=clang++ | Ubuntu 20.04+            | 始终使用最新的 Ubuntu 版本提供给 CI                            |
| `ubuntu2004_x86_64`     | x86\_64 | Ubuntu 20.04 LTS       | 10.0.0       | CC=gcc, CXX=g++       | Ubuntu 20.04+            | 提供给熟悉 Ubuntu 20.04 LTS 版本的开发者                  |
| `ubuntu2104_armv7l`     | armhf   | Ubuntu 21.04           | 12.0.0       | CC=gcc, CXX=g++       | Ubuntu 21.04+            | 提供给 armhf 发布的版本                                                           |
| `manylinux2014_x86_64`  | x86\_64 | CentOS 7, 7.9.2009     | 12.0.0       | CC=gcc, CXX=g++       | Ubuntu 16.04+, CentOS 7+ | 提供给熟悉 X86_64 架构的 CentOS 的开发者           |
| `manylinux2014_aarch64` | aarch64 | CentOS 7, 7.9.2009     | 12.0.0       | CC=gcc, CXX=g++       | Ubuntu 16.04+, CentOS 7+ | 提供给熟悉 Aarch64 架构的 CentOS 的开发者           |
