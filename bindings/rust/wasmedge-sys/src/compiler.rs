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
    pub fn create(config: Option<Config>) -> WasmEdgeResult<Self> {
        let ctx = match config {
            Some(mut config) => {
                let ctx = unsafe { wasmedge::WasmEdge_CompilerCreate(config.ctx) };
                config.ctx = std::ptr::null_mut();
                ctx
            }
            None => unsafe { wasmedge::WasmEdge_CompilerCreate(std::ptr::null_mut()) },
        };

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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        error::{CoreError, CoreLoadError},
        CompilerOutputFormat, Config,
    };
    use std::io::Read;

    #[test]
    fn test_compiler() {
        {
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();

            // create a AOT Compiler without configuration
            let result = Compiler::create(None);
            assert!(result.is_ok());

            // create a AOT Compiler with a given configuration
            let result = Compiler::create(Some(config));
            assert!(result.is_ok());
            let compiler = result.unwrap();

            // compile a file for universal WASM output format
            let in_path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("test/api/apiTestData/test.wasm");
            let out_path = std::path::PathBuf::from("test_aot.wasm");
            assert!(!out_path.exists());
            let result = compiler.compile(&in_path, &out_path);
            assert!(result.is_ok());
            assert!(out_path.exists());
            assert!(std::fs::remove_file(out_path).is_ok());

            // compile a virtual file
            let result = compiler.compile("not_exist.wasm", "not_exist_ast.wasm");
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Core(CoreError::Load(CoreLoadError::IllegalPath))
            );
        }

        {
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();
            // compile file for shared library output format
            let config = config.set_compiler_output_format(CompilerOutputFormat::Native);

            let result = Compiler::create(Some(config));
            assert!(result.is_ok());
            let compiler = result.unwrap();
            let in_path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/tests/data/test.wasm");
            let out_path = std::path::PathBuf::from("test_aot.so");
            assert!(!out_path.exists());
            let result = compiler.compile(&in_path, &out_path);
            assert!(result.is_ok());
            assert!(out_path.exists());

            // read buffer
            let result = std::fs::File::open(&out_path);
            assert!(result.is_ok());
            let mut f = result.unwrap();
            let mut buffer = [0u8; 4];
            let result = f.read(&mut buffer);
            assert!(result.is_ok());
            let wasm_magic: [u8; 4] = [0x00, 0x61, 0x73, 0x6D];
            assert_ne!(buffer, wasm_magic);

            assert!(std::fs::remove_file(out_path).is_ok());
        }
    }
}
