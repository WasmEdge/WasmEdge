use crate::{error::Result, wasmedge, Config, GlobalType, MemoryType, Signature, TableType};
use std::{borrow::Cow, path::Path};

#[derive(Debug)]
pub struct Module {
    pub(crate) inner: wasmedge::Module,
}
impl Module {
    /// Returns a validated module from a file.
    ///
    /// This function does not validate the loaded module.
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
    /// This function does not validate the loaded module.
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

    pub fn count_of_imports(&self) -> u32 {
        self.inner.count_of_imports()
    }

    pub fn import_iter(&self) -> impl Iterator<Item = ImportType> {
        self.inner.imports_iter().map(|inner| ImportType {
            inner,
            module: self,
        })
    }

    pub fn count_of_exports(&self) -> u32 {
        self.inner.count_of_exports()
    }

    pub fn export_iter(&self) -> impl Iterator<Item = ExportType> {
        self.inner.exports_iter().map(|inner| ExportType {
            inner,
            module: self,
        })
    }

    pub fn get_export(&self, name: impl AsRef<str>) -> Option<ExternalType> {
        let exports = self
            .export_iter()
            .filter(|x| x.name() == name.as_ref())
            .collect::<Vec<_>>();
        match exports.is_empty() {
            true => None,
            false => exports[0].ty().ok(),
        }
    }
}

#[derive(Debug)]
pub struct ImportType<'module> {
    inner: wasmedge::Import<'module>,
    module: &'module Module,
}
impl<'module> ImportType<'module> {
    pub fn name(&self) -> Cow<'_, str> {
        self.inner.name()
    }

    pub fn module_name(&self) -> Cow<'_, str> {
        self.inner.module_name()
    }

    pub fn ty(&self) -> Result<ExternalType> {
        match self.inner.ty() {
            wasmedge::ExternalType::Function => {
                let func_ty = self.inner.function_type(&self.module.inner)?;
                Ok(ExternalType::Func(func_ty.into()))
            }
            wasmedge::ExternalType::Global => {
                let global_ty = self.inner.global_type(&self.module.inner)?;
                Ok(ExternalType::Global(global_ty.into()))
            }
            wasmedge::ExternalType::Memory => {
                let mem_ty = self.inner.memory_type(&self.module.inner)?;
                Ok(ExternalType::Memory(mem_ty.into()))
            }
            wasmedge::ExternalType::Table => {
                let table_ty = self.inner.table_type(&self.module.inner)?;
                Ok(ExternalType::Table(table_ty.into()))
            }
        }
    }
}

#[derive(Debug)]
pub struct ExportType<'module> {
    inner: wasmedge::Export<'module>,
    module: &'module Module,
}
impl<'module> ExportType<'module> {
    pub fn name(&self) -> Cow<'_, str> {
        self.inner.name()
    }

    pub fn ty(&self) -> Result<ExternalType> {
        match self.inner.ty() {
            wasmedge::ExternalType::Function => {
                let func_ty = self.inner.function_type(&self.module.inner)?;
                Ok(ExternalType::Func(func_ty.into()))
            }
            wasmedge::ExternalType::Global => {
                let global_ty = self.inner.global_type(&self.module.inner)?;
                Ok(ExternalType::Global(global_ty.into()))
            }
            wasmedge::ExternalType::Memory => {
                let mem_ty = self.inner.memory_type(&self.module.inner)?;
                Ok(ExternalType::Memory(mem_ty.into()))
            }
            wasmedge::ExternalType::Table => {
                let table_ty = self.inner.table_type(&self.module.inner)?;
                Ok(ExternalType::Table(table_ty.into()))
            }
        }
    }
}

pub enum ExternalType {
    Func(Signature),
    Table(TableType),
    Memory(MemoryType),
    Global(GlobalType),
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
            WasmEdgeError::Operation(wasmedge::WasmEdgeError::Core(
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
            WasmEdgeError::Operation(wasmedge::WasmEdgeError::Core(
                wasmedge::error::CoreError::Load(wasmedge::error::CoreLoadError::UnexpectedEnd)
            ))
        );
    }
}
