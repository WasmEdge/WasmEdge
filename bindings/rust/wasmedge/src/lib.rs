//! # Overview
//!
//! The [wasmedge-rs](https://crates.io/crates/wasmedge-sdk) crate defines a group of high-level Rust APIs, which are used to build up business applications.
//!
//! See also
//!
//! * [wasmedge-sys: WasmEdge Low-level Rust APIs](https://crates.io/crates/wasmedge-sys)
//! * [WasmEdge Runtime](https://wasmedge.org/)
//! * [WasmEdge C API Documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md)

#[doc(hidden)]
#[cfg(feature = "aot")]
pub mod compiler;
pub mod config;
#[doc(hidden)]
pub mod error;
#[doc(hidden)]
pub mod executor;
#[doc(hidden)]
pub mod instance;
#[doc(hidden)]
pub mod module;
#[doc(hidden)]
pub mod statistics;
#[doc(hidden)]
pub mod store;
pub mod types;
// #[doc(hidden)]
// pub mod vm;

#[doc(inline)]
#[cfg(feature = "aot")]
pub use compiler::Compiler;
#[doc(inline)]
pub use executor::Executor;
#[doc(inline)]
pub use instance::{
    Func, FuncRef, FuncTypeBuilder, Global, ImportObject, ImportObjectBuilder, Instance, Memory,
    Table,
};
#[doc(inline)]
pub use module::{ExportType, ImportType, Module};
#[doc(inline)]
pub use statistics::Statistics;
#[doc(inline)]
pub use store::Store;
// #[doc(hidden)]
// pub use vm::Vm;
#[doc(inline)]
pub use error::Result;
#[doc(hidden)]
pub use wasmedge_sys::types::*;

// #[doc(hidden)]
// pub trait Engine {
//     fn register_wasm_from_module(&mut self);
//     fn register_wasm_from_import(&mut self, import: &mut ImportModule);
//     fn run_func(&self);
// }

/// Alias type for host function
pub type HostFunc = wasmedge_sys::HostFunc;

/// Parses in-memory bytes as either the [WebAssembly Text format](http://webassembly.github.io/spec/core/text/index.html), or a binary WebAssembly module.
pub use wasmedge_types::wat2wasm;

/// The object that is used to perform a [host function](crate::Func) is required to implement this trait.
pub trait Engine {
    /// Runs a host function instance and returns the results.
    ///
    /// # Arguments
    ///
    /// * `func` - The function instance to run.
    ///
    /// * `params` - The arguments to pass to the function.
    ///
    /// # Erros
    ///
    /// If fail to run the host function, then an error is returned.
    fn run_func(
        &mut self,
        func: &Func,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> Result<Vec<WasmValue>>;

    /// Runs a host function instance by calling its reference and returns the results.
    ///
    /// # Arguments
    ///
    /// * `func_ref` - A reference to the target host function instance.
    ///
    /// * `params` - The arguments to pass to the function.
    ///
    /// # Erros
    ///
    /// If fail to run the host function, then an error is returned.
    fn run_func_ref(
        &mut self,
        func_ref: &FuncRef,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> Result<Vec<WasmValue>>;
}
