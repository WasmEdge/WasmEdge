
# WasmEdge Rust SDK

<!-- @import "[TOC]" {cmd="toc" depthFrom=1 depthTo=6 orderedList=false} -->

<!-- code_chunk_output -->

- [WasmEdge Rust SDK](#wasmedge-rust-sdk)
  - [Introduction](#introduction)
  - [`wasmedge-sdk` crate](#wasmedge-sdk-crate)
  - [`wasmedge-sys` crate](#wasmedge-sys-crate)
    - [Build](#build)
    - [Examples](#examples)

<!-- /code_chunk_output -->

## Introduction

`WasmEdge` supports embedding into Rust applications via WasmEdge Rust SDK. WasmEdge Rust SDK consists of three crates:

- [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) crate. It defines a group of safe, ergonomic high-level APIs, which are used to build up business applications.
  - [API documentation](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/)

- [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crate. It defines a group of low-level Rust APIs, which simply wrap WasmEdge C-API and provide the safe counterparts. It is not recommended to use it directly to build up application.
  - [API documentation](https://wasmedge.github.io/WasmEdge/wasmedge_sys/)

- [wasmedge-types](https://crates.io/crates/wasmedge-types) crate. The data structures that are commonly used in `wasmedge-sdk` and `wasmedge-sys` are defined in this crate.
  - [API documentation](https://wasmedge.github.io/WasmEdge/wasmedge_types/)

## `wasmedge-sdk` crate

## `wasmedge-sys` crate

### Build

`wasmedge-sys` depends on the dynamic library and the header files of `WasmEdge`. To `cargo build` `wasmedge-sys` there are several choices to specify the dependencies:

- By specifying `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR`

  - Suppose that you have already downloaded the `Wasmedge-0.9.1` binary package from [WasmEdge Releases](https://github.com/WasmEdge/WasmEdge/releases) and put it in the `~/workspace/me/` directory. The directory structure of the released package is shown below.

    ```bash
    root@0a877562f39e:~/workspace/me/WasmEdge-0.9.1-Linux# pwd
    /root/workspace/me/WasmEdge-0.9.1-Linux

    root@0a877562f39e:~/workspace/me/WasmEdge-0.9.1-Linux# tree .
    .
    |-- bin
    |   |-- wasmedge
    |   `-- wasmedgec
    |-- include
    |   `-- wasmedge
    |       |-- enum_configure.h
    |       |-- enum_errcode.h
    |       |-- enum_types.h
    |       |-- int128.h
    |       |-- version.h
    |       `-- wasmedge.h
    `-- lib64
        `-- libwasmedge_c.so

    4 directories, 9 files
    ```

  - Set `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR` environment variables to specify the `include` and `lib` (or `lib64`) directories. After that, go to the `wasmedge-sys` directory and `cargo build` the crate.

    ```bash
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_INCLUDE_DIR=/root/workspace/me/WasmEdge-0.9.1-Linux/include/wasmedge
    
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_LIB_DIR=/root/workspace/me/WasmEdge-0.9.1-Linux/lib64
    ```

- By specifying `WASMEDGE_BUILD_DIR`

  - Suppose that you `git clone` WasmEdge repo in `~/workspace/me/WasmEdge`, and follow the [instructions](https://wasmedge.org/book/en/extend/build.html) to build WasmEdge native library. The generated `include` and `lib` directories should be in `~/workspace/me/WasmEdge/build`.

  - Then, set `WASMEDGE_BUILD_DIR` environment variable to specify the `build` directory. After that, go to the `wasmedge-sys` directory and `cargo build` the crate.

    ```bash
    root@0a877562f39e:~/workspace/me/WasmEdge# export WASMEDGE_BUILD_DIR=/root/workspace/me/WasmEdge/build
    ```

- By the `standalone` mode

  Besides the two methods mentioned above, the `standalone` mode enables building `WasmEdge` native library directly before building the crate itself.

  - Suppose that you `git clone` WasmEdge repo in `~/workspace/me/WasmEdge`. Go to the `wasmedge-sys` directory, and follow the instructions shown below:

    ```bash
    // set WASMEDGE_DIR
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_DIR=/root/workspace/me/WasmEdge

    // cargo build with standalone feature
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# cargo build --features standalone
    ```

- By WasmEdge docker image

  If you choose WasmEdge docker image to build your own container for development, the pre-built WasmEdge binary package is located in `$HOME/.wasmedge` directory by default. The build script (`build.rs`) of `wasmedge-sys` crate can detect the package and build the crate automatically.

### Examples

- [[wasmedge-sys] Run a WebAssembly function with WasmEdge low-level APIs](rust/sys_run_host_func.md)

- [[wasmedge-sys] Compute Fibonacci numbers concurrently](rust/concurrent_fib.md)

- [[wasmedge-sys] The usage of WasmEdge module instances](rust/how_to_use_module_instance.md)
