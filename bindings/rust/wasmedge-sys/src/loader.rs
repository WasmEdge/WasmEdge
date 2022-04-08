//! Defines WasmEdge Loader struct.

use crate::{
    ast_module::{InnerModule, Module},
    error::{check, WasmEdgeError},
    ffi, utils, Config, WasmEdgeResult,
};
use std::path::Path;

/// Struct of WasmEdge Loader.
///
/// [Loader](crate::Loader) is used to load WASM modules from the given WASM files or buffers.
#[derive(Debug)]
pub struct Loader {
    pub(crate) inner: InnerLoader,
    pub(crate) registered: bool,
}
impl Loader {
    /// Create a new [Loader](crate::Loader) to be associated with the given global configuration.
    ///
    /// # Arguements
    ///
    /// - `config` specifies a global configuration.
    ///
    /// # Error
    ///
    /// If fail to create a [Loader](crate), then an error is returned.
    pub fn create(config: Option<Config>) -> WasmEdgeResult<Self> {
        let ctx = match config {
            Some(mut config) => {
                let ctx = unsafe { ffi::WasmEdge_LoaderCreate(config.inner.0) };
                config.inner.0 = std::ptr::null_mut();
                ctx
            }
            None => unsafe { ffi::WasmEdge_LoaderCreate(std::ptr::null_mut()) },
        };

        match ctx.is_null() {
            true => Err(WasmEdgeError::LoaderCreate),
            false => Ok(Self {
                inner: InnerLoader(ctx),
                registered: false,
            }),
        }
    }

    /// Loads a WASM module from a WASM file with the suffix `.wasm`.
    ///
    /// # Arguments
    ///
    /// - `file` specifies the path to the target WASM file.
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.
    ///
    /// # Example
    ///
    /// ```ignore
    /// let file = "path/to/foo.wasm"
    /// let module = loader.from_file(file)?;
    /// ```
    pub fn from_file(&self, file: impl AsRef<Path>) -> WasmEdgeResult<Module> {
        let c_path = utils::path_to_cstring(file.as_ref())?;
        let mut mod_ctx = std::ptr::null_mut();
        unsafe {
            check(ffi::WasmEdge_LoaderParseFromFile(
                self.inner.0,
                &mut mod_ctx,
                c_path.as_ptr(),
            ))?;
        }

        match mod_ctx.is_null() {
            true => Err(WasmEdgeError::ModuleCreate),
            false => Ok(Module {
                inner: InnerModule(mod_ctx),
            }),
        }
    }

    /// Loads a WASM module from a in-memory bytes.
    ///
    /// # Arguments
    ///
    /// - `buffer` specifies a WASM buffer.
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.
    ///
    /// # Example
    ///
    /// ```ignore
    /// let buffer = b"\0asm\x01\0\0\0";
    /// let module = loader.from_buffer(&buffer)?;
    /// ```
    ///
    /// Note that the text format is not accepted:
    ///
    /// ```ignore
    /// assert!(loader.from_buffer(b"(module)").is_err());
    /// ```
    pub fn from_bytes(&self, buffer: impl AsRef<[u8]>) -> WasmEdgeResult<Module> {
        let mut mod_ctx: *mut ffi::WasmEdge_ASTModuleContext = std::ptr::null_mut();

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

            check(ffi::WasmEdge_LoaderParseFromBuffer(
                self.inner.0,
                &mut mod_ctx,
                ptr as *const u8,
                buffer.as_ref().len() as u32,
            ))?;

            libc::free(ptr as *mut libc::c_void);
        }

        match mod_ctx.is_null() {
            true => Err(WasmEdgeError::ModuleCreate),
            false => Ok(Module {
                inner: InnerModule(mod_ctx),
            }),
        }
    }
}
impl Drop for Loader {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_LoaderDelete(self.inner.0) }
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerLoader(pub(crate) *mut ffi::WasmEdge_LoaderContext);
unsafe impl Send for InnerLoader {}
unsafe impl Sync for InnerLoader {}

#[cfg(test)]
mod tests {
    use super::Loader;
    use crate::{
        error::{CoreError, CoreLoadError, WasmEdgeError},
        Config,
    };
    use std::{
        sync::{Arc, Mutex},
        thread,
    };

    #[test]
    fn test_loader() {
        // create a Loader instance without configuration
        let result = Loader::create(None);
        assert!(result.is_ok());

        // create a Loader instance with configuration
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.reference_types(true);
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        // load from file
        {
            // load .wasm file
            let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
            let result = loader.from_file(path);
            assert!(result.is_ok());
            let module = result.unwrap();
            assert!(!module.inner.0.is_null());

            // Not support .wat file
            let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wat");
            let result = loader.from_file(path);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Core(CoreError::Load(CoreLoadError::MalformedMagic))
            );

            let result = loader.from_file("not_exist_file");
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Core(CoreError::Load(CoreLoadError::IllegalPath))
            );
        }

        // load from buffer
        {
            let buffer = b"\0asm\x01\0\0\0";
            let result = loader.from_bytes(&buffer);
            assert!(result.is_ok());
            let module = result.unwrap();
            assert!(!module.inner.0.is_null());

            // the text format is not accepted
            let result = loader.from_bytes(b"(module)");
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Core(CoreError::Load(CoreLoadError::MalformedMagic))
            );

            // empty is not accepted
            let result = loader.from_bytes(&[]);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Core(CoreError::Load(CoreLoadError::UnexpectedEnd))
            );
        }
    }

    #[test]
    fn test_loader_send() {
        // create a Loader instance without configuration
        let result = Loader::create(None);
        assert!(result.is_ok());

        // create a Loader instance with configuration
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.reference_types(true);
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        let handle = thread::spawn(move || {
            assert!(!loader.inner.0.is_null());
            println!("{:?}", loader.inner);
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_loader_sync() {
        // create a Loader instance without configuration
        let result = Loader::create(None);
        assert!(result.is_ok());

        // create a Loader instance with configuration
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.reference_types(true);
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = Arc::new(Mutex::new(result.unwrap()));

        let loader_cloned = Arc::clone(&loader);
        let handle = thread::spawn(move || {
            let result = loader_cloned.lock();
            assert!(result.is_ok());
            let loader = result.unwrap();

            assert!(!loader.inner.0.is_null());
        });

        handle.join().unwrap();
    }
}
