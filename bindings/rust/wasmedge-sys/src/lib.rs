#![deny(rust_2018_idioms, unreachable_pub)]

use std::{cell::RefCell, collections::HashMap, env::var};

#[allow(warnings)]
pub mod wasmedge {
    include!(concat!(env!("OUT_DIR"), "/wasmedge.rs"));
}

pub mod config;
pub mod error;
pub mod executor;
pub mod import_obj;
pub mod instance;
pub mod io;
pub mod module;
pub mod statistics;
pub mod store;
pub mod string;
pub mod types;
pub mod utils;
pub mod value;
pub mod version;
pub mod vm;
pub mod wasi;

pub use config::{Config, OptLevel};
pub use error::{Error, WasmEdgeError, WasmEdgeResult};
pub use executor::Executor;
pub use import_obj::ImportObj;
pub(crate) use io::WasmFnIO;
pub use io::{I1, I2};
pub use module::Module;
pub use statistics::Statistics;
pub use store::Store;
pub use string::{StringBuf, StringRef, WasmEdgeString};
pub use value::Value;
pub use version::{full_version, semv_version};
pub use vm::Vm;

thread_local! {
    // TODO: allow modify capacity before running
    #[allow(clippy::type_complexity)]
    static HOST_FUNCS:
      RefCell<
        HashMap<usize, Box<dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8>>>> = RefCell::new(HashMap::with_capacity(var("MAX_HOST_FUNC_LENGTH").map(|s| s.parse::<usize>().expect("MAX_HOST_FUNC_LENGTH should be a number")).unwrap_or(500)));
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
