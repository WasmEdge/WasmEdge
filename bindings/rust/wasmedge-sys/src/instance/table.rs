use crate::wasmedge;

pub struct Table {
    ctx: *mut wasmedge::WasmEdge_TableInstanceContext,
}

impl Drop for Table {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_TableInstanceDelete(self.ctx) };
    }
}
