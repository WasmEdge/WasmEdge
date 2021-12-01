use crate::wasmedge;

pub struct Memory {
    ctx: *mut wasmedge::WasmEdge_MemoryInstanceContext,
}

impl Drop for Memory {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_MemoryInstanceDelete(self.ctx) };
    }
}
