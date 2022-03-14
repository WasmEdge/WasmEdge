use crate::{error::Result, wasmedge, Config};
use std::path::Path;

#[derive(Debug)]
pub struct Compiler {
    pub(crate) inner: wasmedge::Compiler,
}
impl Compiler {
    pub fn new(config: Option<&Config>) -> Result<Self> {
        let inner_config = match config {
            Some(config) => Some(Config::copy_from(config)?.inner),
            None => None,
        };
        let inner = wasmedge::Compiler::create(inner_config)?;

        Ok(Self { inner })
    }

    pub fn compile(&self, in_path: impl AsRef<Path>, out_path: impl AsRef<Path>) -> Result<()> {
        self.inner.compile(in_path, out_path)?;

        Ok(())
    }
}
