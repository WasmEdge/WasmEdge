use super::wasmedge;

#[derive(Debug)]
pub struct Store {
    pub(crate) ctx: *mut wasmedge::WasmEdge_StoreContext,
}

impl Store {
    pub fn new() -> Self {
        let ctx = unsafe { wasmedge::WasmEdge_StoreCreate() };
        Self { ctx }
    }
}

impl Default for Store {
    fn default() -> Self {
        Store::new()
    }
}

impl Drop for Store {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_StoreDelete(self.ctx) };
    }
}
