
# WasmEdge Rust SDK

- [WasmEdge Rust SDK](#wasmedge-rust-sdk)
  - [Introduction](#introduction)
  - [`wasmedge-sdk` crate](#wasmedge-sdk-crate)
    - [Usage](#usage)
      - [Embedding](#embedding)
      - [Build](#build)
  - [`wasmedge-sys` crate](#wasmedge-sys-crate)
    - [Build in `non-standalone` mode](#build-in-non-standalone-mode)
  - [Docker image](#docker-image)
  - [Examples](#examples)
    - [`wasmedge-sdk` Examples](#wasmedge-sdk-examples)
    - [`wasmedge-sys` Examples](#wasmedge-sys-examples)

## Introduction

`WasmEdge` supports embedding into Rust applications via WasmEdge Rust SDK. WasmEdge Rust SDK consists of three crates:

- [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) crate. It defines a group of safe, ergonomic high-level APIs, which are used to build up business applications.
  - [API documentation](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/)

- [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crate. It defines a group of low-level Rust APIs, which simply wrap WasmEdge C-API and provide the safe counterparts. It is not recommended to use it directly to build up application.
  - [API documentation](https://wasmedge.github.io/WasmEdge/wasmedge_sys/)

- [wasmedge-types](https://crates.io/crates/wasmedge-types) crate. The data structures that are commonly used in `wasmedge-sdk` and `wasmedge-sys` are defined in this crate.
  - [API documentation](https://wasmedge.github.io/WasmEdge/wasmedge_types/)

## `wasmedge-sdk` crate

### Usage

#### Embedding

To use `wasmedge-sdk` in your Rust crate, simple add it to your `Cargo.toml`:

```toml
[dependencies]
wasmedge-sdk = "0.1.0"
```

N.B. that since the `WasmEdge` library is a dependency of WasmEdge Rust SDK, it is required that `WasmEdge` is deployed first on your target system. The required header files, library and plugins should be placed in `$HOME/.wasmedge/` directory. The directory structure on Linux looks like below:

```bash
// $HOME/.wasmedge/
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

In the future version, WasmEdge Rust SDK will support the `standalone` mode that supports automatic deployment of the `WasmEdge` library.

#### Build

For those who are interesting in building `wasmedge-sdk` from source, as mentioned before, the `WasmEdge` library should be deployed on your target system before you can build the `wasmedge-sdk` crate.

## `wasmedge-sys` crate

`wasmedge-sys` serves as a wraper layer of `WasmEdge` C-API, and provides a group of safe low-level Rust interfaces. For those who are interested in using `wasmedge-sys` in their projects, two modes are avaible: one is `standalone` mode, the other `non-standalone`. The `standalone` mode is recommended for most of the projects. To enable the `standalone` mode, simply put the following lines in your `Cargo.toml`:

```toml
[dependencies]
wasmedge-sys = [version = "0.7.0", features = ["standalone"]]
```

For those who want to build `wasmedge-sys` from source in terminal, just go to the `wasmedge-sys` directory and type `cargo build --features standalone` command that will compile the `wasmedge-sys` crate in `standalone` mode.

### Build in `non-standalone` mode

For those who would like to use `wasmedge-sys` in the `non-standalone` mode, you should first guarantee that the `WasmEdge` binary is downloaded and deployed in your local environment. The directory structure on `Ubuntu-20.04` looks like below:

```bash
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

With the required `WasmEdge` library, you can then choose one of the following ways to build `wasmedge-sys`:

- By specifying `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR`

  N.B. that it is strongly recommended to use this way if you'd like to use `wasmedge-sys` crate in your Rust project, but not want to enable the `standalone` feature.

  - Suppose that you have already downloaded the `Wasmedge-0.10.0` binary package from [WasmEdge Releases](https://github.com/WasmEdge/WasmEdge/releases/tag/0.10.0) to a local directory, for example, `~/workspace/me/`. The directory structure of the released package should looks like below:

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

  - Use `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR` environment variables to specify the `include` and `lib` (or `lib64`) directories. After that, you can run `cargo build` command in terminal to build `wasmedge-sys`.

    ```bash
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_INCLUDE_DIR=/root/workspace/me/WasmEdge-0.10.0-Linux/include/wasmedge
    
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_LIB_DIR=/root/workspace/me/WasmEdge-0.10.0-Linux/lib64
    ```

- By specifying `WASMEDGE_BUILD_DIR`

  N.B. that it is strongly recommended to use this way if you'd like to build the `WasmEdge` binary from the master branch in person.

  - Suppose that you `git clone` WasmEdge repo in your local directory, for example, `~/workspace/me/WasmEdge`, and follow the [instructions](https://wasmedge.org/book/en/extend/build.html) to build WasmEdge native library. After that, you should find the generated `include` and `lib` directories in `~/workspace/me/WasmEdge/build`.

  - Then, use `WASMEDGE_BUILD_DIR` environment variable to specify the `build` directory. After that, you can run `cargo build` command in terminal to build `wasmedge-sys`.

    ```bash
    root@0a877562f39e:~/workspace/me/WasmEdge# export WASMEDGE_BUILD_DIR=/root/workspace/me/WasmEdge/build
    ```

- By default location

  For those who do not want to define environment variables, you can put the downloaded `WasmEdge` binary package in the default location `$HOME/.wasmedge/`. The directory structure of the default location should looks like below (for example, on `Ubuntu-20.04`):

  ```bash
  // $HOME/.wasmedge/
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
  
## Docker image

For those who would like to dev in Docker environment, you can reference the [Use Docker](/src/start/docker.md) section of this book, which details how to use Docker for `WasmEdge` application development.

## Examples

### `wasmedge-sdk` Examples

- [[wasmedge-sdk] Hello World!](rust/say_hello.md)

- [[wasmedge-sdk] Memory manipulation](rust/memory_manipulation.md)

### `wasmedge-sys` Examples

- [[wasmedge-sys] Run a WebAssembly function with WasmEdge low-level APIs](rust/sys_run_host_func.md)

- [[wasmedge-sys] Compute Fibonacci numbers concurrently](rust/concurrent_fib.md)

- [[wasmedge-sys] The usage of WasmEdge module instances](rust/how_to_use_module_instance.md)
