#![deny(rust_2018_idioms, unreachable_pub)]

pub use wasmedge_sys as wasmedge;

pub mod config;
pub mod error;
pub mod module;
pub mod vm;
pub mod wasi_conf;

pub use config::Config;
pub use module::Module;
pub use vm::Vm;
