//! Defines Module, ImportType, and ExportType.

use crate::{config::Config, error::Result, types::ExternalType, wasmedge};
use std::marker::PhantomData;
use std::{borrow::Cow, path::Path};

/// Struct of WasmEdge Module.
///
/// A [Module] is a compiled in-memory representation of an input WebAssembly binary. In the instantiation process, a [Module] is instatiated to a module [instance](crate::instance), from which the exported [function](crate::Func), [table](crate::Table), [memory](crate::Memory), and [global](crate::Global) instances can be fetched.
#[derive(Debug)]
pub struct Module {
    pub(crate) inner: wasmedge::Module,
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
        let inner_loader = wasmedge::Loader::create(inner_config)?;
        // load module
        let inner = inner_loader.from_file(file.as_ref())?;

        let inner_config = match config {
            Some(config) => Some(Config::copy_from(config)?.inner),
            None => None,
        };
        let inner_validator = wasmedge::Validator::create(inner_config)?;
        // validate module
        inner_validator.validate(&inner)?;

        Ok(Self { inner })
    }

    /// Returns a validated module from a buffer.
    ///
    /// # Arguments
    ///
    /// - `config` specifies a global configuration.
    ///
    /// - `buffer` specifies a WASM buffer.
    ///
    /// # Error
    ///
    /// If fail to load and valiate a module from a buffer, returns an error.
    pub fn from_buffer(config: Option<&Config>, buffer: impl AsRef<[u8]>) -> Result<Self> {
        let inner_config = match config {
            Some(config) => Some(Config::copy_from(config)?.inner),
            None => None,
        };
        let inner_loader = wasmedge::Loader::create(inner_config)?;
        // load a module from a wasm buffer
        let inner = inner_loader.from_buffer(buffer.as_ref())?;

        let inner_config = match config {
            Some(config) => Some(Config::copy_from(config)?.inner),
            None => None,
        };
        let inner_validator = wasmedge::Validator::create(inner_config)?;
        // validate module
        inner_validator.validate(&inner)?;

        Ok(Self { inner })
    }

    /// Returns the count of the [import types](crate::ImportType) of the [Module].
    pub fn count_of_imports(&self) -> u32 {
        self.inner.count_of_imports()
    }

    /// Returns a vector of [import types](crate::ImportType) of the [module](crate::Module).
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

    /// Returns the count of the [export types](crate::ExportType) of the [module](crate::Module).
    pub fn count_of_exports(&self) -> u32 {
        self.inner.count_of_exports()
    }

    /// Returns the [export types](crate::ExportType) of the [module](crate::Module).
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

    /// Gets the [export type](crate::ExportType) by name.
    ///
    /// # Argument
    ///
    /// - `name` specifies the name of the target [export type](crate::ExportType).
    pub fn get_export(&self, name: impl AsRef<str>) -> Option<ExternalType> {
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
/// [ImportType] is used for getting the type information of the imports from a WasmEdge [module](crate::Module).
#[derive(Debug)]
pub struct ImportType<'module> {
    inner: wasmedge::Import<'module>,
    _marker: PhantomData<&'module Module>,
}
impl<'module> ImportType<'module> {
    /// Returns the name of the [ImportType].
    pub fn name(&self) -> Cow<'_, str> {
        self.inner.name()
    }

    /// Returns the module name from the [ImportType].
    pub fn module_name(&self) -> Cow<'_, str> {
        self.inner.module_name()
    }

    /// Returns the type of the [ImportType].
    pub fn ty(&self) -> Result<ExternalType> {
        match self.inner.ty() {
            wasmedge::ExternalType::Function => {
                let func_ty = self.inner.function_type()?;
                Ok(ExternalType::Func(func_ty.into()))
            }
            wasmedge::ExternalType::Global => {
                let global_ty = self.inner.global_type()?;
                Ok(ExternalType::Global(global_ty.into()))
            }
            wasmedge::ExternalType::Memory => {
                let mem_ty = self.inner.memory_type()?;
                Ok(ExternalType::Memory(mem_ty.into()))
            }
            wasmedge::ExternalType::Table => {
                let table_ty = self.inner.table_type()?;
                Ok(ExternalType::Table(table_ty.into()))
            }
        }
    }
}

/// Struct of WasmEdge ExportType.
///
/// [ExportType] is used for getting the type information of the exports from a [module](crate::Module).
#[derive(Debug)]
pub struct ExportType<'module> {
    inner: wasmedge::Export<'module>,
    _marker: PhantomData<&'module Module>,
}
impl<'module> ExportType<'module> {
    /// Returns the name of the [ExportType].
    pub fn name(&self) -> Cow<'_, str> {
        self.inner.name()
    }

    /// Returns the type of the [ExportType].
    pub fn ty(&self) -> Result<ExternalType> {
        match self.inner.ty() {
            wasmedge::ExternalType::Function => {
                let func_ty = self.inner.function_type()?;
                Ok(ExternalType::Func(func_ty.into()))
            }
            wasmedge::ExternalType::Global => {
                let global_ty = self.inner.global_type()?;
                Ok(ExternalType::Global(global_ty.into()))
            }
            wasmedge::ExternalType::Memory => {
                let mem_ty = self.inner.memory_type()?;
                Ok(ExternalType::Memory(mem_ty.into()))
            }
            wasmedge::ExternalType::Table => {
                let table_ty = self.inner.table_type()?;
                Ok(ExternalType::Table(table_ty.into()))
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{error::WasmEdgeError, wasmedge};

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
            WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Core(
                wasmedge::error::CoreError::Load(wasmedge::error::CoreLoadError::IllegalPath)
            ))
        );
    }

    #[test]
    fn test_module_from_buffer() {
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = std::fs::read(file);
        assert!(result.is_ok());
        let buffer = result.unwrap();

        let result = Module::from_buffer(None, &buffer);
        assert!(result.is_ok());

        // attempt to load an empty buffer
        let result = Module::from_buffer(None, &[]);
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Core(
                wasmedge::error::CoreError::Load(wasmedge::error::CoreLoadError::UnexpectedEnd)
            ))
        );
    }
}
