//! Defines WasmEdge AOT Compiler.

use crate::{error::check, utils, wasmedge, Config, WasmEdgeError, WasmEdgeResult};
use std::path::Path;

/// Struct of WasmEdge AOT Compiler.
#[derive(Debug)]
pub struct Compiler {
    pub(crate) ctx: *mut wasmedge::WasmEdge_CompilerContext,
}
impl Drop for Compiler {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_CompilerDelete(self.ctx) }
        }
    }
}
impl Compiler {
    /// Creates a new AOT [`Compiler`].
    ///
    /// # Error
    ///
    /// If fail to create a AOT [`Compiler`], then an error is returned.
    pub fn create(config: &Config) -> WasmEdgeResult<Self> {
        let ctx = unsafe { wasmedge::WasmEdge_CompilerCreate(config.ctx) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::CompilerCreate),
            false => Ok(Self { ctx }),
        }
    }

    /// Compiles the input WASM from the given file path.
    ///
    /// # Arguments
    ///
    /// - `in_path` specifies the input WASM file path.
    ///
    /// - `out_path` specifies the output WASM file path.
    ///
    /// # Error
    ///
    /// If fail to compile, then an error is returned.
    pub fn compile(
        &self,
        in_path: impl AsRef<Path>,
        out_path: impl AsRef<Path>,
    ) -> WasmEdgeResult<()> {
        let in_path = utils::path_to_cstring(in_path.as_ref())?;
        let out_path = utils::path_to_cstring(out_path.as_ref())?;
        unsafe {
            check(wasmedge::WasmEdge_CompilerCompile(
                self.ctx,
                in_path.as_ptr(),
                out_path.as_ptr(),
            ))
        }
    }
}
