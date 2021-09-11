use super::wasmedge;
use crate::raw_result::{ErrReport, decode_result};
use std::ffi::CString;

// Since `wasmedge-sys` is also a standalone crate
// if someone else wants to rely on it to implement a third-party sdk
// then we expect that the third-party sdk will only use the interface we want it to use.
#[derive(Debug)]
pub struct Module {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ASTModuleContext,
    _private: (),
}

impl Default for Module{
    fn default() -> Self {
        Self{ ctx: std::ptr::null_mut(), _private: ()}
    }
}

struct LoaderContext(*mut wasmedge::WasmEdge_LoaderContext);


impl Drop for LoaderContext {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_LoaderDelete(self.0) };
    }
}


impl Drop for Module {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_ASTModuleDelete(self.ctx) };
    }
}

impl Module {
    fn loader_create(config: &crate::config::Config) -> Result<LoaderContext, ErrReport> {
        let loader_cxt = unsafe{ 
            wasmedge::WasmEdge_LoaderCreate(config.ctx)
        };
        if loader_cxt.is_null() {
            Err(ErrReport::default())
        } else {
            Ok(LoaderContext(loader_cxt))
        }
    }

    pub fn load_from_file(config: &crate::config::Config, path: CString ) -> Result<Self, ErrReport> {
        let loader_ctx = Self::loader_create(config)?;
        let mut module_ctx = Self::default().ctx;

        let res = unsafe{ 
            wasmedge::WasmEdge_LoaderParseFromFile(
                loader_ctx.0,
                &mut module_ctx as *mut _,
                path.as_ptr(),
            )
        };
        decode_result(res)?;

        if module_ctx.is_null() {
            Err(ErrReport::default())
        } else {
            Ok(Self{ ctx: module_ctx , _private: ()} )
        }
    }

}

