mod function;
mod global;
mod import;
mod memory;
mod module;
mod table;

pub use function::{Func, FuncRef, FuncTypeBuilder};
pub use global::Global;
// pub use import::{ImportModule, ImportModuleBuilder};
pub use memory::Memory;
pub use module::Instance;
pub use table::Table;
