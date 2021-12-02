use crate::{wasmedge, Error, WasmEdgeResult};
use std::ops::Range;

#[derive(Debug)]
pub struct Memory {
    pub(crate) ctx: *mut wasmedge::WasmEdge_MemoryInstanceContext,
    pub(crate) registered: bool,
}
impl Memory {
    pub fn create(limit: Range<u32>) -> WasmEdgeResult<Self> {
        let mut mem_ty = MemType::create(limit)?;
        let ctx = unsafe { wasmedge::WasmEdge_MemoryInstanceCreate(mem_ty.ctx) };
        mem_ty.ctx = std::ptr::null_mut();
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create Memory instance",
            ))),
            false => Ok(Memory {
                ctx,
                registered: false,
            }),
        }
    }
}
impl Drop for Memory {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_MemoryInstanceDelete(self.ctx) };
        }
    }
}

#[derive(Debug)]
pub struct MemType {
    pub(crate) ctx: *mut wasmedge::WasmEdge_MemoryTypeContext,
    pub(crate) registered: bool,
}
impl MemType {
    pub fn create(limit: Range<u32>) -> WasmEdgeResult<Self> {
        let ctx =
            unsafe { wasmedge::WasmEdge_MemoryTypeCreate(wasmedge::WasmEdge_Limit::from(limit)) };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create MemType instance",
            ))),
            false => Ok(Self {
                ctx,
                registered: false,
            }),
        }
    }

    pub fn limit(&self) -> Range<u32> {
        let limit = unsafe { wasmedge::WasmEdge_MemoryTypeGetLimit(self.ctx) };
        Range::from(limit)
    }
}
impl Drop for MemType {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_MemoryTypeDelete(self.ctx) }
        }
    }
}
