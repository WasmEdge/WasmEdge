//! Defines WasmEdge AST Module, ImportType, and ExportType.

use crate::{config::Config, ExternalInstanceType, WasmEdgeResult};
use std::{borrow::Cow, marker::PhantomData, path::Path};
use wasmedge_sys as sys;

/// Defines compiled in-memory representation of an input WASM binary.
///
/// A [Module] is a compiled in-memory representation of an input WebAssembly binary. In the instantiation process, a [Module] is instatiated to a module [instance](crate::Instance), from which the exported [function](crate::Func), [table](crate::Table), [memory](crate::Memory), and [global](crate::Global) instances can be fetched.
#[derive(Debug, Clone)]
pub struct Module {
    pub(crate) inner: sys::Module,
}
impl Module {
    /// Returns a validated module from a file.
    ///
    /// # Arguments
    ///
    /// * `config` - The global configuration.
    ///
    /// * `file` - A wasm file or an AOT wasm file.
    ///
    /// # Error
    ///
    /// If fail to load and valiate a module from a file, returns an error.
    pub fn from_file(config: Option<&Config>, file: impl AsRef<Path>) -> WasmEdgeResult<Self> {
        let inner_config = config.map(|cfg| &cfg.inner);

        // load module
        let inner_module = sys::Loader::create(inner_config)?.from_file(file.as_ref())?;

        // validate module
        sys::Validator::create(inner_config)?.validate(&inner_module)?;

        Ok(Self {
            inner: inner_module,
        })
    }

    /// Loads a WebAssembly binary module from in-memory bytes.
    ///
    /// # Arguments
    ///
    /// * `config` - The global configuration.
    ///
    /// * `bytes` - The in-memory bytes to be parsed.
    ///
    /// # Error
    ///
    /// If fail to load and valiate the WebAssembly module from the given in-memory bytes, returns an error.
    pub fn from_bytes(config: Option<&Config>, bytes: impl AsRef<[u8]>) -> WasmEdgeResult<Self> {
        let inner_config = config.map(|cfg| &cfg.inner);

        // load module
        let inner_module = sys::Loader::create(inner_config)?.from_bytes(bytes.as_ref())?;

        // validate module
        sys::Validator::create(inner_config)?.validate(&inner_module)?;

        Ok(Self {
            inner: inner_module,
        })
    }

    /// Returns the count of the imported WasmEdge instances in the [module](crate::Module).
    pub fn count_of_imports(&self) -> u32 {
        self.inner.count_of_imports()
    }

    /// Returns the [import types](crate::ImportType) of all imported WasmEdge instances in the [module](crate::Module).
    pub fn imports(&self) -> Vec<ImportType> {
        let mut imports = Vec::new();
        for inner_import in self.inner.imports() {
            let import = ImportType {
                inner: inner_import,
                _marker: PhantomData,
            };
            imports.push(import);
        }
        imports
    }

    /// Returns the count of the exported WasmEdge instances from the [module](crate::Module).
    pub fn count_of_exports(&self) -> u32 {
        self.inner.count_of_exports()
    }

    /// Returns the [export types](crate::ExportType) of all exported WasmEdge instances from the [module](crate::Module).
    pub fn exports(&self) -> Vec<ExportType> {
        let mut exports = Vec::new();
        for inner_export in self.inner.exports() {
            let export = ExportType {
                inner: inner_export,
                _marker: PhantomData,
            };
            exports.push(export);
        }
        exports
    }

    /// Gets the [export type](crate::ExportType) by the name of a specific exported WasmEdge instance.
    ///
    /// # Argument
    ///
    /// * `name` - The name of the target exported WasmEdge instance.
    pub fn get_export(&self, name: impl AsRef<str>) -> Option<ExternalInstanceType> {
        let exports = self
            .exports()
            .into_iter()
            .filter(|x| x.name() == name.as_ref())
            .collect::<Vec<_>>();
        match exports.is_empty() {
            true => None,
            false => exports[0].ty().ok(),
        }
    }
}

/// Defines the types of the imported instances.
#[derive(Debug)]
pub struct ImportType<'module> {
    inner: sys::ImportType<'module>,
    _marker: PhantomData<&'module Module>,
}
impl<'module> ImportType<'module> {
    /// Returns the imported name of the WasmEdge instance.
    pub fn name(&self) -> Cow<'_, str> {
        self.inner.name()
    }

    /// Returns the name of the module hosting the imported WasmEdge instance.
    pub fn module_name(&self) -> Cow<'_, str> {
        self.inner.module_name()
    }

    /// Returns the type of the imported WasmEdge instance, which is one of the types defined in [ExternalInstanceType](wasmedge_types::ExternalInstanceType).
    pub fn ty(&self) -> WasmEdgeResult<ExternalInstanceType> {
        let ty = self.inner.ty()?;
        Ok(ty)
    }
}

/// Defines the types of the exported instances.
#[derive(Debug)]
pub struct ExportType<'module> {
    inner: sys::ExportType<'module>,
    _marker: PhantomData<&'module Module>,
}
impl<'module> ExportType<'module> {
    /// Returns the exported name of the WasmEdge instance.
    pub fn name(&self) -> Cow<'_, str> {
        self.inner.name()
    }

    /// Returns the type of the exported WasmEdge instance, which is one of the types defined in [ExternalInstanceType](wasmedge_types::ExternalInstanceType).
    pub fn ty(&self) -> WasmEdgeResult<ExternalInstanceType> {
        let ty = self.inner.ty()?;
        Ok(ty)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        error::{CoreError, CoreLoadError, WasmEdgeError},
        wat2wasm,
    };

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_module_from_wasm() {
        // load wasm module from a specified wasm file
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sdk/examples/data/fibonacci.wat");

        let result = Module::from_file(None, file);
        assert!(result.is_ok());

        // attempt to load a non-existent wasm file
        let result = Module::from_file(None, "not_exist_file.wasm");
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Load(
                CoreLoadError::IllegalPath
            )))
        );
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_module_from_wat() {
        // load wasm module from a specified wasm file
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wat");

        let result = Module::from_file(None, file);
        assert!(result.is_ok());
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_module_from_bytes() {
        // read the wasm bytes
        let wasm_bytes = wat2wasm(
            br#"
        (module
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
        )
        .unwrap();

        let result = Module::from_bytes(None, wasm_bytes);
        assert!(result.is_ok());

        // attempt to load an empty buffer
        let result = Module::from_bytes(None, []);
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Load(
                CoreLoadError::UnexpectedEnd
            ))),
        );
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_module_clone() {
        // read the wasm bytes
        let wasm_bytes = wat2wasm(
            br#"
        (module
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
        )
        .unwrap();

        let result = Module::from_bytes(None, wasm_bytes);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert_eq!(module.exports().len(), 1);

        // clone the module
        let module_clone = module.clone();
        assert_eq!(module.exports().len(), module_clone.exports().len());
    }
}
