//! Defines WasmEdge Validator struct.

use crate::{error::WasmEdgeError, ffi, utils::check, Config, Module, WasmEdgeResult};

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
    /// * `config` - The global environment configuration.
    ///
    /// # Error
    ///
    /// If fail to create a [Validator], then an error is returned.
    pub fn create(config: Option<&Config>) -> WasmEdgeResult<Self> {
        let ctx = match config {
            Some(config) => unsafe { ffi::WasmEdge_ValidatorCreate(config.inner.0) },
            None => unsafe { ffi::WasmEdge_ValidatorCreate(std::ptr::null_mut()) },
        };
        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::CompilerCreate)),
            false => Ok(Self {
                inner: InnerValidator(ctx),
                registered: false,
            }),
        }
    }

    /// Validates a given WasmEdge [Module](crate::Module).
    ///
    /// [Module](crate::Module)s are valid when all components they contain are valid. Furthermore, most definitions are themselves classified with a suitable type.
    ///
    /// # Arguments
    ///
    /// * `module` - The [Module](crate::Module) to be validated.
    ///
    /// # Error
    ///
    /// If the validation fails, then an error is returned.
    pub fn validate(&self, module: &Module) -> WasmEdgeResult<()> {
        unsafe {
            check(ffi::WasmEdge_ValidatorValidate(
                self.inner.0,
                module.inner.0,
            ))
        }
    }

    /// Provides a raw pointer to the inner Validator context.
    #[cfg(feature = "ffi")]
    pub fn as_ptr(&self) -> *const ffi::WasmEdge_ValidatorContext {
        self.inner.0 as *const _
    }
}
impl Drop for Validator {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_ValidatorDelete(self.inner.0) }
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerValidator(pub(crate) *mut ffi::WasmEdge_ValidatorContext);
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
    #[allow(clippy::assertions_on_result_states)]
    fn test_validator() {
        // create a Validator context without configuration
        let result = Validator::create(None);
        assert!(result.is_ok());

        // create a Loader context with configuration
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.reference_types(true);
        let result = Loader::create(Some(&config));
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
    #[allow(clippy::assertions_on_result_states)]
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
    #[allow(clippy::assertions_on_result_states)]
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
