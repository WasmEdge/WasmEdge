use crate::wasmedge;
use thiserror::Error;

pub type Result<T> = std::result::Result<T, WasmEdgeError>;

#[derive(Error, Clone, Debug, PartialEq)]
pub enum WasmEdgeError {
    #[error("{0}")]
    Operation(wasmedge::WasmEdgeError),
    #[error("Unknown error")]
    Unknown,
}
impl From<wasmedge::WasmEdgeError> for WasmEdgeError {
    fn from(error: wasmedge::WasmEdgeError) -> Self {
        WasmEdgeError::Operation(error)
    }
}
