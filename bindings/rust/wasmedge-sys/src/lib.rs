//! # Rust WasmEdge bindings
//! This library contains safe Rust bindings for WasmEdge, a lightweight, high-performance, and extensible WebAssembly runtime for cloud native, edge, and decentralized applications.
//!
//! WasmEdge 0.9.0 is the lowest supported version for the underlying library.
//!
//! Most of this documentation is generated from the C API. Until all parts of the documentation have been reviewed there will be incongruities with the actual Rust API.
//!
//! See also
//! - [WasmEdge Runtime](https://wasmedge.org/)
//! - [WasmEdge C API Documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md)

#![deny(rust_2018_idioms, unreachable_pub)]

use std::{cell::RefCell, collections::HashMap, env::var};

#[doc(hidden)]
#[allow(warnings)]
pub mod wasmedge {
    include!(concat!(env!("OUT_DIR"), "/wasmedge.rs"));
}
#[doc(hidden)]
pub mod compiler;
#[doc(hidden)]
pub mod config;
#[doc(hidden)]
pub mod error;
#[doc(hidden)]
pub mod executor;
#[doc(hidden)]
pub mod import_obj;
// #[doc(hidden)]
pub mod instance;
#[doc(hidden)]
pub mod io;
#[doc(hidden)]
pub mod loader;
#[doc(hidden)]
pub mod module;
#[doc(hidden)]
pub mod statistics;
#[doc(hidden)]
pub mod store;
#[doc(hidden)]
pub mod string;
#[doc(hidden)]
pub mod types;
#[doc(hidden)]
pub mod utils;
#[doc(hidden)]
pub mod validator;
#[doc(hidden)]
pub mod version;
#[doc(hidden)]
pub mod vm;

#[doc(inline)]
pub use compiler::Compiler;
#[doc(inline)]
pub use config::Config;
#[doc(inline)]
pub use error::{Error, WasmEdgeError, WasmEdgeResult};
#[doc(inline)]
pub use executor::Executor;
#[doc(inline)]
pub use import_obj::ImportObj;
#[doc(inline)]
pub use instance::{
    function::{FuncType, Function},
    global::{Global, GlobalType},
};
#[doc(inline)]
pub(crate) use io::WasmFnIO;
#[doc(inline)]
pub use io::{I1, I2};
#[doc(inline)]
pub use loader::Loader;
#[doc(inline)]
pub use module::Module;
#[doc(inline)]
pub use statistics::Statistics;
#[doc(inline)]
pub use store::Store;
#[doc(inline)]
pub use string::{StringBuf, StringRef, WasmEdgeString};
#[doc(inline)]
pub use types::{Mutability, ValType, Value};
#[doc(inline)]
pub use validator::Validator;
#[doc(inline)]
pub use version::{full_version, semv_version};
#[doc(inline)]
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
