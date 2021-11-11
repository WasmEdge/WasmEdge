use super::wasmedge;
use crate::{instance::Function, string::StringRef};

#[derive(Debug)]
pub struct ImportObj {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ImportObjectContext,
}

impl ImportObj {
    pub fn create(module_name: impl AsRef<str>) -> Self {
        let raw_module_name: wasmedge::WasmEdge_String =
            StringRef::from(module_name.as_ref()).into();
        let ctx = unsafe { wasmedge::WasmEdge_ImportObjectCreate(raw_module_name) };
        ImportObj { ctx }
    }

    pub fn add_func(&mut self, func_name: impl AsRef<str>, func: &mut Function) {
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(func_name.as_ref()).into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddFunction(self.ctx, raw_func_name, (*func).ctx);
        }
        func.registed = true;
    }
}

impl Drop for ImportObj {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_ImportObjectDelete(self.ctx) };
    }
}
