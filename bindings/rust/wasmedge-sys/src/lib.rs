//! # Rust WasmEdge bindings
//! This library contains safe Rust bindings for WasmEdge, a lightweight, high-performance, and extensible WebAssembly runtime for cloud native, edge, and decentralized applications.
//!
//! Most of this documentation is generated from the C API. Until all parts of the documentation have been reviewed there will be incongruities with the actual Rust API.
//!
//! ## Usage
//!
//! A quick-start example below is using `wasmedge-sys` to run a WebAssembly module written with its WAT format (textual format):
//!
//! ```rust
//! use wasmedge_sys::{Vm, WasmValue};
//! use wasmedge_types::wat2wasm;
//!
//! fn main() -> Result<(), Box<dyn std::error::Error>> {
//!     // read the wasm bytes
//!     let wasm_bytes = wat2wasm(
//!         br#"
//!         (module
//!             (export "fib" (func $fib))
//!             (func $fib (param $n i32) (result i32)
//!              (if
//!               (i32.lt_s
//!                (get_local $n)
//!                (i32.const 2)
//!               )
//!               (return
//!                (i32.const 1)
//!               )
//!              )
//!              (return
//!               (i32.add
//!                (call $fib
//!                 (i32.sub
//!                  (get_local $n)
//!                  (i32.const 2)
//!                 )
//!                )
//!                (call $fib
//!                 (i32.sub
//!                  (get_local $n)
//!                  (i32.const 1)
//!                 )
//!                )
//!               )
//!              )
//!             )
//!            )
//! "#,
//!     )?;
//!
//!     // create a Vm instance
//!     let mut vm = Vm::create(None, None)?;
//!
//!     // register the wasm bytes
//!     let module_name = "extern-module";
//!     vm.register_wasm_from_bytes(module_name, &wasm_bytes)?;
//!
//!     // run the exported function named "fib"
//!     let func_name = "fib";
//!     let result = vm.run_registered_function(module_name, func_name, [WasmValue::from_i32(5)])?;
//!
//!     assert_eq!(result.len(), 1);
//!     assert_eq!(result[0].to_i32(), 8);
//!
//!     Ok(())
//! }
//! ```
//! [[Click for more examples]](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust/wasmedge-sys/examples)
//!
//! ## See also
//!
//! - [WasmEdge Runtime](https://wasmedge.org/)
//! - [WasmEdge C API Documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md)
//!

#![deny(rust_2018_idioms, unreachable_pub)]

#[macro_use]
extern crate lazy_static;

use std::{
    collections::HashMap,
    sync::{Arc, Mutex},
};

#[doc(hidden)]
#[allow(warnings)]
pub mod ffi {
    include!(concat!(env!("OUT_DIR"), "/wasmedge.rs"));
}
pub mod ast_module;
#[doc(hidden)]
#[cfg(feature = "aot")]
pub mod compiler;
#[doc(hidden)]
pub mod config;
pub mod error;
#[doc(hidden)]
pub mod executor;
pub mod instance;
#[doc(hidden)]
pub mod io;
#[doc(hidden)]
pub mod loader;
#[doc(hidden)]
pub mod statistics;
#[doc(hidden)]
pub mod store;
pub mod types;
pub mod utils;
#[doc(hidden)]
pub mod validator;
#[doc(hidden)]
pub mod vm;

#[doc(inline)]
#[cfg(feature = "aot")]
pub use compiler::Compiler;
#[doc(inline)]
pub use config::Config;
#[doc(inline)]
pub use executor::Executor;
// #[doc(inline)]
// pub use import_obj::ImportObject;
#[doc(inline)]
pub use ast_module::{ExportType, ImportType, Module};
#[doc(inline)]
pub use instance::{
    function::{FuncRef, FuncType, Function},
    global::{Global, GlobalType},
    memory::{MemType, Memory},
    module::{
        ImportInstance, ImportModule, ImportObject, Instance, WasiModule, WasmEdgeProcessModule,
    },
    table::{Table, TableType},
};
#[doc(inline)]
pub use loader::Loader;
#[doc(inline)]
pub use statistics::Statistics;
#[doc(inline)]
pub use store::Store;
#[doc(inline)]
pub use types::WasmValue;
#[doc(inline)]
pub use validator::Validator;
#[doc(inline)]
pub use vm::Vm;

/// The WasmEdge result type.
pub type WasmEdgeResult<T> = Result<T, error::WasmEdgeError>;

/// Type alias for a host function.
pub type HostFunc = Box<dyn Fn(Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> + Send + Sync>;

lazy_static! {
    static ref HOST_FUNCS: Arc<Mutex<HashMap<usize, HostFunc>>> =
        Arc::new(Mutex::new(HashMap::with_capacity(
            std::env::var("MAX_HOST_FUNC_LENGTH")
                .map(|s| s
                    .parse::<usize>()
                    .expect("MAX_HOST_FUNC_LENGTH should be a positive integer."))
                .unwrap_or(500)
        )));
}

/// The object that is used to perform a [host function](crate::Function) is required to implement this trait.
pub trait Engine {
    /// Runs a host function instance and returns the results.
    ///
    /// # Arguments
    ///
    /// * `func` - The function instance to run.
    ///
    /// * `params` - The arguments to pass to the function.
    ///
    /// # Erros
    ///
    /// If fail to run the host function, then an error is returned.
    fn run_func(
        &mut self,
        func: &Function,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>>;

    /// Runs a host function instance by calling its reference and returns the results.
    ///
    /// # Arguments
    ///
    /// * `func_ref` - A reference to the target host function instance.
    ///
    /// * `params` - The arguments to pass to the function.
    ///
    /// # Erros
    ///
    /// If fail to run the host function, then an error is returned.
    fn run_func_ref(
        &mut self,
        func_ref: &FuncRef,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>>;
}
