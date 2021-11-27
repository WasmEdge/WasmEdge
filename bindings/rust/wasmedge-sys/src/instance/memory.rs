use crate::types::Limit;
use crate::wasmedge;

#[derive(Debug)]
pub struct Memory {
    pub(crate) ctx: *mut wasmedge::WasmEdge_MemoryInstanceContext,
    pub(crate) registered: bool,
}
impl Memory {
    pub fn create(limit: Limit) -> Option<Self> {
        let mut mem_ty = MemType::create(limit)?;
        let ctx = unsafe { wasmedge::WasmEdge_MemoryInstanceCreate(mem_ty.ctx) };
        mem_ty.ctx = std::ptr::null_mut();
        match ctx.is_null() {
            true => None,
            false => Some(Memory {
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
    pub fn create(limit: Limit) -> Option<Self> {
        let ctx =
            unsafe { wasmedge::WasmEdge_MemoryTypeCreate(wasmedge::WasmEdge_Limit::from(limit)) };
        match ctx.is_null() {
            true => None,
            false => Some(Self {
                ctx,
                registered: false,
            }),
        }
    }
}
impl Drop for MemType {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_MemoryTypeDelete(self.ctx) }
        }
    }
}
