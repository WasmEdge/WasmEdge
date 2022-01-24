use crate::{error::WasmEdgeResult, wasmedge, Func, Global, Memory, Table, Vm};
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

        // funcs in the active module
        if self.inner.func_len() > 0 {
            let func_names = self.inner.func_names().unwrap();
            for name in func_names {
                let inner = self.inner.find_func(&name)?;
                let func = Func {
                    inner,
                    name: Some(name.clone()),
                    mod_name: None,
                };
                funcs.push(func);
            }
        }

        // funcs in the registered modules
        if self.inner.reg_module_len() > 0 {
            let mod_names = self.mod_names().unwrap();
            for mod_name in mod_names {
                if self.inner.reg_func_len(&mod_name) > 0 {
                    let func_names = self.inner.reg_func_names(&mod_name).unwrap();
                    for name in func_names {
                        let inner = self.inner.find_func_registered(&mod_name, &name)?;
                        let func = Func {
                            inner,
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
            .filter(|x| x.name().is_some() && (x.name().unwrap() == name.as_ref()));

        Ok(funcs.collect())
    }

    pub fn tables(&self) -> WasmEdgeResult<Vec<Table>> {
        let mut tables = Vec::new();

        // tables in the active module
        if self.inner.table_len() > 0 {
            let table_names = self.inner.table_names().unwrap();
            for name in table_names {
                let inner = self.inner.find_table(&name)?;
                let table = Table {
                    inner,
                    name: Some(name.clone()),
                    mod_name: None,
                };
                tables.push(table);
            }
        }

        // tables in the registered modules
        if self.inner.reg_module_len() > 0 {
            let mod_names = self.mod_names().unwrap();
            for mod_name in mod_names {
                if self.inner.reg_table_len(&mod_name) > 0 {
                    let table_names = self.inner.reg_table_names(&mod_name).unwrap();
                    for name in table_names {
                        let inner = self.inner.find_table_registered(&mod_name, &name)?;
                        let table = Table {
                            inner,
                            name: Some(name.clone()),
                            mod_name: None,
                        };
                        tables.push(table);
                    }
                }
            }
        }

        Ok(tables)
    }

    pub fn table(&self, name: impl AsRef<str>) -> WasmEdgeResult<Vec<Table>> {
        let tables = self
            .tables()?
            .into_iter()
            .filter(|x| x.name().is_some() && (x.name().unwrap() == name.as_ref()));

        Ok(tables.collect())
    }

    pub fn memories(&self) -> WasmEdgeResult<Vec<Memory>> {
        let mut memories = Vec::new();

        // memories in the active module
        if self.inner.mem_len() > 0 {
            let mem_names = self.inner.mem_names().unwrap();
            for name in mem_names {
                let inner = self.inner.find_memory(&name)?;
                let memory = Memory {
                    inner,
                    name: Some(name.clone()),
                    mod_name: None,
                };
                memories.push(memory);
            }
        }

        // memories in the registered modules
        if self.inner.reg_module_len() > 0 {
            let mod_names = self.mod_names().unwrap();
            for mod_name in mod_names {
                if self.inner.reg_mem_len(&mod_name) > 0 {
                    let mem_names = self.inner.reg_mem_names(&mod_name).unwrap();
                    for name in mem_names {
                        let inner = self.inner.find_memory_registered(&mod_name, &name)?;
                        let memory = Memory {
                            inner,
                            name: Some(name.clone()),
                            mod_name: None,
                        };
                        memories.push(memory);
                    }
                }
            }
        }

        Ok(memories)
    }

    pub fn memory(&self, name: impl AsRef<str>) -> WasmEdgeResult<Vec<Memory>> {
        let memories = self
            .memories()?
            .into_iter()
            .filter(|x| x.name().is_some() && (x.name().unwrap() == name.as_ref()));

        Ok(memories.collect())
    }

    pub fn globals(&self) -> WasmEdgeResult<Vec<Global>> {
        let mut globals = Vec::new();

        // globals in the active module
        if self.inner.global_len() > 0 {
            let global_names = self.inner.global_names().unwrap();
            for name in global_names {
                let inner = self.inner.find_global(&name)?;
                let global = Global {
                    inner,
                    name: Some(name),
                    mod_name: None,
                };
                globals.push(global);
            }
        }

        // globals in the registered modules
        if self.inner.reg_module_len() > 0 {
            let mod_names = self.mod_names().unwrap();
            for mod_name in mod_names {
                if self.inner.reg_global_len(&mod_name) > 0 {
                    let global_names = self.inner.reg_global_names(&mod_name).unwrap();
                    for name in global_names {
                        let inner = self.inner.find_global_registered(&mod_name, &name)?;
                        let global = Global {
                            inner,
                            name: Some(name),
                            mod_name: Some(mod_name.clone()),
                        };
                        globals.push(global);
                    }
                }
            }
        }

        Ok(globals)
    }

    pub fn global(&self, name: impl AsRef<str>) -> WasmEdgeResult<Vec<Global>> {
        let globals = self
            .globals()?
            .into_iter()
            .filter(|x| x.name().is_some() && (x.name().unwrap() == name.as_ref()));

        Ok(globals.collect())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{ConfigBuilder, Module, SignatureBuilder, ValType, Value, VmBuilder};

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

        // TODO run the "fib" registered function
        let result = vm.run_func(None, "fib", [Value::I32(5)]);
        assert!(result.is_ok());
    }
}
