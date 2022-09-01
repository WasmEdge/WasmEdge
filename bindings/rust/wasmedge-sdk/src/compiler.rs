//! Defines WasmEdge ahead-of-time compiler.

use crate::{config::Config, WasmEdgeResult};
use std::path::Path;
use wasmedge_sys as sys;

/// Defines WasmEdge ahead-of-time(AOT) compiler and the relevant APIs.
#[derive(Debug)]
#[cfg(feature = "aot")]
pub struct Compiler {
    pub(crate) inner: sys::Compiler,
}
impl Compiler {
    /// Creates a new AOT [compiler](crate::Compiler).
    ///
    /// # Error
    ///
    /// If fail to create a AOT [compiler](crate::Compiler), then an error is returned.
    pub fn new(config: Option<&Config>) -> WasmEdgeResult<Self> {
        let inner_config = match config {
            Some(config) => Some(Config::copy_from(config)?.inner),
            None => None,
        };
        let inner = sys::Compiler::create(inner_config)?;

        Ok(Self { inner })
    }

    /// Compiles the input WASM from the given file path for the AOT mode and stores the result to the output file path.
    ///
    /// # Arguments
    ///
    /// * `in_path` specifies the input WASM file path.
    ///
    /// * `out_path` specifies the output WASM file path.
    ///
    /// # Error
    ///
    /// If fail to compile, then an error is returned.
    pub fn compile_from_file(
        &self,
        in_path: impl AsRef<Path>,
        out_path: impl AsRef<Path>,
    ) -> WasmEdgeResult<()> {
        self.inner.compile_from_file(in_path, out_path)?;

        Ok(())
    }

    /// Compiles the input WASM from the given bytes for the AOT mode and stores the result to the output file path.
    ///
    /// # Argument
    ///
    /// * `bytes` - A in-memory WASM bytes.
    ///
    /// * `out_path` - The output WASM file path.
    ///
    /// # Error
    ///
    /// If fail to compile, then an error is returned.
    #[cfg(feature = "aot")]
    pub fn compile_from_bytes(
        &self,
        bytes: impl AsRef<[u8]>,
        out_path: impl AsRef<Path>,
    ) -> WasmEdgeResult<()> {
        self.inner.compile_from_bytes(bytes, out_path)?;

        Ok(())
    }
}
