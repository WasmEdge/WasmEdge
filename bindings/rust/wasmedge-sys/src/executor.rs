use super::wasmedge;
use crate::{
    error::{check, WasmEdgeResult},
    string::StringRef,
    Config, ImportObj, Module, Statistics, Store,
};
use std::ptr;

pub struct Executor {
    ctx: *mut wasmedge::WasmEdge_ExecutorContext,
}
impl Executor {
    pub fn create(conf: Option<&Config>, stat: Option<&mut Statistics>) -> Option<Self> {
        let conf = match conf {
            Some(conf) => conf.ctx,
            None => ptr::null(),
        };
        let stat = match stat {
            Some(stat) => stat.ctx,
            None => ptr::null_mut(),
        };
        let raw = unsafe { wasmedge::WasmEdge_ExecutorCreate(conf, stat) };
        match raw.is_null() {
            true => None,
            false => Some(Executor { ctx: raw }),
        }
    }

    pub fn register_import_object_module(
        self,
        store: &mut Store,
        imp_obj: &ImportObj,
    ) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_ExecutorRegisterImport(
                self.ctx,
                store.ctx,
                imp_obj.ctx,
            ))?;
        }
        Ok(self)
    }

    pub fn register_ast_module(
        self,
        store: &mut Store,
        ast_mod: &Module,
        mod_name: impl AsRef<str>,
    ) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_ExecutorRegisterModule(
                self.ctx,
                store.ctx,
                ast_mod.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
            ))?;
        }
        Ok(self)
    }

    pub fn instantiate(self, store: &mut Store, ast_mod: &Module) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_ExecutorInstantiate(
                self.ctx,
                store.ctx,
                ast_mod.ctx,
            ))?;
        }
        Ok(self)
    }
}
impl Drop for Executor {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_ExecutorDelete(self.ctx) }
        }
    }
}
