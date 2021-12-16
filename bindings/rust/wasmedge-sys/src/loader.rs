use crate::{error::check, utils, wasmedge, Config, Error, Module, WasmEdgeResult};
use std::path::Path;

#[derive(Debug)]
pub struct Loader {
    pub(crate) ctx: *mut wasmedge::WasmEdge_LoaderContext,
}
impl Loader {
    /// Create a Loader instance
    pub fn create(config: Option<&Config>) -> WasmEdgeResult<Self> {
        let config_ctx = match config {
            Some(config) => config.ctx,
            None => std::ptr::null(),
        };
        let ctx = unsafe { wasmedge::WasmEdge_LoaderCreate(config_ctx) };

        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create Loader instance",
            ))),
            false => Ok(Self { ctx }),
        }
    }

    /// Load a WASM module from a WASM file
    pub fn from_file(&self, path: impl AsRef<Path>) -> WasmEdgeResult<Module> {
        if !path.as_ref().exists() {
            return Err(Error::OperationError(format!(
                "Not found file: {}",
                path.as_ref().to_string_lossy()
            )));
        }

        let c_path = utils::path_to_cstring(path.as_ref())?;
        let mut mod_ctx = std::ptr::null_mut();
        unsafe {
            check(wasmedge::WasmEdge_LoaderParseFromFile(
                self.ctx,
                &mut mod_ctx,
                c_path.as_ptr(),
            ))?;
        }

        match mod_ctx.is_null() {
            true => Err(Error::OperationError(format!(
                "fail to load wasm module from {}",
                path.as_ref().to_string_lossy()
            ))),
            false => Ok(Module {
                ctx: mod_ctx,
                registered: false,
            }),
        }
    }

    /// Load and parse the WASM module from a buffer
    pub fn from_buffer(&self, buffer: &[u8]) -> WasmEdgeResult<Module> {
        let mut mod_ctx = std::ptr::null_mut();
        unsafe {
            check(wasmedge::WasmEdge_LoaderParseFromBuffer(
                self.ctx,
                &mut mod_ctx,
                buffer.as_ptr(),
                buffer.len() as u32,
            ))?;
        }

        match mod_ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to load wasm module from the buffer",
            ))),
            false => Ok(Module {
                ctx: mod_ctx,
                registered: false,
            }),
        }
    }
}
impl Drop for Loader {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_LoaderDelete(self.ctx) }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Loader;
    use crate::Config;

    #[test]
    fn test_loader_create() {
        // create a Loader instance without configuration
        let result = Loader::create(None);
        assert!(result.is_ok());

        // create a Loader instance with configuration
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let config = config.enable_referencetypes(true);
        let result = Loader::create(Some(&config));
        assert!(result.is_ok());
    }

    #[test]
    fn test_loader_parse_from_file() {
        // create a Loader instance with configuration
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let config = config.enable_referencetypes(true);
        let result = Loader::create(Some(&config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        let path =
            std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("test/api/apiTestData/test.wasm");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.ctx.is_null());
    }

    #[test]
    fn test_loader_parse_from_buffer() {
        // create a Loader instance with configuration
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let config = config.enable_referencetypes(true);
        let result = Loader::create(Some(&config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        let path =
            std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("test/api/apiTestData/test.wasm");
        let result = std::fs::read(path);
        assert!(result.is_ok());
        let buffer = result.unwrap();

        let result = loader.from_buffer(&buffer);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.ctx.is_null());
    }
}
