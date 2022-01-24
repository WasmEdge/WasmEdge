use crate::{wasmedge, Config};
use std::path::Path;
use thiserror::Error;
use wasmedge_sys as sys;

#[derive(Debug)]
pub struct Module {
    pub(crate) inner: sys::Module,
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

    // TODO imports_iter
    pub fn import_iter(&self) -> impl Iterator<Item = sys::Import> {
        self.inner.imports_iter()
    }

    // TODO exports_iter
    pub fn export_iter(&self) -> impl Iterator<Item = sys::Export> {
        self.inner.exports_iter()
    }

    pub fn get_export(&self, name: impl AsRef<str>) -> Option<sys::ExternalType> {
        let exports = self.inner.exports_iter();
        let exports = exports
            .filter(|x| x.name() == name.as_ref())
            .collect::<Vec<_>>();
        match exports.is_empty() {
            true => None,
            false => Some(exports[0].ty()),
        }
    }
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
