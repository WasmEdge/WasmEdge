use super::wasmedge;
use crate::{
    raw_result::{check, WasmEdgeResult},
    utils,
};
use std::path::Path;

#[derive(Debug)]
pub struct Module {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ASTModuleContext,
}

impl Drop for Module {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_ASTModuleDelete(self.ctx) };
        }
    }
}

impl Module {
    pub fn load_from_file<P: AsRef<Path>>(
        config: &crate::config::Config,
        path: P,
    ) -> WasmEdgeResult<Self> {
        let loader_ctx = unsafe { wasmedge::WasmEdge_LoaderCreate(config.ctx) };
        let mut ctx: *mut wasmedge::WasmEdge_ASTModuleContext = std::ptr::null_mut();

        let path = utils::path_to_cstring(path.as_ref())?;

        unsafe {
            check(wasmedge::WasmEdge_LoaderParseFromFile(
                loader_ctx,
                &mut ctx as *mut _,
                path.as_ptr(),
            ))?;
        }

        assert!(!ctx.is_null(), "WasmEdge failed to load from file!");

        Ok(Self { ctx })
    }

    pub fn load_from_buffer(config: &crate::config::Config, buffer: &[u8]) -> WasmEdgeResult<Self> {
        let loader_ctx = unsafe { wasmedge::WasmEdge_LoaderCreate(config.ctx) };
        let mut ctx: *mut wasmedge::WasmEdge_ASTModuleContext = std::ptr::null_mut();

        unsafe {
            check(wasmedge::WasmEdge_LoaderParseFromBuffer(
                loader_ctx,
                &mut ctx as *mut _,
                buffer.as_ptr(),
                buffer.len() as u32,
            ))?;
        }

        assert!(!ctx.is_null(), "WasmEdge failed to load from buffer!");

        Ok(Self { ctx })
    }
}

#[cfg(test)]
mod tests {
    use super::Module;
    use crate::Config;

    #[test]
    fn test_module_from_buffer() {
        let wasm_path = std::path::PathBuf::from(env!("WASMEDGE_SRC_DIR"))
            .join("test/api/apiTestData/test.wasm");
        let result = std::fs::read(wasm_path);
        assert!(result.is_ok());
        let buf = result.unwrap();

        let conf = Config::default().enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        let result = Module::load_from_buffer(&conf, &buf);
        assert!(result.is_ok());
    }

    #[test]
    fn test_module_from_file() {
        let path = std::path::PathBuf::from(env!("WASMEDGE_SRC_DIR"))
            .join("test/api/apiTestData/test.wasm");

        let conf = Config::default().enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        let result = Module::load_from_file(&conf, path);
        assert!(result.is_ok());
    }
}
