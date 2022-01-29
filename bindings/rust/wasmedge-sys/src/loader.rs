//! Defines WasmEdge Loader struct.

use crate::{error::check, utils, wasmedge, Config, Module, WasmEdgeError, WasmEdgeResult};
use std::path::Path;

/// Struct of WasmEdge Loader.
///
/// [`Loader`] is used to load WASM modules from the given WASM files or buffers.
#[derive(Debug)]
pub struct Loader {
    pub(crate) ctx: *mut wasmedge::WasmEdge_LoaderContext,
}
impl Loader {
    /// Create a new [`Loader`] to be associated with the given global configuration.
    ///
    /// # Arguements
    ///
    /// - `config` specifies a global configuration.
    ///
    /// # Error
    ///
    /// If fail to create a [`Loader`], then an error is returned.
    pub fn create(config: Option<Config>) -> WasmEdgeResult<Self> {
        let ctx = match config {
            Some(mut config) => {
                let ctx = unsafe { wasmedge::WasmEdge_LoaderCreate(config.ctx) };
                config.ctx = std::ptr::null_mut();
                ctx
            }
            None => unsafe { wasmedge::WasmEdge_LoaderCreate(std::ptr::null_mut()) },
        };

        match ctx.is_null() {
            true => Err(WasmEdgeError::LoaderCreate),
            false => Ok(Self { ctx }),
        }
    }

    /// Loads a WASM module from a WASM file.
    ///
    /// # Arguments
    ///
    /// - `path` specifies the file path to the target WASM file.
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.
    pub fn from_file(&self, path: impl AsRef<Path>) -> WasmEdgeResult<Module> {
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
            true => Err(WasmEdgeError::ModuleCreate),
            false => Ok(Module {
                ctx: mod_ctx,
                registered: false,
            }),
        }
    }

    /// Loads a WASM module from a buffer.
    ///
    /// # Arguments
    ///
    /// - `buffer` specifies a WASM buffer.
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.
    pub fn from_buffer(&self, buffer: impl AsRef<[u8]>) -> WasmEdgeResult<Module> {
        let mut mod_ctx: *mut wasmedge::WasmEdge_ASTModuleContext = std::ptr::null_mut();

        unsafe {
            let ptr = libc::malloc(buffer.as_ref().len());
            let dst = ::core::slice::from_raw_parts_mut(
                ptr.cast::<std::mem::MaybeUninit<u8>>(),
                buffer.as_ref().len(),
            );
            let src = ::core::slice::from_raw_parts(
                buffer.as_ref().as_ptr().cast::<std::mem::MaybeUninit<u8>>(),
                buffer.as_ref().len(),
            );
            dst.copy_from_slice(src);

            check(wasmedge::WasmEdge_LoaderParseFromBuffer(
                self.ctx,
                &mut mod_ctx,
                ptr as *const u8,
                buffer.as_ref().len() as u32,
            ))?;

            libc::free(ptr as *mut libc::c_void);
        }

        match mod_ctx.is_null() {
            true => Err(WasmEdgeError::ModuleCreate),
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
    use crate::{
        error::{CoreError, CoreLoadError},
        Config, WasmEdgeError,
    };

    #[test]
    fn test_loader() {
        // create a Loader instance without configuration
        let result = Loader::create(None);
        assert!(result.is_ok());

        // create a Loader instance with configuration
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let config = config.reference_types(true);
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        // load from file
        {
            let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
            let result = loader.from_file(path);
            assert!(result.is_ok());
            let module = result.unwrap();
            assert!(!module.ctx.is_null());

            let result = loader.from_file("not_exist_file");
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Core(CoreError::Load(CoreLoadError::IllegalPath))
            );
        }

        // load from buffer
        {
            let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
            let result = std::fs::read(path);
            assert!(result.is_ok());
            let buffer = result.unwrap();

            let result = loader.from_buffer(&buffer);
            assert!(result.is_ok());
            let module = result.unwrap();
            assert!(!module.ctx.is_null());

            let result = loader.from_buffer(&[]);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Core(CoreError::Load(CoreLoadError::UnexpectedEnd))
            );
        }
    }
}
