#![doc(
    html_logo_url = "https://github.com/cncf/artwork/blob/master/projects/wasm-edge-runtime/icon/color/wasm-edge-runtime-icon-color.png?raw=true",
    html_favicon_url = "https://raw.githubusercontent.com/cncf/artwork/49169bdbc88a7ce3c4a722c641cc2d548bd5c340/projects/wasm-edge-runtime/icon/color/wasm-edge-runtime-icon-color.svg"
)]
// If the version of rust used is less than v1.63, please uncomment the follow attribute.
// #![feature(explicit_generic_args_with_impl_trait)]
#![allow(clippy::vec_init_then_push)]

//! # Overview
//!
//! The [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) crate defines a group of high-level Rust APIs, which are used to build up business applications.
//!
//! Notice that
//! - `WasmEdge Rust SDK` uses nightly version of Rust.
//!
//! - Due to [issue #1527](https://github.com/WasmEdge/WasmEdge/issues/1527), `WasmEdge Rust SDK` cannot build successfully on Windows platform. Please [use Docker](https://wasmedge.org/book/en/start/docker.html) to build `WasmEdge Rust SDK` on Windows.
//!
//! ## Usage
//!
//! To use or build the `wasmedge-sdk` crate, the `WasmEdge` library is required. The required header files, library and plugins should be placed in `$HOME/.wasmedge/` directory. The directory structure on `Ubuntu 20.04` looks like below:
//!
//! ```bash
//! // $HOME/.wasmedge/
//! .
//! ├── bin
//! │   ├── wasmedge
//! │   └── wasmedgec
//! ├── include
//! │   └── wasmedge
//! │       ├── dense_enum_map.h
//! │       ├── enum.inc
//! │       ├── enum_configure.h
//! │       ├── enum_errcode.h
//! │       ├── enum_types.h
//! │       ├── int128.h
//! │       ├── spare_enum_map.h
//! │       ├── version.h
//! │       └── wasmedge.h
//! └── lib64
//!     ├── libwasmedge_c.so
//!     └── wasmedge
//!         └── libwasmedgePluginWasmEdgeProcess.so
//!
//! 5 directories, 13 files
//!
//! ```
//!
//! ## A quick-start example
//!
//! The example below is using `wasmedge-sdk` to run a WebAssembly module written with its WAT format (textual format):
//!
//!  ```rust
//!  // If the version of rust used is less than v1.63, please uncomment the follow attribute.
//!  // #![feature(explicit_generic_args_with_impl_trait)]
//!
//!  use wasmedge_sdk::{Executor, FuncTypeBuilder, ImportObjectBuilder, Module, Store};
//!  use wasmedge_sys::WasmValue;
//!  use wasmedge_types::wat2wasm;
//!  
//!  #[cfg_attr(test, test)]
//!  fn main() -> anyhow::Result<()> {
//!      let wasm_bytes = wat2wasm(
//!          br#"
//!  (module
//!    ;; First we define a type with no parameters and no results.
//!    (type $no_args_no_rets_t (func (param) (result)))
//!  
//!    ;; Then we declare that we want to import a function named "env" "say_hello" with
//!    ;; that type signature.
//!    (import "env" "say_hello" (func $say_hello (type $no_args_no_rets_t)))
//!  
//!    ;; Finally we create an entrypoint that calls our imported function.
//!    (func $run (type $no_args_no_rets_t)
//!      (call $say_hello))
//!    ;; And mark it as an exported function named "run".
//!    (export "run" (func $run)))
//!  "#,
//!      )?;
//!  
//!      // We define a function to act as our "env" "say_hello" function imported in the
//!      // Wasm program above.
//!      fn say_hello_world(_inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> {
//!          println!("Hello, world!");
//!  
//!          Ok(vec![])
//!      }
//!  
//!      // create an import module
//!      let import = ImportObjectBuilder::new()
//!          .with_func::<(), ()>("say_hello", Box::new(say_hello_world))?
//!          .build("env")?;
//!  
//!      // loads a wasm module from the given in-memory bytes
//!      let module = Module::from_bytes(None, &wasm_bytes)?;
//!  
//!      // create an executor
//!      let mut executor = Executor::new(None, None)?;
//!  
//!      // create a store
//!      let mut store = Store::new()?;
//!  
//!      // register the module into the store
//!      store.register_import_module(&mut executor, &import)?;
//!      let extern_instance = store.register_named_module(&mut executor, "extern", &module)?;
//!  
//!      // get the exported function "run"
//!      let run = extern_instance.func("run").ok_or(anyhow::Error::msg(
//!          "Not found exported function named 'run'.",
//!      ))?;
//!  
//!      // run host function
//!      run.call(&mut executor, [])?;
//!  
//!      Ok(())
//!  }
//!
//!   ```
//!   [[Click for more examples]](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust/wasmedge-sdk/examples)
//!
//! ## See also
//!
//! * [WasmEdge Runtime](https://wasmedge.org/)
//! * [WasmEdge C API Documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md)
//! * [wasmedge-sys: WasmEdge Low-level Rust APIs](https://crates.io/crates/wasmedge-sys)
//! * [wasmedge-types: WasmEdge Types](https://crates.io/crates/wasmedge-types)
//!

use wasmedge_types::WasmEdgeResult;

#[doc(hidden)]
#[cfg(feature = "aot")]
mod compiler;
pub mod config;
mod executor;
mod externals;
mod import;
mod instance;
#[doc(hidden)]
pub mod io;
mod module;
mod statistics;
mod store;
pub mod types;
#[doc(hidden)]
pub mod vm;

#[doc(inline)]
#[cfg(feature = "aot")]
pub use compiler::Compiler;
#[doc(inline)]
pub use executor::Executor;
#[doc(inline)]
pub use externals::{Func, FuncRef, FuncTypeBuilder, Global, Memory, Table};
#[doc(inline)]
pub use import::{ImportObject, ImportObjectBuilder};
pub use instance::{Instance, WasiInstance, WasmEdgeProcessInstance};
#[doc(inline)]
pub use io::{WasmVal, WasmValType, WasmValTypeList};
#[doc(inline)]
pub use module::{ExportType, ImportType, Module};
#[doc(inline)]
pub use statistics::Statistics;
#[doc(inline)]
pub use store::Store;
#[doc(inline)]
pub use vm::Vm;

pub type WasmValue = wasmedge_sys::types::WasmValue;

/// The object that is used to perform a [host function](crate::Func) is required to implement this trait.
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
        func: &Func,
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

/// The version info of WasmEdge core
pub struct CoreVersion {}
impl CoreVersion {
    /// Returns the major version value of WasmEdge core.
    pub fn major() -> u32 {
        wasmedge_sys::utils::version_major_value()
    }

    /// Returns the minor version value of WasmEdge core.
    pub fn minor() -> u32 {
        wasmedge_sys::utils::version_minor_value()
    }

    /// Returns the patch version value of WasmEdge core.
    pub fn patch() -> u32 {
        wasmedge_sys::utils::version_patch_value()
    }

    /// Returns the version string of WasmEdge core.
    pub fn version_string() -> String {
        wasmedge_sys::utils::version_string()
    }
}
