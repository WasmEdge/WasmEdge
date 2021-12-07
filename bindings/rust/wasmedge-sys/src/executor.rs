use super::wasmedge;
use crate::{
    error::{check, Error, WasmEdgeResult},
    string::StringRef,
    Config, ImportObj, Module, Statistics, Store, Value,
};
use std::ptr;

pub struct Executor {
    ctx: *mut wasmedge::WasmEdge_ExecutorContext,
}
impl Executor {
    pub fn create(conf: Option<&Config>, stat: Option<&mut Statistics>) -> WasmEdgeResult<Self> {
        let conf = match conf {
            Some(conf) => conf.ctx,
            None => ptr::null(),
        };
        let stat_ctx = match stat {
            Some(stat) => {
                let stat_ctx = stat.ctx;
                stat.ctx = std::ptr::null_mut();
                stat_ctx
            }
            None => ptr::null_mut(),
        };
        let raw = unsafe { wasmedge::WasmEdge_ExecutorCreate(conf, stat_ctx) };
        match raw.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create Executor instance",
            ))),
            false => Ok(Executor { ctx: raw }),
        }
    }

    /// Register and instantiate WasmEdge import object into a store.
    pub fn register_import_object(
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

    /// Register and instantiate WasmEdge AST Module into a store.
    pub fn register_module(
        self,
        store: &mut Store,
        ast_mod: &mut Module,
        mod_name: impl AsRef<str>,
    ) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_ExecutorRegisterModule(
                self.ctx,
                store.ctx,
                ast_mod.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
            ))?;
            ast_mod.ctx = std::ptr::null_mut();
            ast_mod.registered = true;
        }
        Ok(self)
    }

    /// Instantiate WasmEdge AST Module into a store.
    pub fn instantiate(self, store: &mut Store, ast_mod: &mut Module) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_ExecutorInstantiate(
                self.ctx,
                store.ctx,
                ast_mod.ctx,
            ))?;
            ast_mod.ctx = std::ptr::null_mut();
            ast_mod.registered = false;
        }
        Ok(self)
    }

    /// Invoke a WASM function by name.
    pub fn invoke_function(
        &self,
        store: &Store,
        func_name: impl AsRef<str>,
        params: impl Iterator<Item = Value>,
    ) -> WasmEdgeResult<impl Iterator<Item = Value>> {
        let raw_params = params
            .map(wasmedge::WasmEdge_Value::from)
            .collect::<Vec<_>>();

        // get the length of the function's returns
        let returns_len = store
            .find_func(func_name.as_ref())
            .ok_or_else(|| {
                Error::OperationError(format!(
                    "fail to find the function named '{}'",
                    func_name.as_ref()
                ))
            })?
            .get_type()?
            .returns_len();
        let mut returns = Vec::with_capacity(returns_len);

        unsafe {
            check(wasmedge::WasmEdge_ExecutorInvoke(
                self.ctx,
                store.ctx,
                StringRef::from(func_name.as_ref()).into(),
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len as u32,
            ))?;
            returns.set_len(returns_len);
        }

        Ok(returns.into_iter().map(Into::into))
    }

    /// Invoke a WASM function by its module name and function name.
    pub fn invoke_registered_function(
        &self,
        store: &Store,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
        params: impl Iterator<Item = Value>,
    ) -> WasmEdgeResult<impl Iterator<Item = Value>> {
        let raw_params = params
            .map(wasmedge::WasmEdge_Value::from)
            .collect::<Vec<_>>();

        // get the length of the function's returns
        let returns_len = store
            .find_func_registered(mod_name.as_ref(), func_name.as_ref())
            .ok_or_else(|| {
                Error::OperationError(format!(
                    "fail to find the registered function named '{}'",
                    func_name.as_ref()
                ))
            })?
            .get_type()?
            .returns_len();
        let mut returns = Vec::with_capacity(returns_len);

        unsafe {
            check(wasmedge::WasmEdge_ExecutorInvokeRegistered(
                self.ctx,
                store.ctx,
                StringRef::from(mod_name.as_ref()).into(),
                StringRef::from(func_name.as_ref()).into(),
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len as u32,
            ))?;
            returns.set_len(returns_len);
        }

        Ok(returns.into_iter().map(Into::into))
    }
}
impl Drop for Executor {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_ExecutorDelete(self.ctx) }
        }
    }
}
