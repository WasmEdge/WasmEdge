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

    /// Returns all exported functions in the store, including both registered and non-registered.
    pub fn functions(&self) -> WasmEdgeResult<Vec<Func>> {
        let mut funcs = Vec::new();

        // funcs
        if self.inner.func_len() > 0 {
            let func_names = self.inner.func_names().unwrap();
            for name in func_names {
                let func = self.inner.find_func(&name)?;
                let func = Func {
                    inner: func,
                    name: Some(name),
                    mod_name: None,
                };
                funcs.push(func);
            }
        }

        // registered funcs
        if self.inner.reg_module_len() > 0 {
            let mod_names = self.mod_names().unwrap();
            for mod_name in mod_names {
                if self.inner.reg_func_len(&mod_name) > 0 {
                    let func_names = self.inner.reg_func_names(&mod_name).unwrap();
                    for name in func_names {
                        let func = self.inner.find_func_registered(&mod_name, &name)?;
                        let func = Func {
                            inner: func,
                            name: Some(name),
                            mod_name: Some(mod_name.clone()),
                        };
                        funcs.push(func);
                    }
                }
            }
        }

        Ok(funcs)
    }

    pub fn function(&self, name: impl AsRef<str>) -> WasmEdgeResult<Vec<Func>> {
        let funcs = self
            .functions()?
            .into_iter()
            .filter(|x| x.name().expect("Found anonymous function") == name.as_ref());

        Ok(funcs.collect())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{ConfigBuilder, Module, SignatureBuilder, ValType, VmBuilder};

    #[test]
    fn test_store_functions() {
        // create a Config
        let config = ConfigBuilder::new()
            .expect("fail to create a ConfigBuilder")
            .build();

        // create a Store
        let result = Store::new();
        assert!(result.is_ok());
        let store = result.unwrap();

        // create a Vm context
        let result = VmBuilder::new()
            .with_config(&config)
            .with_store(&store)
            .build();
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register a wasm module from a file
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("tools/wasmedge/examples/fibonacci.wasm");
        let result = Module::from_file(Some(&config), file);
        assert!(result.is_ok());
        let mut module = result.unwrap();
        let result = vm.register_wasm_from_module("fib-module", &mut module);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // get store
        let result = vm.store_mut();
        assert!(result.is_ok());
        let store = result.unwrap();

        // get all Func instances in the store
        let result = store.functions();
        assert!(result.is_ok());
        let funcs = result.unwrap();
        assert_eq!(funcs.len(), 1);
        assert!(funcs[0].registered());
        assert_eq!(funcs[0].mod_name().unwrap(), "fib-module");
        assert_eq!(funcs[0].name().unwrap(), "fib");
        let result = funcs[0].signature();
        assert!(result.is_ok());
        let signature = result.unwrap();
        assert_eq!(
            signature,
            SignatureBuilder::new()
                .with_args([ValType::I32])
                .with_returns([ValType::I32])
                .build()
        );
    }
}
