//! Defines WasmEdge Validator struct.

use crate::{error::check, wasmedge, Config, Error, Module, WasmEdgeResult};

/// Struct of WasmEdge Validator.
#[derive(Debug)]
pub struct Validator {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ValidatorContext,
}
impl Validator {
    /// Creates a new [`Validator`] to be associated with the given global configuration.
    ///
    /// # Arguments
    ///
    /// - `config` specifies the global environment configuration.
    ///
    /// # Error
    ///
    /// If fail to create a [`Validator`], then an error is returned.
    pub fn create(config: Option<&Config>) -> WasmEdgeResult<Self> {
        let config_ctx = match config {
            Some(config) => config.ctx,
            None => std::ptr::null_mut(),
        };
        let ctx = unsafe { wasmedge::WasmEdge_ValidatorCreate(config_ctx) };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create Validator instance",
            ))),
            false => Ok(Self { ctx }),
        }
    }

    /// Validates a given WasmEdge [`Module`].
    ///
    /// [`Module`]s are valid when all components they contain are valid. Furthermore, most
    /// definitions are themselves classified with a suitable type.
    ///
    /// # Arguments
    ///
    /// - `module` specifies the [`Module`] to be validated.
    ///
    /// # Error
    ///
    /// If the validation fails, then an error is returned.
    pub fn validate(&self, module: &Module) -> WasmEdgeResult<()> {
        unsafe { check(wasmedge::WasmEdge_ValidatorValidate(self.ctx, module.ctx)) }
    }
}
impl Drop for Validator {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_ValidatorDelete(self.ctx) }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{Config, Loader};

    #[test]
    fn test_validator() {
        // create a Loader instance with configuration
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let config = config.enable_referencetypes(true);
        let result = Loader::create(Some(&config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        // load a WASM module
        let path =
            std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("test/api/apiTestData/test.wasm");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.ctx.is_null());

        // create a Validator instance
        let result = Validator::create(None);
        assert!(result.is_ok());
        let validator = result.unwrap();

        // validate the module loaded.
        let result = validator.validate(&module);
        assert!(result.is_ok());
    }
}
