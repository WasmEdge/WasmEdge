use std::path::Path;
use thiserror::Error;
use wasmedge_sys as sys;

pub struct Module {
    inner: sys::Module,
}
impl Module {
    /// load + validate
    pub fn from_file(engine: &impl Engine, file: impl AsRef<Path>) -> Result<Self, WasmError> {
        todo!()
    }

    /// load + validate
    pub fn from_binary(engine: &impl Engine, binary: &[u8]) -> Result<Self, WasmError> {
        todo!()
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
