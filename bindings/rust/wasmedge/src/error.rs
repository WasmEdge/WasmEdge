use thiserror::Error;
use wasmedge_sys as sys;

pub type Result<T> = std::result::Result<T, WasmEdgeError>;

#[derive(Error, Clone, Debug, PartialEq)]
pub enum WasmEdgeError {
    #[error("{0}")]
    Operation(sys::error::WasmEdgeError),
    #[error("{0}")]
    Store(StoreError),
    #[error("Unknown error")]
    Unknown,
}
impl From<sys::error::WasmEdgeError> for WasmEdgeError {
    fn from(error: sys::error::WasmEdgeError) -> Self {
        WasmEdgeError::Operation(error)
    }
}

/// Defines the errors raised from [Store](crate::Store).
#[derive(Error, Clone, Debug, PartialEq)]
pub enum StoreError {
    #[error("An import object with the same name existed in the store. The name is {0}.")]
    DuplicateImport(String),
}
