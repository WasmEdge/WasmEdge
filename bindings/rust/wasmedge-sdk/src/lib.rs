#![doc(
    html_logo_url = "https://github.com/cncf/artwork/blob/master/projects/wasm-edge-runtime/icon/color/wasm-edge-runtime-icon-color.png?raw=true",
    html_favicon_url = "https://raw.githubusercontent.com/cncf/artwork/49169bdbc88a7ce3c4a722c641cc2d548bd5c340/projects/wasm-edge-runtime/icon/color/wasm-edge-runtime-icon-color.svg"
)]

//! # Overview
//!
//! The [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) crate defines a group of high-level Rust APIs, which are used to build up business applications.
//!
//! Notice that `wasmedge-sdk` requires the nightly version of Rust.
//!
//! ## Usage
//!
//! A quick-start example below is using `wasmedge-sdk` to run a WebAssembly module written with its WAT format (textual format):
//!
//!  ```rust
//!  #![feature(explicit_generic_args_with_impl_trait)]
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

#![feature(explicit_generic_args_with_impl_trait)]

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

#[doc(inline)]
#[cfg(feature = "aot")]
pub use compiler::Compiler;
#[doc(inline)]
pub use executor::Executor;
#[doc(inline)]
pub use externals::{Func, FuncRef, FuncTypeBuilder, Global, Memory, Table};
#[doc(inline)]
pub use import::{ImportObject, ImportObjectBuilder};
pub use instance::Instance;
#[doc(inline)]
pub use io::{WasmVal, WasmValType, WasmValTypeList};
#[doc(inline)]
pub use module::{ExportType, ImportType, Module};
#[doc(inline)]
pub use statistics::Statistics;
#[doc(inline)]
pub use store::Store;

use wasmedge_sys::types::WasmValue;

/// Alias type for host function
pub type HostFunc = wasmedge_sys::HostFunc;

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
