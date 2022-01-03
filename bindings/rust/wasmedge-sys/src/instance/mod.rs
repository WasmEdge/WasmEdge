//! Defines WasmEdge instance structs, including Function, Global, Memory, and Table.

pub mod function;
pub mod global;
pub mod memory;
pub mod table;

#[doc(inline)]
pub use function::{FuncType, Function};
#[doc(inline)]
pub use global::{Global, GlobalType};
#[doc(inline)]
pub use memory::{MemType, Memory};
#[doc(inline)]
pub use table::{Table, TableType};
