#![deny(rust_2018_idioms, unreachable_pub)]


pub use wasmedge_sys as wasmedge;

pub mod module;
pub mod vm;
pub mod error;