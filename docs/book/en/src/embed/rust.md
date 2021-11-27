# Rust SDK

You can also embed WasmEdge into your Rust application via the WasmEdge Rust SDK.

The WasmEdge Rust SDK inlcudes two Rust crates, wasmedge-sys and wasmege-rs, for the Rust API. 

* The [wasmedge-sys](https://crates.io/crates/wasmedge-sys) is a low-level API generated from the WasmEdge C API. 
* The [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) is an idiomatic Rust API wrapped around the low-level wasmedge-sys to make it safer and more developer-friendly. The full wasmedge-sdk crate is still under active development. Check out [the source code](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust) on GitHub. Feedbacks and contributions are welcome.

