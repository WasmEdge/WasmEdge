# Linux

WasmEdge supports a wide range of Linux distributions dated back to 2007. The official release contains statically linked binaries and libraries for older Linux systems.
The table below shows build targets in WasmEdge's official release packages.

| tag name                | arch    | based operating system | LLVM version | ENVs                  | compatibility            | comments                                                                            |
| ---                     | ---     | ---                    | ---          | ---                   | ---                      | ---                                                                                 |
| `latest`                | x86\_64 | Ubuntu 20.04 LTS       | 12.0.0       | CC=clang, CXX=clang++ | Ubuntu 20.04+            | This is for CI, will always use the latest Ubuntu release                           |
| `ubuntu-build-gcc`      | x86\_64 | Ubuntu 20.04 LTS       | 12.0.0       | CC=gcc, CXX=g++       | Ubuntu 20.04+            | This is for CI, will always use the latest Ubuntu release                           |
| `ubuntu-build-clang`    | x86\_64 | Ubuntu 20.04 LTS       | 12.0.0       | CC=clang, CXX=clang++ | Ubuntu 20.04+            | This is for CI, will always use the latest Ubuntu release                           |
| `ubuntu2004_x86_64`     | x86\_64 | Ubuntu 20.04 LTS       | 10.0.0       | CC=gcc, CXX=g++       | Ubuntu 20.04+            | This is for developers who familiar with Ubuntu 20.04 LTS release                   |
| `ubuntu2104_armv7l`     | armhf   | Ubuntu 21.04           | 12.0.0       | CC=gcc, CXX=g++       | Ubuntu 21.04+            | This is for armhf release                                                           |
| `manylinux2014_x86_64`  | x86\_64 | CentOS 7, 7.9.2009     | 12.0.0       | CC=gcc, CXX=g++       | Ubuntu 16.04+, CentOS 7+ | This is for developers who familiar with CentOS on x86\_64 architecture             |
| `manylinux2014_aarch64` | aarch64 | CentOS 7, 7.9.2009     | 12.0.0       | CC=gcc, CXX=g++       | Ubuntu 16.04+, CentOS 7+ | This is for developers who familiar with CentOS on aarch64 architecture             |
