# Rust SDK

You can also embed WasmEdge into your Rust application via the WasmEdge Rust SDK.

The definitions of WasmEdge Rust SDK involve two Rust crates, [wasmedge-sys](https://crates.io/crates/wasmedge-sys) and [wasmedge-rs](https://crates.io/crates/wasmedge-sdk). They are designed based on different principles and for different purposes. The wasmedge-sys crate defines a group of low-level Rust APIs, which simply wrap WasmEdge C APIs and provide the safe counterpart, while the wasmedge-rs crate provides more elegant and ergonomic APIs, which are more suitable for application development.

* The [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crate defines a group of low-level Rust APIs, which simply wrap WasmEdge C APIs and provide the safe counterparts. The APIs in [wasmedge-sys](https://crates.io/crates/wasmedge-sys) should be used to construct high-level libraries.

* The [wasmedge-rs](https://crates.io/crates/wasmedge-sdk) crate is based on the wasmedge-sys crate and provides a more elegant and idiomatic Rust APIs. These APIs are more suitable for business-oriented design and development. The wasmedge-rs crate is still under active development and coming soon.

## Build `wasmedge-sys` crate

`wasmedge-sys` depends on the dynamic library and the header files of `WasmEdge`. To `cargo build` `wasmedge-sys` there are several choices to specify the dependencies:

* By specifying `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR`

  * Suppose that you have already downloaded the `Wasmedge-0.9.1` binary package from [WasmEdge Releases](https://github.com/WasmEdge/WasmEdge/releases) and put it in the `~/workspace/me/` directory. The directory structure of the released package is shown below.

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

  * Set `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR` environment variables to specify the `include` and `lib` (or `lib64`) directories. After that, go to the `wasmedge-sys` directory and `cargo build` the crate.

    ```bash
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_INCLUDE_DIR=/root/workspace/me/WasmEdge-0.9.1-Linux/include/wasmedge
    
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_LIB_DIR=/root/workspace/me/WasmEdge-0.9.1-Linux/lib64
    ```

* By specifying `WASMEDGE_BUILD_DIR`

  * Suppose that you `git clone` WasmEdge repo in `~/workspace/me/WasmEdge`, and follow the [instructions](https://wasmedge.org/book/en/extend/build.html) to build WasmEdge native library. The generated `include` and `lib` directories should be in `~/workspace/me/WasmEdge/build`.

  * Then, set `WASMEDGE_BUILD_DIR` environment variable to specify the `build` directory. After that, go to the `wasmedge-sys` directory and `cargo build` the crate.

    ```bash
    root@0a877562f39e:~/workspace/me/WasmEdge# export WASMEDGE_BUILD_DIR=/root/workspace/me/WasmEdge/build
    ```

* By the `standalone` mode

  Besides the two methods mentioned above, the `standalone` mode enables building `WasmEdge` native library directly before building the crate itself.

  * Suppose that you `git clone` WasmEdge repo in `~/workspace/me/WasmEdge`. Go to the `wasmedge-sys` directory, and follow the instructions shown below:

    ```bash
    // set WASMEDGE_DIR
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_DIR=/root/workspace/me/WasmEdge

    // cargo build with standalone feature
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# cargo build --features standalone
    ```

* By WasmEdge docker image

  If you choose WasmEdge docker image to build your own container for development, the pre-built WasmEdge binary package is located in `$HOME/.wasmedge` directory by default. The build script (`build.rs`) of `wasmedge-sys` crate can detect the package and build the crate automatically.

## Examples

* [Run a WebAssembly function with WasmEdge low-level APIs](rust/wasmedge-sys-api.md)
