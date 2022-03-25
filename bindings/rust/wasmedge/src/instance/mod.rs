mod function;
mod global;
mod import;
mod memory;
mod module;
mod table;

pub use function::{Func, Signature, SignatureBuilder};
pub use global::{Global, GlobalType};
pub use import::{
    ImportModule, ImportModuleBuilder, WasiImportModule, WasmEdgeProcessImportModule,
};
pub use memory::{Memory, MemoryType};
pub use module::Instance;
pub use table::{Table, TableType};
