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
//! * Notice that [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) requires **Rust v1.66 or above** in the **stable** channel.
//!
//! ## Build
//!
//! To use or build the `wasmedge-sdk` crate, the `WasmEdge` library is required. Please refer to [WasmEdge Installation and Uninstallation](https://wasmedge.org/book/en/quick_start/install.html) to install the `WasmEdge` library.
//!
//! * The following table provides the versioning information about each crate of WasmEdge Rust bindings.
//!
//!   | wasmedge-sdk  | WasmEdge lib  | wasmedge-sys  | wasmedge-types| wasmedge-macro|
//!   | :-----------: | :-----------: | :-----------: | :-----------: | :-----------: |
//!   | 0.8.0         | 0.12.0        | 0.13.0        | 0.4.0         | 0.3.0         |
//!   | 0.7.1         | 0.11.2        | 0.12.2        | 0.3.1         | 0.3.0         |
//!   | 0.7.0         | 0.11.2        | 0.12          | 0.3.1         | 0.3.0         |
//!   | 0.6.0         | 0.11.2        | 0.11          | 0.3.0         | 0.2.0         |
//!   | 0.5.0         | 0.11.1        | 0.10          | 0.3.0         | 0.1.0         |
//!   | 0.4.0         | 0.11.0        | 0.9           | 0.2.1         | -             |
//!   | 0.3.0         | 0.10.1        | 0.8           | 0.2           | -             |
//!   | 0.1.0         | 0.10.0        | 0.7           | 0.1           | -             |
//!  
//! ## Example
//!
//! The example below is using `wasmedge-sdk` to run a WebAssembly module written with its WAT format (textual format). If you would like more examples, please refer to [Examples of WasmEdge RustSDK](https://github.com/second-state/wasmedge-rustsdk-examples).
//!
//!  ```rust
//!  use wasmedge_sdk::{
//!      error::HostFuncError, host_function, params, wat2wasm, Caller, ImportObjectBuilder, Module,
//!      VmBuilder, WasmValue,
//!  };
//!  
//!  // We define a function to act as our "env" "say_hello" function imported in the
//!  // Wasm program above.
//!  #[host_function]
//!  pub fn say_hello(_caller: Caller, _args: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
//!      println!("Hello, world!");
//!  
//!      Ok(vec![])
//!  }
//!  
//!  #[cfg_attr(test, test)]
//!  fn main() -> anyhow::Result<()> {
//!      // create an import module
//!      let import = ImportObjectBuilder::new()
//!          .with_func::<(), ()>("say_hello", say_hello)?
//!          .build("env")?;
//!  
//!      let wasm_bytes = wat2wasm(
//!          br#"
//!      (module
//!        ;; First we define a type with no parameters and no results.
//!        (type $no_args_no_rets_t (func (param) (result)))
//!      
//!        ;; Then we declare that we want to import a function named "env" "say_hello" with
//!        ;; that type signature.
//!        (import "env" "say_hello" (func $say_hello (type $no_args_no_rets_t)))
//!      
//!        ;; Finally we create an entrypoint that calls our imported function.
//!        (func $run (type $no_args_no_rets_t)
//!          (call $say_hello))
//!        ;; And mark it as an exported function named "run".
//!        (export "run" (func $run)))
//!      "#,
//!      )?;
//!  
//!      // loads a wasm module from the given in-memory bytes
//!      let module = Module::from_bytes(None, wasm_bytes)?;
//!  
//!      // create an executor
//!      VmBuilder::new()
//!          .build()?
//!          .register_import_module(import)?
//!          .register_module(Some("extern"), module)?
//!          .run_func(Some("extern"), "run", params!())?;
//!  
//!      Ok(())
//!  }
//!  
//!  ```
//!

#[doc(hidden)]
pub mod caller;
#[doc(hidden)]
#[cfg(feature = "aot")]
mod compiler;
pub mod config;
pub mod dock;
mod executor;
mod externals;
mod import;
mod instance;
#[doc(hidden)]
pub mod io;
#[doc(hidden)]
pub mod log;
mod module;
pub mod plugin;
mod statistics;
mod store;
pub mod types;
#[doc(hidden)]
pub mod utils;
#[doc(hidden)]
pub mod vm;
pub mod wasi;

pub use caller::Caller;
#[doc(inline)]
#[cfg(feature = "aot")]
pub use compiler::Compiler;
#[doc(inline)]
pub use executor::Executor;
#[doc(inline)]
pub use externals::{Func, FuncRef, FuncTypeBuilder, Global, Memory, Table};
#[doc(inline)]
pub use import::{ImportObject, ImportObjectBuilder};
pub use instance::{AsInstance, Instance};
#[doc(inline)]
pub use io::{WasmVal, WasmValType, WasmValTypeList};
#[doc(inline)]
pub use log::LogManager;
#[doc(inline)]
pub use module::{ExportType, ImportType, Module};
#[doc(inline)]
pub use statistics::Statistics;
#[doc(inline)]
pub use store::Store;
#[doc(inline)]
pub use utils::Driver;
#[doc(inline)]
pub use vm::{Vm, VmBuilder};

/// Parses in-memory bytes as either the [WebAssembly Text format](http://webassembly.github.io/spec/core/text/index.html), or a binary WebAssembly module
pub use wasmedge_types::{
    error, wat2wasm, CompilerOptimizationLevel, CompilerOutputFormat, ExternalInstanceType,
    FuncType, GlobalType, HostRegistration, MemoryType, Mutability, RefType, TableType, ValType,
    WasmEdgeResult,
};

pub use wasmedge_macro::{async_host_function, host_function};

/// WebAssembly value type.
pub type WasmValue = wasmedge_sys::types::WasmValue;

pub type CallingFrame = wasmedge_sys::CallingFrame;

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
    /// # Errors
    ///
    /// If fail to run the host function, then an error is returned.
    fn run_func(
        &self,
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
    /// # Errors
    ///
    /// If fail to run the host function, then an error is returned.
    fn run_func_ref(
        &self,
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
