use crate::wasmedge;
use thiserror::Error;

pub type Result<T> = std::result::Result<T, WasmEdgeError>;

#[derive(Error, Clone, Debug, PartialEq)]
pub enum WasmEdgeError {
    #[error("{0}")]
    Operation(wasmedge::error::WasmEdgeError),
    #[error("Unknown error")]
    Unknown,
}
impl From<wasmedge::error::WasmEdgeError> for WasmEdgeError {
    fn from(error: wasmedge::error::WasmEdgeError) -> Self {
        WasmEdgeError::Operation(error)
    }
}
