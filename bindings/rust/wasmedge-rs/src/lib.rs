#![deny(rust_2018_idioms, unreachable_pub)]

pub use wasmedge_sys as wasmedge;

pub mod config;
pub mod error;
pub mod module;
pub mod vm;

pub use config::Config;
pub use vm::Vm;
