# Rust SDK

You can also embed WasmEdge into your Rust application via the WasmEdge Rust SDK.

The definitions of WasmEdge Rust SDK involve two Rust crates, [wasmedge-sys](https://crates.io/crates/wasmedge-sys) and [wasmedge-rs](https://crates.io/crates/wasmedge-sdk). They are designed based on different principles and for different purposes. The wasmedge-sys crate defines a group of low-level Rust APIs, which simply wrap WasmEdge C APIs and provide the safe counterpart, while the wasmedge-rs crate provides more elegant and ergonomic APIs, which are more suitable for application development.

* The [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crate defines a group of low-level Rust APIs, which simply wrap WasmEdge C APIs and provide the safe counterparts. The APIs in [wasmedge-sys](https://crates.io/crates/wasmedge-sys) should be used to construct high-level libraries.

* The [wasmedge-rs](https://crates.io/crates/wasmedge-sdk) crate is based on the wasmedge-sys crate and provides a more elegant and idiomatic Rust APIs. These APIs are more suitable for business-oriented design and development. The wasmedge-rs crate is still under active development. Check out [the source code](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust) on GitHub. Feedbacks and contributions are welcome.

## Examples

* [Run a WebAssembly function with WasmEdge low-level APIs](rust/wasmedge-sys-api.md)
