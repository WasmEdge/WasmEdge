# Rust SDK

你可以通过 WasmEdge Rust SDK 将 WasmEdge 嵌入到 Rust 应用程序中。

WasmEdge Rust SDK 的定义涉及两个 Rust crate， [wasmedge-sys](https://crates.io/crates/wasmedge-sys) 和 [wasmedge-rs](https://crates.io/crates/wasmedge-sdk)。 它们根据不同的原则和目的设计。 wasmedge-sys crate 定义了一组低级 Rust API，它们简单地包装 WasmEdge C API 并提供安全的对应物，而 wasmedge-rs crate 提供更优雅和符合人体工程学的 API，更适合应用程序开发。

* [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crate 定义了一组低级 Rust API，它们简单地包装 WasmEdge C API 并提供安全的对应物。 [wasmedge-sys](https://crates.io/crates/wasmedge-sys) 中的 API 应该用于构建高级库。

* [wasmedge-rs](https://crates.io/crates/wasmedge-sdk) crate 基于 wasmedge-sys crate，并提供更优雅和惯用的 Rust API。 这些 API 更适合面向业务的设计和开发。 wasmedge-rs crate 仍在积极开发中。 查看 GitHub 上的 [源代码](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust)。 欢迎反馈和贡献。

## 案例

* [使用 WasmEdge 低级 API 运行 WebAssembly 函数](rust/wasmedge-sys-api.md)
