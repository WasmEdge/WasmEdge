use wasmedge_sys as wasmedge;

pub mod config;
pub mod error;
pub mod executor;
pub mod instance;
pub mod module;
pub mod statistics;
pub mod store;
pub mod vm;

pub use config::Config;
pub use executor::Executor;
pub use instance::{
    Func, Global, GlobalType, HostFunc, ImportMod, Instance, Memory, MemoryType, Signature,
    SignatureBuilder, Table, TableType, WasiImportMod, WasmEdgeProcessImportMod,
};
pub use module::{ExportType, ExternalType, ImportType, Module};
pub use statistics::Statistics;
pub use store::Store;
pub use vm::Vm;
pub use wasmedge_sys::types::*;

pub trait Engine {
    fn register_wasm_from_module(&mut self);
    fn register_wasm_from_import(&mut self, import: &mut ImportMod);
    fn run_func(&self);
}
