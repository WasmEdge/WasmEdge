use crate::{error::WasmEdgeResult, wasmedge, Func, Vm};
use std::marker::PhantomData;

#[derive(Debug)]
pub struct Store<'vm> {
    pub(crate) inner: wasmedge::Store,
    pub(crate) _marker: PhantomData<&'vm Vm>,
}
impl<'vm> Store<'vm> {
    pub fn new() -> WasmEdgeResult<Self> {
        let inner = wasmedge::Store::create()?;
        Ok(Self {
            inner,
            _marker: PhantomData,
        })
    }

    /// Returns the names of all registered [modules](crate::Module)
    pub fn mod_names(&self) -> Option<Vec<String>> {
        self.inner.reg_module_names()
    }

    pub fn functions(&self) -> WasmEdgeResult<Vec<Func>> {
        let mut funcs = Vec::new();

        // funcs
        if self.inner.func_len() > 0 {
            let func_names = self.inner.func_names().unwrap();
            for name in func_names {
                let func = self.inner.find_func(&name)?;
                let func = Func {
                    inner: func,
                    name: Some(name.clone()),
                    mod_name: None,
                };
                funcs.push(func);
            }
        }

        Ok(funcs)
    }
}
