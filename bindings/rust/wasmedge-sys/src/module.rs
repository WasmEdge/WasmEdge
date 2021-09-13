use super::wasmedge;
use crate::raw_result::{decode_result, ErrReport};
use std::ffi::CString;

// If there is a third-party sdk based on wasmedge-sys
// the private encapsulation  here can force the third-party sdk to use the api
#[derive(Debug)]
pub struct Module {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ASTModuleContext,
    _private: (),
}

impl Drop for Module {
    fn drop(&mut self) {
        println!("module drop!");
        unsafe { wasmedge::WasmEdge_ASTModuleDelete(self.ctx) };
    }
}

impl Module {
    pub fn load_from_file(
        config: &crate::config::Config,
        path: CString,
    ) -> Result<Self, ErrReport> {
        let loader_ctx = unsafe { wasmedge::WasmEdge_LoaderCreate(config.ctx) };
        let mut ctx: *mut wasmedge::WasmEdge_ASTModuleContext = std::ptr::null_mut();

        let res = unsafe {
            wasmedge::WasmEdge_LoaderParseFromFile(loader_ctx, &mut ctx as *mut _, path.as_ptr())
        };
        decode_result(res)?;

        if ctx.is_null() {
            Err(ErrReport::default())
        } else {
            Ok(Self { ctx, _private: () })
        }
    }
}
