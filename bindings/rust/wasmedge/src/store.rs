use crate::{error::WasmEdgeResult, wasmedge, Func};

#[derive(Debug)]
pub struct Store {
    pub(crate) inner: wasmedge::Store,
}
impl Store {
    pub fn new() -> WasmEdgeResult<Self> {
        let inner = wasmedge::Store::create()?;
        Ok(Self { inner })
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
