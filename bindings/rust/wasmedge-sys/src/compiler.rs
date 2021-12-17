use crate::{error::check, utils, wasmedge, Config, Error, WasmEdgeResult};
use std::path::Path;

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
    /// Create a Compiler instance
    pub fn create(config: &Config) -> WasmEdgeResult<Self> {
        let ctx = unsafe { wasmedge::WasmEdge_CompilerCreate(config.ctx) };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create Compiler instance",
            ))),
            false => Ok(Self { ctx }),
        }
    }

    pub fn compile(
        &self,
        in_path: impl AsRef<Path>,
        out_path: impl AsRef<Path>,
    ) -> WasmEdgeResult<()> {
        let ref_in_path = in_path.as_ref();
        if !ref_in_path.exists() {
            return Err(Error::OperationError(format!(
                "Invalid input path: {}",
                ref_in_path.to_string_lossy()
            )));
        }

        let ref_out_path = out_path.as_ref();
        if !ref_out_path.exists() {
            return Err(Error::OperationError(format!(
                "Invalid output path: {}",
                ref_out_path.to_string_lossy()
            )));
        }

        let in_path = utils::path_to_cstring(ref_in_path)?;
        let out_path = utils::path_to_cstring(ref_out_path)?;
        unsafe {
            check(wasmedge::WasmEdge_CompilerCompile(
                self.ctx,
                in_path.as_ptr(),
                out_path.as_ptr(),
            ))
        }
    }
}
