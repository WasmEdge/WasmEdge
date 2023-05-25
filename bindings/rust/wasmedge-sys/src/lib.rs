#![doc(
    html_logo_url = "https://github.com/cncf/artwork/blob/master/projects/wasm-edge-runtime/icon/color/wasm-edge-runtime-icon-color.png?raw=true",
    html_favicon_url = "https://raw.githubusercontent.com/cncf/artwork/49169bdbc88a7ce3c4a722c641cc2d548bd5c340/projects/wasm-edge-runtime/icon/color/wasm-edge-runtime-icon-color.svg"
)]

//! # Overview
//! The [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crate defines a group of low-level Rust APIs for WasmEdge, a light-weight, high-performance, and extensible WebAssembly runtime for cloud-native, edge, and decentralized applications.
//!
//! For developers, it is strongly recommended that the APIs in `wasmedge-sys` are used to construct high-level libraries, while `wasmedge-sdk` is for building up business applications.
//!
//! * Notice that [wasmedge-sys](https://crates.io/crates/wasmedge-sys) requires **Rust v1.66 or above** in the **stable** channel.
//!

//! ## Build
//!
//! To use or build the `wasmedge-sys` crate, the `WasmEdge` library is required. Please refer to [WasmEdge Installation and Uninstallation](https://wasmedge.org/book/en/quick_start/install.html) to install the `WasmEdge` library.
//!
//! * The following table provides the versioning information about each crate of WasmEdge Rust bindings.
//!
//!   | wasmedge-sdk  | WasmEdge lib  | wasmedge-sys  | wasmedge-types| wasmedge-macro|
//!   | :-----------: | :-----------: | :-----------: | :-----------: | :-----------: |
//!   | 0.8.1         | 0.12.1        | 0.13.1        | 0.4.1         | 0.3.0         |
//!   | 0.8.0         | 0.12.0        | 0.13.0        | 0.4.1         | 0.3.0         |
//!   | 0.7.1         | 0.11.2        | 0.12.2        | 0.3.1         | 0.3.0         |
//!   | 0.7.0         | 0.11.2        | 0.12          | 0.3.1         | 0.3.0         |
//!   | 0.6.0         | 0.11.2        | 0.11          | 0.3.0         | 0.2.0         |
//!   | 0.5.0         | 0.11.1        | 0.10          | 0.3.0         | 0.1.0         |
//!   | 0.4.0         | 0.11.0        | 0.9           | 0.2.1         | -             |
//!   | 0.3.0         | 0.10.1        | 0.8           | 0.2           | -             |
//!   | 0.1.0         | 0.10.0        | 0.7           | 0.1           | -             |
//!
//!
//!

//! ## See also
//!
//! * [WasmEdge Runtime Official Website](https://wasmedge.org/)
//! * [WasmEdge Docs](https://wasmedge.org/book/en/)
//! * [WasmEdge C API Documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md)

#![deny(rust_2018_idioms, unreachable_pub)]

#[allow(warnings)]
/// Foreign function interfaces generated from WasmEdge C-API.
pub mod ffi {
    include!(concat!(env!("OUT_DIR"), "/wasmedge.rs"));
}
#[doc(hidden)]
pub mod ast_module;
#[doc(hidden)]
#[cfg(all(feature = "async", target_os = "linux"))]
pub mod r#async;
#[doc(hidden)]
#[cfg(all(feature = "async", target_os = "linux"))]
pub mod async_wasi;
#[doc(hidden)]
#[cfg(feature = "aot")]
pub mod compiler;
#[doc(hidden)]
pub mod config;
#[doc(hidden)]
pub mod executor;
pub mod frame;
#[doc(hidden)]
pub mod instance;
#[doc(hidden)]
pub mod io;
#[doc(hidden)]
pub mod loader;
pub mod plugin;
#[doc(hidden)]
pub mod statistics;
#[doc(hidden)]
pub mod store;
pub mod types;
pub mod utils;
#[doc(hidden)]
pub mod validator;

#[doc(inline)]
pub use ast_module::{ExportType, ImportType, Module};
#[doc(inline)]
#[cfg(feature = "aot")]
pub use compiler::Compiler;
#[doc(inline)]
pub use config::Config;
#[doc(inline)]
pub use executor::Executor;
#[doc(inline)]
pub use frame::CallingFrame;
#[cfg(not(feature = "async"))]
#[doc(inline)]
pub use instance::module::WasiModule;
#[cfg(all(feature = "async", target_os = "linux"))]
#[doc(inline)]
pub use instance::{function::AsyncHostFn, module::AsyncWasiModule};
#[doc(inline)]
pub use instance::{
    function::{FuncRef, FuncType, Function, HostFn},
    global::{Global, GlobalType},
    memory::{MemType, Memory},
    module::{AsImport, AsInstance, ImportModule, ImportObject, Instance},
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
use wasmedge_types::{error, WasmEdgeResult};

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
    /// # Errors
    ///
    /// If fail to run the host function, then an error is returned.
    fn run_func(
        &self,
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
    /// # Errors
    ///
    /// If fail to run the host function, then an error is returned.
    fn run_func_ref(
        &self,
        func_ref: &FuncRef,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>>;
}
