use crate::wasmedge;
use crate::{
    types::{Mutability, ValType},
    Value,
};

#[derive(Debug)]
pub struct Global {
    pub(crate) ctx: *mut wasmedge::WasmEdge_GlobalInstanceContext,
    pub(crate) registered: bool,
}
impl Global {
    pub fn create(ty: &mut GlobalType, val: Value) -> Option<Self> {
        let ctx = unsafe {
            wasmedge::WasmEdge_GlobalInstanceCreate(ty.ctx, wasmedge::WasmEdge_Value::from(val))
        };
        ty.registered = true;
        match ctx.is_null() {
            true => None,
            false => Some(Self {
                ctx,
                registered: false,
            }),
        }
    }

    pub fn get_value(&self) -> Value {
        let val = unsafe { wasmedge::WasmEdge_GlobalInstanceGetValue(self.ctx) };
        Value::from(val)
    }

    pub fn set_value(&mut self, val: Value) {
        unsafe {
            wasmedge::WasmEdge_GlobalInstanceSetValue(self.ctx, wasmedge::WasmEdge_Value::from(val))
        }
    }
}
impl Drop for Global {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_GlobalInstanceDelete(self.ctx) };
        }
    }
}

#[derive(Debug)]
pub struct GlobalType {
    pub(crate) ctx: *mut wasmedge::WasmEdge_GlobalTypeContext,
    pub(crate) registered: bool,
}
impl GlobalType {
    pub fn create(val_ty: ValType, mutable: Mutability) -> Option<Self> {
        let ctx = unsafe {
            wasmedge::WasmEdge_GlobalTypeCreate(
                wasmedge::WasmEdge_ValType::from(val_ty),
                wasmedge::WasmEdge_Mutability::from(mutable),
            )
        };
        match ctx.is_null() {
            true => None,
            false => Some(Self {
                ctx,
                registered: false,
            }),
        }
    }
}
impl Drop for GlobalType {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_GlobalTypeDelete(self.ctx) }
        }
    }
}
