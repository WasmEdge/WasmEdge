# WasmEdge Rust Bindings

** Rust Binding and related projects are moved to [wasmedge-rust-sdk](https://github.com/WasmEdge/wasmedge-rust-sdk) **
** Please refer to the new repository for the further updates **

WasmEdge Rust bindings consist of the following crates. They together provide different levels of APIs for Rust developers to use WasmEdge runtime. For example, `wasmedge-sdk` defines the high-level APIs for application development.

## Versioning Table

The following table provides the versioning information about each crate of WasmEdge Rust bindings.

| wasmedge-sdk  | WasmEdge lib  | wasmedge-sys  | wasmedge-types| wasmedge-macro|
| :-----------: | :-----------: | :-----------: | :-----------: | :-----------: |
| 0.7.1         | 0.11.2        | 0.12.2        | 0.3.1         | 0.3.0         |
| 0.7.0         | 0.11.2        | 0.12          | 0.3.1         | 0.3.0         |
| 0.6.0         | 0.11.2        | 0.11          | 0.3.0         | 0.2.0         |
| 0.5.0         | 0.11.1        | 0.10          | 0.3.0         | 0.1.0         |
| 0.4.0         | 0.11.0        | 0.9           | 0.2.1         | -             |
| 0.3.0         | 0.10.1        | 0.8           | 0.2           | -             |
| 0.1.0         | 0.10.0        | 0.7           | 0.1           | -             |

## wasmedge-sdk

** `wasmedge-sdk` is moved to [wasmedge-rust-sdk](https://github.com/WasmEdge/wasmedge-rust-sdk)
The `wasmedge-sdk` crate defines a group of high-level Rust APIs, which are used to build up business applications.


## wasmedge-sys

** `wasmedge-sys` is under [wasmedge-rust-sdk/crates/wasmedge-sys](https://github.com/WasmEdge/wasmedge-rust-sdk/tree/main/crates/wasmedge-sys) now. **

The `wasmedge-sys` crate defines a group of low-level Rust APIs for WasmEdge, a light-weight, high-performance, and extensible WebAssembly runtime for cloud-native, edge, and decentralized applications.

## wasmedge-types

** `wasmedge-types` is under [wasmedge-rust-sdk/crates/wasmedge-types](https://github.com/WasmEdge/wasmedge-rust-sdk/tree/main/crates/wasmedge-types) now. **

The `wasmedge-types` crate defines a group of common data structures used by both [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) and [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crates.

## wasmedge-macro

** `wasmedge-mcaro` is under [wasmedge-rust-sdk/crates/wasmedge-mcaro](https://github.com/WasmEdge/wasmedge-rust-sdk/tree/main/crates/wasmedge-mcaro) now. **

The [wasmedge-macro](https://crates.io/crates/wasmedge-macro) crate defines a group of procedural macros used by both [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) and [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crates.
