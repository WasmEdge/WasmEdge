# WasmEdge Rust Bindings

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

The `wasmedge-sdk` crate defines a group of high-level Rust APIs, which are used to build up business applications.

<p align = "left">
    <strong>
        <a href="https://github.com/WasmEdge/WasmEdge/blob/master/bindings/rust/wasmedge-sdk/README.md">README</a> | <a href="https://wasmedge.github.io/WasmEdge/wasmedge_sdk/">API Documentation</a>
    </strong>
</p>
<p align="left">
    <a href="https://crates.io/crates/wasmedge-sdk">
        <img src="https://img.shields.io/crates/v/wasmedge-sdk.svg">
    </a>
</p>

## wasmedge-sys

The `wasmedge-sys` crate defines a group of low-level Rust APIs for WasmEdge, a light-weight, high-performance, and extensible WebAssembly runtime for cloud-native, edge, and decentralized applications.

<p align = "left">
    <strong>
        <a href="https://github.com/WasmEdge/WasmEdge/blob/master/bindings/rust/wasmedge-sys/README.md">README</a> | <a href="https://wasmedge.github.io/WasmEdge/wasmedge_sys/">API Documentation</a>
    </strong>
</p>
<p align="left">
    <a href="https://crates.io/crates/wasmedge-sys">
        <img src="https://img.shields.io/crates/v/wasmedge-sys.svg">
    </a>
</p>

## wasmedge-types

The `wasmedge-types` crate defines a group of common data structures used by both [wasmedge-rs](https://crates.io/crates/wasmedge-sdk) and [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crates.

<p align = "left">
    <strong>
        <a href="https://github.com/WasmEdge/WasmEdge/blob/master/bindings/rust/wasmedge-types/README.md">README</a> | <a href="https://wasmedge.github.io/WasmEdge/wasmedge_types/">API Documentation</a>
    </strong>
</p>
<p align="left">
    <a href="https://crates.io/crates/wasmedge-types">
        <img src="https://img.shields.io/crates/v/wasmedge-types.svg">
    </a>
</p>

## wasmedge-macro

The [wasmedge-macro](https://crates.io/crates/wasmedge-macro) crate defines a group of procedural macros used by both [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) and [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crates.

<p align = "left">
    <strong>
        <a href="https://github.com/WasmEdge/WasmEdge/blob/master/bindings/rust/wasmedge-macro/README.md">README</a> | <a href="https://wasmedge.github.io/WasmEdge/wasmedge_macro/">API Documentation</a>
    </strong>
</p>
<p align="left">
    <a href="https://crates.io/crates/wasmedge-macro">
        <img src="https://img.shields.io/crates/v/wasmedge-macro.svg">
    </a>
</p>
