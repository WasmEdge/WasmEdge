use crate::{wasmedge, Config, GlobalType, MemoryType, Signature, TableType};
use std::{borrow::Cow, path::Path};
use thiserror::Error;
use wasmedge_sys as sys;

#[derive(Debug)]
pub struct Module {
    pub(crate) inner: wasmedge::Module,
}
impl Module {
    pub fn from_file(config: Option<&Config>, file: impl AsRef<Path>) -> WasmEdgeResult<Self> {
        let config = match config {
            Some(config) => Some(&config.inner),
            None => None,
        };

        // create a Loader instance
        let loader = wasmedge::Loader::create(config)?;

        // load a module from a wasm file
        let inner = loader.from_file(file.as_ref())?;
        Ok(Self { inner })
    }

    pub fn from_buffer(config: Option<&Config>, buffer: impl AsRef<[u8]>) -> WasmEdgeResult<Self> {
        let config = match config {
            Some(config) => Some(&config.inner),
            None => None,
        };

        // create a Loader instance
        let loader = wasmedge::Loader::create(config)?;

        // load a module from a wasm buffer
        let inner = loader.from_buffer(buffer.as_ref())?;
        Ok(Self { inner })
    }

    pub fn import_iter(&self) -> impl Iterator<Item = ImportType> {
        self.inner.imports_iter().map(|inner| ImportType {
            inner,
            module: self,
        })
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
    inner: wasmedge::Import,
    module: &'module Module,
}
impl<'module> ImportType<'module> {
    pub fn name(&self) -> Cow<'_, str> {
        self.inner.name()
    }

    pub fn module_name(&self) -> Cow<'_, str> {
        self.inner.module_name()
    }

    pub fn ty(&self) -> WasmEdgeResult<ExternalType> {
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
    inner: wasmedge::Export,
    module: &'module Module,
}
impl<'module> ExportType<'module> {
    pub fn name(&self) -> Cow<'_, str> {
        self.inner.name()
    }

    pub fn ty(&self) -> WasmEdgeResult<ExternalType> {
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

pub trait Engine {
    fn instantiate(&self) -> Result<(), WasmError>;

    // TODO: fn run_funtion

    // TODO: fn run_registered_function
}

pub struct Vm {
    inner: sys::Vm,
}
impl Engine for Vm {
    fn instantiate(&self) -> Result<(), WasmError> {
        self.inner.instantiate_new()?;
        Ok(())
    }
}

#[derive(Error, Clone, Debug, PartialEq)]
pub enum WasmError {
    #[error("{0}")]
    Operation(String),
    #[error("Unknown error")]
    Unknown,
}

impl From<sys::WasmEdgeError> for WasmError {
    fn from(error: sys::WasmEdgeError) -> Self {
        todo!()
    }
}

pub type WasmEdgeResult<T> = Result<T, WasmError>;

pub struct Func {
    inner: sys::Function,
}
impl Func {
    pub fn call(
        engine: &impl Engine,
        store: &Store,
        params: impl IntoIterator<Item = sys::Value>,
    ) -> Result<Vec<sys::Value>, WasmError> {
        todo!()
    }
}

pub struct Store {
    inner: sys::Store,
}
