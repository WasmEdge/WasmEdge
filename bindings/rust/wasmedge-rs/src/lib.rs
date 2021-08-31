//! Idiomatic Rust bindings to the [WasmEdge](https://github.com/WasmEdge/WasmEdge) VM.

#![deny(rust_2018_idioms, unreachable_pub)]

pub mod config;
pub mod module;
pub mod string;
pub mod value;
pub mod vm;

pub use config::Config;
pub use module::Module;
pub use vm::Vm;
