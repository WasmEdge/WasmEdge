//! Defines Module, ImportType, and ExportType.

use crate::{config::Config, error::Result};
use std::marker::PhantomData;
use std::{borrow::Cow, path::Path};
use wasmedge_sys as sys;
use wasmedge_types::ExternalInstanceType;

/// Struct of WasmEdge Module.
///
/// A [Module] is a compiled in-memory representation of an input WebAssembly binary. In the instantiation process, a [Module] is instatiated to a module [instance](crate::instance), from which the exported [function](crate::Func), [table](crate::Table), [memory](crate::Memory), and [global](crate::Global) instances can be fetched.
#[derive(Debug)]
pub struct Module {
    pub(crate) inner: sys::Module,
}
impl Module {
    /// Returns a validated module from a file.
    ///
    /// # Arguments
    ///
    /// - `config` specifies a global configuration.
    ///
    /// - `file` specifies the path to the target WASM file.
    ///
    /// # Error
    ///
    /// If fail to load and valiate a module from a file, returns an error.
    pub fn from_file(config: Option<&Config>, file: impl AsRef<Path>) -> Result<Self> {
        let inner_config = match config {
            Some(config) => Some(Config::copy_from(config)?.inner),
            None => None,
        };
        let inner_loader = sys::Loader::create(inner_config)?;
        // load module
        let inner = inner_loader.from_file(file.as_ref())?;

        let inner_config = match config {
            Some(config) => Some(Config::copy_from(config)?.inner),
            None => None,
        };
        let inner_validator = sys::Validator::create(inner_config)?;
        // validate module
        inner_validator.validate(&inner)?;

        Ok(Self { inner })
    }

    /// Loads a WebAssembly binary module from in-memory bytes.
    ///
    /// # Arguments
    ///
    /// - `config` specifies a global configuration.
    ///
    /// - `bytes` specifies the in-memory bytes to be parsed.
    ///
    /// # Error
    ///
    /// If fail to load and valiate the WebAssembly module from the given in-memory bytes, returns an error.
    pub fn from_bytes(config: Option<&Config>, bytes: impl AsRef<[u8]>) -> Result<Self> {
        let inner_config = match config {
            Some(config) => Some(Config::copy_from(config)?.inner),
            None => None,
        };
        let inner_loader = sys::Loader::create(inner_config)?;
        // load a module from a wasm buffer
        let inner = inner_loader.from_bytes(bytes.as_ref())?;

        let inner_config = match config {
            Some(config) => Some(Config::copy_from(config)?.inner),
            None => None,
        };
        let inner_validator = sys::Validator::create(inner_config)?;
        // validate module
        inner_validator.validate(&inner)?;

        Ok(Self { inner })
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
                // module: &self,
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
                // module: &self,
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
    /// - `name` specifies the name of the target exported WasmEdge instance.
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

/// Struct of WasmEdge ImportType.
///
/// [ImportType] is used for getting the type information of the imported WasmEdge instances.
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
    pub fn ty(&self) -> Result<ExternalInstanceType> {
        let ty = self.inner.ty()?;
        Ok(ty)
    }
}

/// Struct of WasmEdge ExportType.
///
/// [ExportType] is used for getting the type information of the exported WasmEdge instances.
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
    pub fn ty(&self) -> Result<ExternalInstanceType> {
        let ty = self.inner.ty()?;
        Ok(ty)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::error::WasmEdgeError;

    #[test]
    fn test_module_from_file() {
        // load wasm module from a specified wasm file
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");

        let result = Module::from_file(None, file);
        assert!(result.is_ok());

        // attempt to load a non-existent wasm file
        let result = Module::from_file(None, "not_exist_file.wasm");
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Operation(sys::error::WasmEdgeError::Core(
                sys::error::CoreError::Load(sys::error::CoreLoadError::IllegalPath)
            ))
        );
    }

    #[test]
    fn test_module_from_bytes() {
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = std::fs::read(file);
        assert!(result.is_ok());
        let buffer = result.unwrap();

        let result = Module::from_bytes(None, &buffer);
        assert!(result.is_ok());

        // attempt to load an empty buffer
        let result = Module::from_bytes(None, &[]);
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Operation(sys::error::WasmEdgeError::Core(
                sys::error::CoreError::Load(sys::error::CoreLoadError::UnexpectedEnd)
            ))
        );
    }
}
