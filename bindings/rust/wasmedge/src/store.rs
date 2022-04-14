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

    pub fn count_of_module(&self) -> u32 {
        self.inner.reg_module_len()
    }

    /// Returns the names of all registered [modules](crate::Module)
    pub fn module_names(&self) -> Option<Vec<String>> {
        self.inner.reg_module_names()
    }

    pub fn count_of_func(&self) -> u32 {
        self.inner.func_len()
    }

    pub fn func_len_by_module(&self, mod_name: impl AsRef<str>) -> u32 {
        self.inner.reg_func_len(mod_name)
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
            let mod_names = self.module_names().unwrap();
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

    pub fn functions_by_module(&self, mod_name: impl AsRef<str>) -> WasmEdgeResult<Vec<Func>> {
        let mut funcs = Vec::new();

        // funcs in the registered modules
        if self.inner.reg_func_len(mod_name.as_ref()) > 0 {
            let func_names = self.inner.reg_func_names(mod_name.as_ref()).unwrap();
            for name in func_names {
                let inner = self.inner.find_func_registered(mod_name.as_ref(), &name)?;
                let func = Func {
                    inner,
                    name: Some(name),
                    mod_name: Some(mod_name.as_ref().into()),
                };
                funcs.push(func);
            }
        }

        Ok(funcs)
    }

    pub fn functions_by_name(&self, func_name: impl AsRef<str>) -> WasmEdgeResult<Vec<Func>> {
        let funcs = self
            .functions()?
            .into_iter()
            .filter(|x| x.name().is_some() && (x.name().unwrap() == func_name.as_ref()));

        Ok(funcs.collect())
    }

    pub fn function(
        &self,
        func_name: impl AsRef<str>,
        mod_name: Option<&str>,
    ) -> WasmEdgeResult<Func> {
        match mod_name {
            Some(mod_name) => {
                let inner = self
                    .inner
                    .find_func_registered(mod_name, func_name.as_ref())?;
                Ok(Func {
                    inner,
                    name: Some(func_name.as_ref().into()),
                    mod_name: Some(mod_name.into()),
                })
            }
            None => {
                let inner = self.inner.find_func(func_name.as_ref())?;
                Ok(Func {
                    inner,
                    name: Some(func_name.as_ref().into()),
                    mod_name: None,
                })
            }
        }
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
                    _marker: PhantomData,
                };
                tables.push(table);
            }
        }

        // tables in the registered modules
        if self.inner.reg_module_len() > 0 {
            let mod_names = self.module_names().unwrap();
            for mod_name in mod_names {
                if self.inner.reg_table_len(&mod_name) > 0 {
                    let table_names = self.inner.reg_table_names(&mod_name).unwrap();
                    for name in table_names {
                        let inner = self.inner.find_table_registered(&mod_name, &name)?;
                        let table = Table {
                            inner,
                            name: Some(name.clone()),
                            mod_name: None,
                            _marker: PhantomData,
                        };
                        tables.push(table);
                    }
                }
            }
        }

        Ok(tables)
    }

    pub fn tables_by_module(&self, mod_name: impl AsRef<str>) -> WasmEdgeResult<Vec<Table>> {
        unimplemented!()
    }

    pub fn tables_by_name(&self, table_name: impl AsRef<str>) -> WasmEdgeResult<Vec<Table>> {
        unimplemented!()
    }

    pub fn table(
        &self,
        table_name: impl AsRef<str>,
        mod_name: Option<&str>,
    ) -> WasmEdgeResult<Table> {
        match mod_name {
            Some(mod_name) => {
                let inner = self
                    .inner
                    .find_table_registered(mod_name, table_name.as_ref())?;
                Ok(Table {
                    inner,
                    name: Some(table_name.as_ref().into()),
                    mod_name: Some(mod_name.into()),
                    _marker: PhantomData,
                })
            }
            None => {
                let inner = self.inner.find_table(table_name.as_ref())?;
                Ok(Table {
                    inner,
                    name: Some(table_name.as_ref().into()),
                    mod_name: None,
                    _marker: PhantomData,
                })
            }
        }
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
                    _marker: PhantomData,
                };
                memories.push(memory);
            }
        }

        // memories in the registered modules
        if self.inner.reg_module_len() > 0 {
            let mod_names = self.module_names().unwrap();
            for mod_name in mod_names {
                if self.inner.reg_mem_len(&mod_name) > 0 {
                    let mem_names = self.inner.reg_mem_names(&mod_name).unwrap();
                    for name in mem_names {
                        let inner = self.inner.find_memory_registered(&mod_name, &name)?;
                        let memory = Memory {
                            inner,
                            name: Some(name.clone()),
                            mod_name: None,
                            _marker: PhantomData,
                        };
                        memories.push(memory);
                    }
                }
            }
        }

        Ok(memories)
    }

    pub fn memories_by_module(&self, mod_name: impl AsRef<str>) -> WasmEdgeResult<Vec<Memory>> {
        unimplemented!()
    }

    pub fn memories_by_name(&self, mem_name: impl AsRef<str>) -> WasmEdgeResult<Vec<Memory>> {
        unimplemented!()
    }

    pub fn memory(
        &self,
        mem_name: impl AsRef<str>,
        mod_name: Option<&str>,
    ) -> WasmEdgeResult<Memory> {
        match mod_name {
            Some(mod_name) => {
                let inner = self
                    .inner
                    .find_memory_registered(mod_name, mem_name.as_ref())?;
                Ok(Memory {
                    inner,
                    name: Some(mem_name.as_ref().into()),
                    mod_name: Some(mod_name.into()),
                    _marker: PhantomData,
                })
            }
            None => {
                let inner = self.inner.find_memory(mem_name.as_ref())?;
                Ok(Memory {
                    inner,
                    name: Some(mem_name.as_ref().into()),
                    mod_name: None,
                    _marker: PhantomData,
                })
            }
        }
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
                    _marker: PhantomData,
                };
                globals.push(global);
            }
        }

        // globals in the registered modules
        if self.inner.reg_module_len() > 0 {
            let mod_names = self.module_names().unwrap();
            for mod_name in mod_names {
                if self.inner.reg_global_len(&mod_name) > 0 {
                    let global_names = self.inner.reg_global_names(&mod_name).unwrap();
                    for name in global_names {
                        let inner = self.inner.find_global_registered(&mod_name, &name)?;
                        let global = Global {
                            inner,
                            name: Some(name),
                            mod_name: Some(mod_name.clone()),
                            _marker: PhantomData,
                        };
                        globals.push(global);
                    }
                }
            }
        }

        Ok(globals)
    }

    pub fn globals_by_module(&self, mod_name: impl AsRef<str>) -> WasmEdgeResult<Vec<Global>> {
        unimplemented!()
    }

    pub fn globals_by_name(&self, global_name: impl AsRef<str>) -> WasmEdgeResult<Vec<Global>> {
        unimplemented!()
    }

    pub fn global(
        &self,
        global_name: impl AsRef<str>,
        mod_name: Option<&str>,
    ) -> WasmEdgeResult<Global> {
        match mod_name {
            Some(mod_name) => {
                let inner = self
                    .inner
                    .find_global_registered(mod_name, global_name.as_ref())?;
                Ok(Global {
                    inner,
                    name: Some(global_name.as_ref().into()),
                    mod_name: Some(mod_name.into()),
                    _marker: PhantomData,
                })
            }
            None => {
                let inner = self.inner.find_global(global_name.as_ref())?;
                Ok(Global {
                    inner,
                    name: Some(global_name.as_ref().into()),
                    mod_name: None,
                    _marker: PhantomData,
                })
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{ConfigBuilder, Module, SignatureBuilder, ValType, Value, VmBuilder};

    #[test]
    fn test_store_instance() {
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

        // check the Func instances
        {
            let result = store.functions();
            assert!(result.is_ok());
            let functions = result.unwrap();
            assert_eq!(functions.len(), 1);
            let result = store.functions_by_module("fib-module");
            assert!(result.is_ok());
            let functions = result.unwrap();
            assert_eq!(functions.len(), 1);
            let result = store.functions_by_name("fib");
            assert!(result.is_ok());
            let functions = result.unwrap();
            assert_eq!(functions.len(), 1);

            // get all Func instances in the store
            let result = store.function("fib", Some("fib-module"));
            assert!(result.is_ok());
            let func = result.unwrap();
            assert!(func.registered());
            assert_eq!(func.mod_name().unwrap(), "fib-module");
            assert_eq!(func.name().unwrap(), "fib");

            // check function signation
            let result = func.signature();
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
            // let result = vm.run_func(None, "fib", [Value::I32(5)]);
            // assert!(result.is_ok());
        }

        {
            // get all Table instances in the store
            let result = store.tables();
            assert!(result.is_ok());
            let tables = result.unwrap();
            dbg!(tables);
        }

        {
            // get all Memory instances in the store
            let result = store.memories();
            assert!(result.is_ok());
            let memories = result.unwrap();
            dbg!(memories);
        }

        {
            // get all Global instances in the store
            let result = store.globals();
            assert!(result.is_ok());
            let globals = result.unwrap();
            dbg!(globals);
        }
    }

    #[test]
    fn test_store_table() {}
}
