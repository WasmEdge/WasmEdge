# WasmEdge Rust SDK

<!-- @import "[TOC]" {cmd="toc" depthFrom=1 depthTo=6 orderedList=false} -->

<!-- code_chunk_output -->

- [WasmEdge Rust SDK](#wasmedge-rust-sdk)
  - [Introduction](#introduction)
  - [Versioning Table](#versioning-table)
  - [Build](#build)
  - [`wasmedge-sys` crate](#wasmedge-sys-crate)
  - [Enable WasmEdge Plugins](#enable-wasmedge-plugins)
  - [Docker image](#docker-image)
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

| wasmedge-sdk  | WasmEdge lib  | wasmedge-sys  | wasmedge-types| wasmedge-macro|
| :-----------: | :-----------: | :-----------: | :-----------: | :-----------: |
| 0.5.0         | 0.11.1        | 0.10          | 0.3.0         | 0.1.0         |
| 0.4.0         | 0.11.0        | 0.9           | 0.2.1         | -             |
| 0.3.0         | 0.10.1        | 0.8           | 0.2           | -             |
| 0.1.0         | 0.10.0        | 0.7           | 0.1           | -             |

## Build

To use `wasmedge-sdk` in your project, you should finish the following two steps before building your project:

- First, deploy `WasmEdge` library on your local system.

  You can reference the versioning table and download `WasmEdge` library from [WasmEdge Releases Page](https://github.com/WasmEdge/WasmEdge/releases). After download the `WasmEdge` library, you can choose one of the following three ways to specify the locations of the required files:
  
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
    │       ├── enum.inc
    │       ├── enum_configure.h
    │       ├── enum_errcode.h
    │       ├── enum_types.h
    │       ├── int128.h
    │       ├── version.h
    │       └── wasmedge.h
    └── lib64
        ├── libwasmedge.so
        └── wasmedge
            └── libwasmedgePluginWasmEdgeProcess.so

    5 directories, 11 files
    ```

    ```bash
    // $HOME/.wasmedge/ on macOS-12
    .
    ├── bin
    │   ├── wasmedge
    │   └── wasmedgec
    ├── include
    │   └── wasmedge
    │       ├── enum.inc
    │       ├── enum_configure.h
    │       ├── enum_errcode.h
    │       ├── enum_types.h
    │       ├── int128.h
    │       ├── version.h
    │       └── wasmedge.h
    └── lib
        └── libwasmedge.dylib

    4 directories, 10 files
    ```

  - By specifying `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR`.

    If you choose to use [install.sh](https://github.com/WasmEdge/WasmEdge/blob/master/utils/install.sh) to install WasmEdge Runtime on your local system, please use `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR` to specify the paths to the `include` and `lib` directories respectively.
    For example, use the following commands to specify the paths after using `bash install.sh --path=$HOME/wasmedge-install` to install WasmEdge Runtime on Ubuntu 20.04:

    ```bash
    export WASMEDGE_INCLUDE_DIR=$HOME/wasmedge-install/include 
    export WASMEDGE_LIB_DIR=$HOME/wasmedge-install/lib
    ```

  - By specifying `WASMEDGE_BUILD_DIR`

    You can choose this way if you'd like to use the latest code in the `master` branch of the `WasmEdge` github repo. For example,

    - Suppose that you `git clone` WasmEdge repo in your local directory, for example, `~/workspace/me/WasmEdge`, and follow the [instructions to build](../contribute/build_from_src/linux.md) WasmEdge native library. After that, you should find the generated `include` and `lib` directories in `~/workspace/me/WasmEdge/build`.

    - Then, set `WASMEDGE_BUILD_DIR` environment variable to specify the `build` directory.

      ```bash
      root@0a877562f39e:~/workspace/me/WasmEdge# export WASMEDGE_BUILD_DIR=/root/workspace/me/WasmEdge/build
      ```

- Second, after deploy the `WasmEdge` library on your local system, copy/paste the following code into the `Cargo.toml` file of your project. Now, you can use `cargo build` command to build your project.

```toml
[dependencies]
wasmedge-sdk = "0.4"
```

## `wasmedge-sys` crate

`wasmedge-sys` serves as a wrapper layer of `WasmEdge` C-API, and provides a group of safe low-level Rust interfaces.
For those who are interested in using `wasmedge-sys` in their projects, you should also deploy the `WasmEdge` library on your local system as described in the [wasmedge-sdk crate](#build) section.
Then, copy/paste the following code in the `Cargo.toml` file of your project.
For details, please refer to [README](https://github.com/WasmEdge/WasmEdge/blob/master/bindings/rust/wasmedge-sys/README.md).

```toml
[dependencies]
wasmedge-sys = "0.9"
```

## Enable WasmEdge Plugins

If you'd like to enable WasmEdge Plugins (currently, only available on Linux platform), please use `WASMEDGE_PLUGIN_PATH` environment variable to specify the path to the directory containing the plugins. For example, use the following commands to specify the path on Ubuntu 20.04:

```bash
export WASMEDGE_PLUGIN_PATH=$HOME/.wasmedge/lib/wasmedge
```

## Docker image

For those who would like to dev in Docker environment, you can reference the [Use Docker](../quick_start/use_docker.md) section of this book, which details how to use Docker for `WasmEdge` application development.

## Examples

For helping you get familiar with WasmEdge Rust bindings, the following quick examples demonstrate how to use the APIs defined in `wasmedge-sdk` and `wasmedge-sys`, respectively. In addition, we'll add more examples continuously. Please file issues [here](https://github.com/WasmEdge/WasmEdge/issues) and let us know if you have any problems with the API usage.

### `wasmedge-sdk` Examples

- [[wasmedge-sdk] Hello World!](rust/say_hello.md)
- [[wasmedge-sdk] Memory manipulation](rust/memory_manipulation.md)
- [[wasmedge-sdk] Table and FuncRef](rust/table_and_funcref.md)

- [More examples](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust/wasmedge-sdk/examples)

### `wasmedge-sys` Examples

- [[wasmedge-sys] Run a WebAssembly function with WasmEdge low-level APIs](rust/sys_run_host_func.md)
- [[wasmedge-sys] Compute Fibonacci numbers concurrently](rust/concurrent_fib.md)
- [[wasmedge-sys] The usage of WasmEdge module instances](rust/how_to_use_module_instance.md)

- [More examples](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust/wasmedge-sys/examples)