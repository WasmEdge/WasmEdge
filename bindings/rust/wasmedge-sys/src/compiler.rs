//! Defines WasmEdge ahead-of-time compiler.

use crate::{error::WasmEdgeError, ffi, utils, utils::check, Config, WasmEdgeResult};
use std::path::Path;

/// Defines WasmEdge ahead-of-time(AOT) compiler and the relevant APIs.
#[cfg(feature = "aot")]
#[derive(Debug)]
pub struct Compiler {
    pub(crate) inner: InnerCompiler,
}
impl Drop for Compiler {
    fn drop(&mut self) {
        if !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_CompilerDelete(self.inner.0) }
        }
    }
}
impl Compiler {
    /// Creates a new AOT [compiler](crate::Compiler).
    ///
    /// # Error
    ///
    /// If fail to create a AOT [compiler](crate::Compiler), then an error is returned.
    #[cfg(feature = "aot")]
    pub fn create(config: Option<Config>) -> WasmEdgeResult<Self> {
        let ctx = match config {
            Some(mut config) => {
                let ctx = unsafe { ffi::WasmEdge_CompilerCreate(config.inner.0) };
                config.inner.0 = std::ptr::null_mut();
                ctx
            }
            None => unsafe { ffi::WasmEdge_CompilerCreate(std::ptr::null_mut()) },
        };

        match ctx.is_null() {
            true => Err(WasmEdgeError::CompilerCreate),
            false => Ok(Self {
                inner: InnerCompiler(ctx),
            }),
        }
    }

    /// The compiler compiles the input WASM from the given file path for the AOT mode and stores the result to the output file path.
    ///
    /// # Arguments
    ///
    /// * `in_path` - The input WASM file path.
    ///
    /// * `out_path` - The output WASM file path.
    ///
    /// # Error
    ///
    /// If fail to compile, then an error is returned.
    #[cfg(feature = "aot")]
    pub fn compile(
        &self,
        in_path: impl AsRef<Path>,
        out_path: impl AsRef<Path>,
    ) -> WasmEdgeResult<()> {
        let in_path = utils::path_to_cstring(in_path.as_ref())?;
        let out_path = utils::path_to_cstring(out_path.as_ref())?;
        unsafe {
            check(ffi::WasmEdge_CompilerCompile(
                self.inner.0,
                in_path.as_ptr(),
                out_path.as_ptr(),
            ))
        }
    }
}

#[cfg(feature = "aot")]
#[derive(Debug)]
pub(crate) struct InnerCompiler(pub(crate) *mut ffi::WasmEdge_CompilerContext);
unsafe impl Send for InnerCompiler {}
unsafe impl Sync for InnerCompiler {}

#[cfg(feature = "aot")]
#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        error::{CoreError, CoreLoadError},
        Config,
    };
    use std::{
        io::Read,
        sync::{Arc, Mutex},
        thread,
    };
    use wasmedge_types::CompilerOutputFormat;

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
                .join("bindings/rust/wasmedge-sys/tests/data/test.wasm");
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
            let mut config = result.unwrap();
            // compile file for shared library output format
            config.set_aot_compiler_output_format(CompilerOutputFormat::Native);

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

    #[test]
    #[ignore]
    fn test_compiler_send() {
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

        let handle = thread::spawn(move || {
            // compile a file for universal WASM output format
            let in_path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wasm");
            let out_path = std::path::PathBuf::from("fibonacci_send_thread_aot.wasm");
            assert!(!out_path.exists());
            let result = compiler.compile(&in_path, &out_path);
            assert!(result.is_ok());
            assert!(out_path.exists());
            assert!(std::fs::remove_file(out_path).is_ok());
        });

        handle.join().unwrap();
    }

    #[test]
    #[ignore]
    fn test_compiler_sync() {
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();

        // create a AOT Compiler without configuration
        let result = Compiler::create(None);
        assert!(result.is_ok());

        // create a AOT Compiler with a given configuration
        let result = Compiler::create(Some(config));
        assert!(result.is_ok());
        let compiler = Arc::new(Mutex::new(result.unwrap()));

        let compiler_cloned = Arc::clone(&compiler);
        let handle = thread::spawn(move || {
            let result = compiler_cloned.lock();
            assert!(result.is_ok());
            let compiler = result.unwrap();

            // compile a file for universal WASM output format
            let in_path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wasm");
            let out_path = std::path::PathBuf::from("fibonacci_sync_thread_aot.wasm");
            assert!(!out_path.exists());
            let result = compiler.compile(&in_path, &out_path);
            assert!(result.is_ok());
            assert!(out_path.exists());
            assert!(std::fs::remove_file(out_path).is_ok());
        });

        {
            let result = compiler.lock();
            assert!(result.is_ok());
            let compiler_main = result.unwrap();
            // compile a file for universal WASM output format
            let in_path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wasm");
            let out_path = std::path::PathBuf::from("fibonacci_sync_main_aot.wasm");
            assert!(!out_path.exists());
            let result = compiler_main.compile(&in_path, &out_path);
            assert!(result.is_ok());
            assert!(out_path.exists());
            assert!(std::fs::remove_file(out_path).is_ok());
        }

        handle.join().unwrap();
    }
}
