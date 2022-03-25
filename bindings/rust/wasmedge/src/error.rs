use crate::sys;
use thiserror::Error;

pub type Result<T> = std::result::Result<T, WasmEdgeError>;

#[derive(Error, Clone, Debug, PartialEq)]
pub enum WasmEdgeError {
    #[error("{0}")]
    Operation(sys::error::WasmEdgeError),
    #[error("Unknown error")]
    Unknown,
}
impl From<sys::error::WasmEdgeError> for WasmEdgeError {
    fn from(error: sys::error::WasmEdgeError) -> Self {
        WasmEdgeError::Operation(error)
    }
}
