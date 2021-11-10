use crate::wasmedge;

pub struct Global {
    ctx: *mut wasmedge::WasmEdge_GlobalInstanceContext,
}

impl Drop for Global {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_GlobalInstanceDelete(self.ctx) };
    }
}
