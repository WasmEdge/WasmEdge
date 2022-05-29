mod function;
mod global;
mod memory;
mod table;

pub use function::{Func, FuncRef, FuncTypeBuilder};
pub use global::Global;
pub use memory::Memory;
pub use table::Table;
