//! Defines WasmEdge Instancestruct.

use crate::{
    error::InstanceError, types::WasmEdgeString, wasmedge, Function, Store, WasmEdgeError,
    WasmEdgeResult,
};
// use std::marker::PhantomData;

pub struct Instance<'store> {
    pub(crate) inner: InnerInstance,
    pub(crate) store: &'store Store,
}
impl<'store> Instance<'store> {
    /// Returns the name of this exported module instance.
    ///
    /// If this module is an active module, return None.
    pub fn name(&self) -> Option<String> {
        let name =
            unsafe { wasmedge::WasmEdge_ModuleInstanceGetModuleName(self.inner.0 as *const _) };

        let name: String = name.into();
        if name.len() == 0 {
            return None;
        }

        Some(name)
    }

    pub fn find_func(&self, name: impl AsRef<str>) -> WasmEdgeResult<Function> {
        let func_name: WasmEdgeString = name.as_ref().into();
        let func_ctx = unsafe {
            wasmedge::WasmEdge_ModuleInstanceFindFunction(
                self.inner.0,
                self.store.ctx,
                func_name.as_raw(),
            )
        };
        match func_ctx.is_null() {
            true => Err(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
                name.as_ref().to_string(),
            ))),
            false => Ok(Function {
                ctx: func_ctx,
                registered: true,
            }),
        }
    }
}

pub(crate) struct InnerInstance(pub(crate) *const wasmedge::WasmEdge_ModuleInstanceContext);
unsafe impl Send for InnerInstance {}
unsafe impl Sync for InnerInstance {}
