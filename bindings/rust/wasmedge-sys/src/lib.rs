#![deny(rust_2018_idioms, unreachable_pub)]

use std::{cell::RefCell, collections::HashMap};

#[allow(warnings)]
pub mod wasmedge {
    include!(concat!(env!("OUT_DIR"), "/wasmedge.rs"));
}

pub mod config;
pub mod import_obj;
pub mod instance;
pub mod io;
pub mod module;
pub mod raw_result;
pub mod string;
pub mod value;
pub mod version;
pub mod vm;
pub mod wasi;

pub use config::{Config, OptLevel};
pub use import_obj::ImportObj;
pub(crate) use io::WasmFnIO;
pub use io::{I1, I2};
pub use module::Module;
pub use raw_result::ErrReport;
pub use string::{StringBuf, StringRef, WasmEdgeString};
pub use value::Value;
pub use version::{full_version, semv_version};
pub use vm::Vm;

thread_local! {
    // TODO: allow modify capacity before running
    #[allow(clippy::type_complexity)]
    static HOST_FUNCS:
      RefCell<
        HashMap<usize, Box<dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8>>>> = RefCell::new(HashMap::with_capacity(500));
}
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn links() {
        unsafe {
            assert!(
                wasmedge::WasmEdge_VersionGetMajor()
                    + wasmedge::WasmEdge_VersionGetMinor()
                    + wasmedge::WasmEdge_VersionGetPatch()
                    != 0
            );
        }
    }
}
