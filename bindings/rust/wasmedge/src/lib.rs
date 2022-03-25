use wasmedge_sys as sys;

#[doc(hidden)]
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
#[doc(hidden)]
pub mod vm;

#[doc(hidden)]
pub use compiler::Compiler;
#[doc(inline)]
pub use executor::Executor;
#[doc(inline)]
pub use instance::{
    Func, Global, GlobalType, HostFunc, ImportModule, ImportModuleBuilder, Instance, Memory,
    MemoryType, Signature, SignatureBuilder, Table, TableType, WasiImportModule,
    WasmEdgeProcessImportModule,
};
#[doc(inline)]
pub use module::{ExportType, ImportType, Module};
#[doc(inline)]
pub use statistics::Statistics;
#[doc(inline)]
pub use store::Store;
#[doc(inline)]
pub use vm::Vm;
#[doc(hidden)]
pub use wasmedge_sys::types::*;

pub trait Engine {
    fn register_wasm_from_module(&mut self);
    fn register_wasm_from_import(&mut self, import: &mut ImportModule);
    fn run_func(&self);
}
