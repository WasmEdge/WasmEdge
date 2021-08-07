//! Idiomatic Rust bindings to the [WasmEdge](https://github.com/WasmEdge/WasmEdge) VM.

#![deny(rust_2018_idioms, unreachable_pub)]

pub mod config;
pub mod module;
pub mod string;
pub mod value;

pub use wasmedge_sys as ffi;

pub use config::Config;
pub use module::Module;

/// A WasmEdge VM instance.
#[derive(Debug)]
pub struct Vm {
    ctx: *mut ffi::WasmEdge_VMContext,
}

impl Vm {
    pub fn new(module: &Module) -> Result<Self, ()> {
        todo!()
    }
}

impl Drop for Vm {
    fn drop(&mut self) {
        unsafe { ffi::WasmEdge_VMDelete(self.ctx) };
    }
}
