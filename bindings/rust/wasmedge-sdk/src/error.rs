use super::wasmedge;
use thiserror::Error;

#[derive(Debug, Error)]
pub enum ModuleError {
    #[error("an unknown error occured")]
    Unknown(wasmedge::WasmEdgeError),

    #[error("`{0}` is not a valid path: {1}")]
    Path(String, Box<dyn std::error::Error + 'static + Send + Sync>),

    #[error("loader error: {}", _0.message)]
    Load(wasmedge::WasmEdgeError),
}

#[derive(Debug, Error)]
pub enum VmError {
    #[error("module loading failed: {}", _0.message)]
    ModuleLoad(wasmedge::WasmEdgeError),

    #[error("module validation failed: {}", _0.message)]
    Validate(wasmedge::WasmEdgeError),

    #[error("module instantiation failed: {}", _0.message)]
    Instantiate(wasmedge::WasmEdgeError),

    #[error("could not find function `{0}` in module")]
    MissingFunction(String),

    #[error("module execution failed: {}", _0.message)]
    Execute(wasmedge::WasmEdgeError),
}
