use super::wasmedge;
use thiserror::Error;

#[derive(Debug, Error)]
pub enum ModuleError {
    #[error("an unknown error occured")]
    Unknown(wasmedge::Error),

    #[error("`{0}` is not a valid path: {1}")]
    Path(String, Box<dyn std::error::Error + 'static + Send + Sync>),

    #[error("loader error: {}", _0)]
    Load(wasmedge::Error),
}

#[derive(Debug, Error)]
pub enum VmError {
    #[error("module loading failed: {}", _0)]
    ModuleLoad(wasmedge::Error),

    #[error("module validation failed: {}", _0)]
    Validate(wasmedge::Error),

    #[error("module instantiation failed: {}", _0)]
    Instantiate(wasmedge::Error),

    #[error("could not find function `{0}` in module")]
    MissingFunction(String),

    #[error("module execution failed: {}", _0)]
    Execute(wasmedge::Error),
}
