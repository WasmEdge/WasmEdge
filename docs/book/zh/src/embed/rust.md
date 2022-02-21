# Rust SDK

你可以通过 WasmEdge Rust SDK 将 WasmEdge 嵌入到 Rust 应用程序中。

WasmEdge Rust SDK 涉及两个 Rust crate [wasmedge-sys](https://crates.io/crates/wasmedge-sys) 和 [wasmedge-rs](https://crates.io/crates/wasmedge- sdk），它们根据不同的原则和不同的目的设计。 wasmedge-sys crate 定义了一组低级 Rust API，它们简单地包装 WasmEdge C API 并提供安全的对应物，而 wasmedge-rs crate 提供更优雅和符合人体工程学的 API，更适合应用程序开发。

* [wasmedge-sys](https://crates.io/crates/wasmedge-sys) 定义了一组低级 Rust API，它们简单地包装 WasmEdge C API 并提供安全的对应物。 [wasmedge-sys](https://crates.io/crates/wasmedge-sys) 中的 API 应该用于构建高级库。

* [wasmedge-rs](https://crates.io/crates/wasmedge-sdk) 基于 wasmedge-sys crate，并提供更优雅和惯用的 Rust API，更适合面向业务的设计和开发。 wasmedge-rs crate 仍在积极开发中。

## 构建 `wasmedge-sys`

`wasmedge-sys` 依赖于 `WasmEdge` 的动态库和头文件。对于 `cargo build` `wasmedge-sys` 有以下选择来指定依赖项：

* 通过指定 `WASMEDGE_INCLUDE_DIR` 和 `WASMEDGE_LIB_DIR`

  * 假设在 [WasmEdge Releases](https://github.com/WasmEdge/WasmEdge/releases) 下载 `Wasmedge-0.9.1` 二进制包并放在 `~/workspace/me/`目录中，发布包的目录结构如下所示：

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

  * 设置 `WASMEDGE_INCLUDE_DIR` 和 `WASMEDGE_LIB_DIR` 环境变量以指定 `include` 和 `lib`（或 `lib64`）目录，进入 `wasmedge-sys` 目录和`cargo build` 构建。

    ```bash
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_INCLUDE_DIR=/root/workspace/me/WasmEdge-0.9.1-Linux/include/wasmedge

    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_LIB_DIR=/root/workspace/me/WasmEdge-0.9.1-Linux/lib64
    ```

* 通过指定 `WASMEDGE_BUILD_DIR`

  * 假设在 `~/workspace/me/WasmEdge` 中 `git clone` WasmEdge repo，并按照 [instructions](https://wasmedge.org/book/en/extend/build.html) 构建 WasmEdge native library 生成 `include` 和 `lib` 目录应该在 `~/workspace/me/WasmEdge/build` 中。

  * 设置 `WASMEDGE_BUILD_DIR` 环境变量并指定 `build` 目录，进入 `wasmedge-sys` 目录和`cargo build` 构建。


      ```bash
      root@0a877562f39e:~/workspace/me/WasmEdge# export WASMEDGE_BUILD_DIR=/root/workspace/me/WasmEdge/build
      ```

* 通过 `standalone` 模式

  上述提到的两种方法之外，`standalone` 模式可以在构建 crate 之前直接构建 `WasmEdge` 原生库。

  * 假设 `~/workspace/me/WasmEdge` 中有 `git clone` WasmEdge repo，在`wasmedge-sys` 目录中按照以下说明进行操作：

    ```bash
    // set WASMEDGE_DIR
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# export WASMEDGE_DIR=/root/workspace/me/WasmEdge

    // cargo build with standalone feature
    root@0a877562f39e:~/workspace/me/WasmEdge/bindings/rust/wasmedge-sys# cargo build --features standalone
    ```

* 通过 WasmEdge docker image

  如果选择 WasmEdge docker image 来构建容器进行开发，则预构建 WasmEdge 二进制包默认位于 `$HOME/.wasmedge` 目录中，`wasmedge-sys` 的构建脚本 (`build.rs`) 可以检测并自动构建 crate。

## 案例

* [使用 WasmEdge 低级别 API 运行 WebAssembly 函数](rust/wasmedge-sys-api.md)
