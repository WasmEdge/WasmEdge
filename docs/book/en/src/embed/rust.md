
# WasmEdge Rust SDK

<!-- @import "[TOC]" {cmd="toc" depthFrom=1 depthTo=6 orderedList=false} -->

<!-- code_chunk_output -->

- [WasmEdge Rust SDK](#wasmedge-rust-sdk)
  - [Introduction](#introduction)
  - [Versioning Table](#versioning-table)
  - [`wasmedge-sdk` crate](#wasmedge-sdk-crate)
  - [`wasmedge-sys` crate](#wasmedge-sys-crate)
  - [Docker image](#docker-image)
    - [Windows Users](#windows-users)
  - [Examples](#examples)
    - [`wasmedge-sdk` Examples](#wasmedge-sdk-examples)
    - [`wasmedge-sys` Examples](#wasmedge-sys-examples)

<!-- /code_chunk_output -->

## Introduction

`WasmEdge` supports embedding into Rust applications via WasmEdge Rust SDK. WasmEdge Rust SDK consists of three crates:

- [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) crate. It defines a group of safe, ergonomic high-level APIs, which are used to build up business applications.
  - [API documentation](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/)

- [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crate. It defines a group of low-level Rust APIs, which simply wrap WasmEdge C-API and provide the safe counterparts. It is not recommended to use it directly to build up application.
  - [API documentation](https://wasmedge.github.io/WasmEdge/wasmedge_sys/)

- [wasmedge-types](https://crates.io/crates/wasmedge-types) crate. The data structures that are commonly used in `wasmedge-sdk` and `wasmedge-sys` are defined in this crate.
  - [API documentation](https://wasmedge.github.io/WasmEdge/wasmedge_types/)

## Versioning Table

  The following table provides the versioning information about each release of `wasmedge-sdk` crate and its dependencies.

  | wasmedge-sdk  | WasmEdge lib                                                       | wasmedge-sys  | wasmedge-types|
  | :-----------: | :----------------------------------------------------------------: | :-----------: | :-----------: |
  | 0.1.0         | [0.10.0](https://github.com/WasmEdge/WasmEdge/releases/tag/0.10.0) | 0.7           | 0.1           |

## `wasmedge-sdk` crate

To use `wasmedge-sdk` in your project, you should finish the followign two steps before building your project:

- First, deploy `WasmEdge` library on your local system.

  You can reference the versiong table and download `WasmEdge` library from [WasmEdge Releases Page](https://github.com/WasmEdge/WasmEdge/releases). After download the `WasmEdge` library, you can choose one of the following three ways to specify the locations of the required files:
  
  - By default location

    For those who do not want to define environment variables, you can put the downloaded `WasmEdge` binary package in the default location `$HOME/.wasmedge/`. The directory structure of the default location should looks like below:

    ```bash
    // $HOME/.wasmedge/ on Ubuntu-20.04
    .
    ├── bin
    │   ├── wasmedge
    │   └── wasmedgec
    ├── include
    │   └── wasmedge
    │       ├── dense_enum_map.h
    │       ├── enum.inc
    │       ├── enum_configure.h
    │       ├── enum_errcode.h
    │       ├── enum_types.h
    │       ├── int128.h
    │       ├── spare_enum_map.h
    │       ├── version.h
    │       └── wasmedge.h
    └── lib64
        ├── libwasmedge_c.so
        └── wasmedge
            └── libwasmedgePluginWasmEdgeProcess.so

    5 directories, 13 files
    ```

    ```bash
    // $HOME/.wasmedge/ on macOS-12
    .
    ├── bin
    │   ├── wasmedge
    │   └── wasmedgec
    ├── include
    │   └── wasmedge
    │       ├── dense_enum_map.h
    │       ├── enum.inc
    │       ├── enum_configure.h
    │       ├── enum_errcode.h
    │       ├── enum_types.h
    │       ├── int128.h
    │       ├── spare_enum_map.h
    │       ├── version.h
    │       └── wasmedge.h
    └── lib
        └── libwasmedge_c.dylib

    4 directories, 12 files
    ```

  - By specifying `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR`.

    - `WASMEDGE_INCLUDE_DIR` is used to specify the directory in which the header files are.

    - `WASMEDGE_LIB_DIR` is used to specify the directory in which the `wasmedge_c` library is.

    - For example, suppose that you have already downloaded the `Wasmedge-0.10.0` binary package from [WasmEdge Releases](https://github.com/WasmEdge/WasmEdge/releases/tag/0.10.0) and placed it into a local directory, for example, `~/workspace/me/`. The directory structure of the released package should looks like below:

      ```bash
      root@0a877562f39e:~/workspace/me/WasmEdge-0.10.0-Linux# tree .
      .
      ├── bin
      │   ├── wasmedge
      │   └── wasmedgec
      ├── include
      │   └── wasmedge
      │       ├── dense_enum_map.h
      │       ├── enum.inc
      │       ├── enum_configure.h
      │       ├── enum_errcode.h
      │       ├── enum_types.h
      │       ├── int128.h
      │       ├── spare_enum_map.h
      │       ├── version.h
      │       └── wasmedge.h
      └── lib64
          ├── libwasmedge_c.so
          └── wasmedge
              └── libwasmedgePluginWasmEdgeProcess.so

      5 directories, 13 files
      ```

    Then set `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR` environment variables to specify the `include` and `lib` (here is `lib64`) directories.

      ```bash
      root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_INCLUDE_DIR=/root/workspace/me/WasmEdge-0.10.0-Linux/include/wasmedge
      
      root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_LIB_DIR=/root/workspace/me/WasmEdge-0.10.0-Linux/lib64
      ```

  - By specifying `WASMEDGE_BUILD_DIR`

    You can choose this way if you'd like to use the latest code in the `master` branch of the `WasmEdge` github repo. For example,

    - Suppose that you `git clone` WasmEdge repo in your local directory, for example, `~/workspace/me/WasmEdge`, and follow the [instructions](https://wasmedge.org/book/en/extend/build.html) to build WasmEdge native library. After that, you should find the generated `include` and `lib` directories in `~/workspace/me/WasmEdge/build`.

    - Then, set `WASMEDGE_BUILD_DIR` environment variable to specify the `build` directory.

      ```bash
      root@0a877562f39e:~/workspace/me/WasmEdge# export WASMEDGE_BUILD_DIR=/root/workspace/me/WasmEdge/build
      ```

- Second, after deploy the `WasmEdge` library on your local system, copy/paste the following code into the `Cargo.toml` file of your project. Now, you can use `cargo build` command to build your project.

```toml
[dependencies]
wasmedge-sdk = "0.1"
```

## `wasmedge-sys` crate

`wasmedge-sys` serves as a wraper layer of `WasmEdge` C-API, and provides a group of safe low-level Rust interfaces. For those who are interested in using `wasmedge-sys` in their projects, you should also deploy the `WasmEdge` library on your local system as described in the [wasmedge-sdk crate](#wasmedge-sdk-crate) section. Then, copy/paste the following code in the `Cargo.toml` file of your project.

```toml
[dependencies]
wasmedge-sys = "0.7"
```

*For Windows users, duo to [issue 1527](https://github.com/WasmEdge/WasmEdge/issues/1527), the current version of `wasmedge-sys` does not support `standalone` mode on Windows platform. Please choose to use our [docker image](#docker-image). We'll fix the issue as soon as possible.*

For those who want to build `wasmedge-sys` from source in terminal, just go to the `wasmedge-sys` directory and type `cargo build --features standalone` command that will compile the `wasmedge-sys` crate in `standalone` mode.

## Docker image

For those who would like to dev in Docker environment, you can reference the [Use Docker](/src/start/docker.md) section of this book, which details how to use Docker for `WasmEdge` application development.

### Windows Users

Duo to [issue 1527](https://github.com/WasmEdge/WasmEdge/issues/1527), WasmEdger Rust bindings can not be used directly on Windows platform for now. Please choose to use our [docker image](#docker-image). We'll fix the issue as soon as possible.

## Examples

For helping you get familiar with WasmEdge Rust bindings, the following quick examples demonstrate how to use the APIs defined in `wasmedge-sdk` and `wasmedge-sys`, respectively. In addition, we'll add more examples continuously. Please file an issue [here](https://github.com/WasmEdge/WasmEdge/issues) and let us know if you have any problems with the API usage.

### `wasmedge-sdk` Examples

- [[wasmedge-sdk] Hello World!](rust/say_hello.md)

- [[wasmedge-sdk] Memory manipulation](rust/memory_manipulation.md)

- [[wasmedge-sdk] Table and FuncRef](rust/table_and_funcref.md)

### `wasmedge-sys` Examples

- [[wasmedge-sys] Run a WebAssembly function with WasmEdge low-level APIs](rust/sys_run_host_func.md)

- [[wasmedge-sys] Compute Fibonacci numbers concurrently](rust/concurrent_fib.md)

- [[wasmedge-sys] The usage of WasmEdge module instances](rust/how_to_use_module_instance.md)
