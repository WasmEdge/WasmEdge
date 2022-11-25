//! Defines WasmEdge ahead-of-time compiler.

use crate::{config::Config, WasmEdgeResult};
use std::path::{Path, PathBuf};
use wasmedge_sys as sys;

/// Defines WasmEdge ahead-of-time(AOT) compiler and the relevant APIs.
#[derive(Debug)]
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
        let inner_config = config.map(|c| c.inner.clone());
        let inner = sys::Compiler::create(inner_config)?;

        Ok(Self { inner })
    }

    /// Compiles the given wasm file into a shared library file (*.so in Linux, *.dylib in macOS, or *.dll in Windows). The file path of the generated shared library file will be returned if the method works successfully.
    ///
    /// # Arguments
    ///
    /// * `wasm_file` - The target wasm file.
    ///
    /// * `filename` - The filename of the generated shared library file.
    ///
    /// * `out_dir` - The target directory to save the generated shared library file.
    ///
    /// # Error
    ///
    /// If fail to compile, then an error is returned.
    pub fn compile_from_file(
        &self,
        wasm_file: impl AsRef<Path>,
        filename: impl AsRef<str>,
        out_dir: impl AsRef<Path>,
    ) -> WasmEdgeResult<PathBuf> {
        #[cfg(target_os = "linux")]
        let extension = "so";
        #[cfg(target_os = "macos")]
        let extension = "dylib";
        #[cfg(target_os = "windows")]
        let extension = "dll";
        let aot_file = out_dir
            .as_ref()
            .join(format!("{}.{}", filename.as_ref(), extension));
        self.inner.compile_from_file(wasm_file, &aot_file)?;

        Ok(aot_file)
    }

    /// Compiles the given wasm bytes into a shared library file (*.so in Linux, *.dylib in macOS, or *.dll in Windows). The file path of the generated shared library file will be returned if the method works successfully.
    ///
    /// # Argument
    ///
    /// * `bytes` - A in-memory WASM bytes.
    ///
    /// * `filename` - The filename of the generated shared library file.
    ///
    /// * `out_dir` - The target directory to save the generated shared library file.
    ///
    /// # Error
    ///
    /// If fail to compile, then an error is returned.
    pub fn compile_from_bytes(
        &self,
        bytes: impl AsRef<[u8]>,
        filename: impl AsRef<str>,
        out_dir: impl AsRef<Path>,
    ) -> WasmEdgeResult<PathBuf> {
        #[cfg(target_os = "linux")]
        let extension = "so";
        #[cfg(target_os = "macos")]
        let extension = "dylib";
        #[cfg(target_os = "windows")]
        let extension = "dll";
        let aot_file = out_dir
            .as_ref()
            .join(format!("{}.{}", filename.as_ref(), extension));
        self.inner.compile_from_bytes(bytes, &aot_file)?;

        Ok(aot_file)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CompilerConfigOptions, ConfigBuilder},
        params, wat2wasm, CompilerOutputFormat, Vm, WasmVal,
    };
    use std::io::Read;

    #[test]
    fn test_compiler_compile_from_file() -> Result<(), Box<dyn std::error::Error>> {
        let config = ConfigBuilder::default()
            .with_compiler_config(
                CompilerConfigOptions::new().out_format(CompilerOutputFormat::Native),
            )
            .build()?;

        let compiler = Compiler::new(Some(&config))?;
        let wasm_file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sdk/examples/data/fibonacci.wat");
        let out_dir = std::env::current_dir()?;
        let aot_filename = "aot_fibonacci_1";
        let aot_file_path = compiler.compile_from_file(wasm_file, aot_filename, out_dir)?;
        assert!(aot_file_path.exists());
        #[cfg(target_os = "macos")]
        assert!(aot_file_path.ends_with("aot_fibonacci_1.dylib"));
        #[cfg(target_os = "linux")]
        assert!(aot_file_path.ends_with("aot_fibonacci_1.so"));
        #[cfg(target_os = "windows")]
        assert!(aot_file_path.ends_with("aot_fibonacci_1.dll"));

        // read buffer
        let mut aot_file = std::fs::File::open(&aot_file_path)?;
        let mut buffer = [0u8; 4];
        aot_file.read_exact(&mut buffer)?;
        let wasm_magic: [u8; 4] = [0x00, 0x61, 0x73, 0x6D];
        assert_ne!(buffer, wasm_magic);

        let res = Vm::new(None)?.run_func_from_file(&aot_file_path, "fib", params!(5))?;
        assert_eq!(res[0].to_i32(), 8);

        // cleanup
        assert!(std::fs::remove_file(aot_file_path).is_ok());

        Ok(())
    }

    #[test]
    fn test_compiler_compile_from_bytes() -> Result<(), Box<dyn std::error::Error>> {
        let wasm_bytes = wat2wasm(
            br#"(module
                (export "fib" (func $fib))
                (func $fib (param $n i32) (result i32)
                 (if
                  (i32.lt_s
                   (get_local $n)
                   (i32.const 2)
                  )
                  (return
                   (i32.const 1)
                  )
                 )
                 (return
                  (i32.add
                   (call $fib
                    (i32.sub
                     (get_local $n)
                     (i32.const 2)
                    )
                   )
                   (call $fib
                    (i32.sub
                     (get_local $n)
                     (i32.const 1)
                    )
                   )
                  )
                 )
                )
               )
          "#,
        )?;

        // create a aot compiler
        let config = ConfigBuilder::default()
            .with_compiler_config(
                CompilerConfigOptions::new().out_format(CompilerOutputFormat::Native),
            )
            .build()?;
        let compiler = Compiler::new(Some(&config))?;

        // compile wasm bytes into a shared library file
        let out_dir = std::env::current_dir()?;
        let aot_filename = "aot_fibonacci_2";
        let aot_file_path = compiler.compile_from_bytes(wasm_bytes, aot_filename, out_dir)?;
        assert!(aot_file_path.exists());
        #[cfg(target_os = "macos")]
        assert!(aot_file_path.ends_with("aot_fibonacci_2.dylib"));
        #[cfg(target_os = "linux")]
        assert!(aot_file_path.ends_with("aot_fibonacci_2.so"));
        #[cfg(target_os = "windows")]
        assert!(aot_file_path.ends_with("aot_fibonacci_2.dll"));

        // read buffer
        let mut aot_file = std::fs::File::open(&aot_file_path)?;
        let mut buffer = [0u8; 4];
        aot_file.read_exact(&mut buffer)?;
        let wasm_magic: [u8; 4] = [0x00, 0x61, 0x73, 0x6D];
        assert_ne!(buffer, wasm_magic);

        let res = Vm::new(None)?.run_func_from_file(&aot_file_path, "fib", params!(5))?;
        assert_eq!(res[0].to_i32(), 8);

        // cleanup
        assert!(std::fs::remove_file(aot_file_path).is_ok());

        Ok(())
    }
}
