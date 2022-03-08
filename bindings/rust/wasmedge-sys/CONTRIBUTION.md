
# Design Principles

In general, the `*-sys` library should keep only `unsafe` C interface bindings and should not have redundant security abstractions. The interfaces exposed by the `*-sys` library are supposed to be stable. That is, when the C interface changes, only the `*-sys` library needs to be changed, not the upper-layer SDK.

The [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crate follows the design principle. `wasmedge-sys` defines a group of low-level Rust APIs, which simply wrap WasmEdge C APIs and provide the safe counterparts. The APIs in [wasmedge-sys](https://crates.io/crates/wasmedge-sys) should be used to construct high-level libraries.
