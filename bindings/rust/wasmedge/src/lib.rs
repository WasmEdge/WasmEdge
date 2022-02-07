use wasmedge_sys as wasmedge;

pub mod config;
pub mod error;
pub mod instance;
pub mod statistics;
pub mod store;
pub mod vm;

pub use config::Config;
pub use instance::{
    ExportType, ExternalType, Func, Global, GlobalType, ImportMod, ImportType, Memory, MemoryType,
    Module, Signature, SignatureBuilder, Table, TableType, WasiImportMod, WasmEdgeProcessImportMod,
};
pub use statistics::Statistics;
pub use store::Store;
pub use vm::{Vm, VmBuilder};
pub use wasmedge_sys::types::*;

pub trait Engine {
    fn register_wasm_from_module(&mut self);
    fn register_wasm_from_import(&mut self, import: &mut ImportMod);
    fn run_func(&self);
}
