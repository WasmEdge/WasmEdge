use super::wasmedge;

use std::path::Path;

use crate::{error::ModuleError, Config};
use anyhow::Result;

#[derive(Debug)]
pub struct Module {
    pub inner: wasmedge::Module,
}

impl Module {
    pub fn new(config: &Config, module_path: &Path) -> Result<Self, anyhow::Error> {
        let module = wasmedge::Module::load_from_file(&config.inner, module_path)
            .map_err(ModuleError::Load)?;

        Ok(Self { inner: module })
    }
}
