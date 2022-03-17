//! Defines WasmEdge Validator struct.

use crate::{
    error::{check, WasmEdgeError},
    wasmedge, Config, Module, WasmEdgeResult,
};

/// Struct of WasmEdge Validator.
#[derive(Debug)]
pub struct Validator {
    pub(crate) inner: InnerValidator,
    pub(crate) registered: bool,
}
impl Validator {
    /// Creates a new [Validator] to be associated with the given global configuration.
    ///
    /// # Arguments
    ///
    /// - `config` specifies the global environment configuration.
    ///
    /// # Error
    ///
    /// If fail to create a [Validator], then an error is returned.
    pub fn create(config: Option<Config>) -> WasmEdgeResult<Self> {
        let ctx = match config {
            Some(mut config) => {
                let ctx = unsafe { wasmedge::WasmEdge_ValidatorCreate(config.inner.0) };
                config.inner.0 = std::ptr::null_mut();
                ctx
            }
            None => unsafe { wasmedge::WasmEdge_ValidatorCreate(std::ptr::null_mut()) },
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::CompilerCreate),
            false => Ok(Self {
                inner: InnerValidator(ctx),
                registered: false,
            }),
        }
    }

    /// Validates a given WasmEdge [Module].
    ///
    /// [Module]s are valid when all components they contain are valid. Furthermore, most
    /// definitions are themselves classified with a suitable type.
    ///
    /// # Arguments
    ///
    /// - `module` specifies the [Module] to be validated.
    ///
    /// # Error
    ///
    /// If the validation fails, then an error is returned.
    pub fn validate(&self, module: &Module) -> WasmEdgeResult<()> {
        unsafe {
            check(wasmedge::WasmEdge_ValidatorValidate(
                self.inner.0,
                module.inner.0,
            ))
        }
    }
}
impl Drop for Validator {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe { wasmedge::WasmEdge_ValidatorDelete(self.inner.0) }
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerValidator(pub(crate) *mut wasmedge::WasmEdge_ValidatorContext);
unsafe impl Send for InnerValidator {}
unsafe impl Sync for InnerValidator {}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{Config, Loader};
    use std::{
        sync::{Arc, Mutex},
        thread,
    };

    #[test]
    fn test_validator() {
        // create a Validator context without configuration
        let result = Validator::create(None);
        assert!(result.is_ok());

        // create a Loader context with configuration
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.reference_types(true);
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        // load a WASM module
        let path =
            std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("test/api/apiTestData/test.wasm");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.inner.0.is_null());

        // create a Validator context without configuration
        let result = Validator::create(None);
        assert!(result.is_ok());
        let validator = result.unwrap();

        // validate the module loaded.
        let result = validator.validate(&module);
        assert!(result.is_ok());
    }

    #[test]
    fn test_validator_send() {
        // create a Validator
        let result = Validator::create(None);
        assert!(result.is_ok());
        let validator = result.unwrap();

        let handle = thread::spawn(move || {
            let result = Loader::create(None);
            assert!(result.is_ok());
            let loader = result.unwrap();

            // load a WASM module
            let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("test/api/apiTestData/test.wasm");
            let result = loader.from_file(path);
            assert!(result.is_ok());
            let module = result.unwrap();
            assert!(!module.inner.0.is_null());

            // validate the module loaded.
            let result = validator.validate(&module);
            assert!(result.is_ok());
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_validator_sync() {
        // create a Validator
        let result = Validator::create(None);
        assert!(result.is_ok());
        let validator = Arc::new(Mutex::new(result.unwrap()));

        let validator_cloned = Arc::clone(&validator);
        let handle = thread::spawn(move || {
            let result = Loader::create(None);
            assert!(result.is_ok());
            let loader = result.unwrap();

            // load a WASM module
            let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("test/api/apiTestData/test.wasm");
            let result = loader.from_file(path);
            assert!(result.is_ok());
            let module = result.unwrap();
            assert!(!module.inner.0.is_null());

            // validate the module loaded.
            let result = validator_cloned.lock();
            assert!(result.is_ok());
            let validator = result.unwrap();
            let result = validator.validate(&module);
            assert!(result.is_ok());
        });

        handle.join().unwrap();
    }
}
