use super::wasmedge;
use crate::raw_result::{decode_result, ErrReport};
use ::core::mem::MaybeUninit as MU;
use std::ffi::CString;

#[derive(Debug)]
pub struct Module {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ASTModuleContext,
}

impl Drop for Module {
    fn drop(&mut self) {
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

        assert!(!ctx.is_null(), "WasmEdge failed to load from file!");

        Ok(Self { ctx })
    }

    pub fn load_from_buffer(
        config: &crate::config::Config,
        buffer: Vec<u8>,
    ) -> Result<Self, ErrReport> {
        let loader_ctx = unsafe { wasmedge::WasmEdge_LoaderCreate(config.ctx) };
        let mut ctx: *mut wasmedge::WasmEdge_ASTModuleContext = std::ptr::null_mut();

        assert!(!buffer.is_empty(), "WasmEdge fail to load an empty buffer");

        let ptr = unsafe {
            let ptr = libc::malloc(buffer.len());
            let dst = ::core::slice::from_raw_parts_mut(ptr.cast::<MU<u8>>(), buffer.len());
            let src = ::core::slice::from_raw_parts(buffer.as_ptr().cast::<MU<u8>>(), buffer.len());
            dst.copy_from_slice(src);
            ptr
        };

        let res = unsafe {
            wasmedge::WasmEdge_LoaderParseFromBuffer(
                loader_ctx,
                &mut ctx as *mut _,
                ptr as *const u8,
                buffer.len() as u32,
            )
        };

        unsafe {
            libc::free(ptr as *mut libc::c_void);
        }

        decode_result(res)?;

        assert!(!ctx.is_null(), "WasmEdge failed to load from buffer!");

        Ok(Self { ctx })
    }
}
