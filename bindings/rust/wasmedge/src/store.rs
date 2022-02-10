use crate::{error::Result, wasmedge, Func, Global, Memory, Table, Vm};
use std::marker::PhantomData;

#[derive(Debug)]
pub struct Store<'vm> {
    pub(crate) inner: wasmedge::Store,
    pub(crate) _marker: PhantomData<&'vm Vm>,
}
impl<'vm> Store<'vm> {
    pub fn mod_count(&self) -> u32 {
        self.inner.reg_module_len()
    }

    /// Returns the names of all registered [modules](crate::Module)
    pub fn mod_names(&self) -> Option<Vec<String>> {
        self.inner.reg_module_names()
    }

    pub fn func_count(&self) -> u32 {
        self.inner.func_len()
    }

    pub fn func_count_by_module(&self, mod_name: impl AsRef<str>) -> u32 {
        self.inner.reg_func_len(mod_name)
    }

    /// Returns all exported functions in the store, including both registered and non-registered.
    pub fn functions(&self) -> Result<Vec<Func>> {
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

    pub fn functions_by_module(&self, mod_name: impl AsRef<str>) -> Result<Vec<Func>> {
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

    pub fn functions_by_name(&self, func_name: impl AsRef<str>) -> Result<Vec<Func>> {
        let funcs = self
            .functions()?
            .into_iter()
            .filter(|x| x.name().is_some() && (x.name().unwrap() == func_name.as_ref()));

        Ok(funcs.collect())
    }

    pub fn function(&self, func_name: impl AsRef<str>, mod_name: Option<&str>) -> Result<Func> {
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

    pub fn table_count(&self) -> u32 {
        self.inner.table_len()
    }

    pub fn table_count_by_module(&self, mod_name: impl AsRef<str>) -> u32 {
        self.inner.reg_table_len(mod_name)
    }

    pub fn tables(&self) -> Result<Vec<Table>> {
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
                            _marker: PhantomData,
                        };
                        tables.push(table);
                    }
                }
            }
        }

        Ok(tables)
    }

    pub fn tables_by_module(&self, _mod_name: impl AsRef<str>) -> Result<Vec<Table>> {
        unimplemented!()
    }

    pub fn tables_by_name(&self, _table_name: impl AsRef<str>) -> Result<Vec<Table>> {
        unimplemented!()
    }

    pub fn table(&self, table_name: impl AsRef<str>, mod_name: Option<&str>) -> Result<Table> {
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

    pub fn memory_count(&self) -> u32 {
        self.inner.mem_len()
    }

    pub fn memory_count_by_module(&self, mod_name: impl AsRef<str>) -> u32 {
        self.inner.reg_mem_len(mod_name)
    }

    pub fn memories(&self) -> Result<Vec<Memory>> {
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
                            _marker: PhantomData,
                        };
                        memories.push(memory);
                    }
                }
            }
        }

        Ok(memories)
    }

    pub fn memories_by_module(&self, _mod_name: impl AsRef<str>) -> Result<Vec<Memory>> {
        unimplemented!()
    }

    pub fn memories_by_name(&self, _mem_name: impl AsRef<str>) -> Result<Vec<Memory>> {
        unimplemented!()
    }

    pub fn memory(&self, mem_name: impl AsRef<str>, mod_name: Option<&str>) -> Result<Memory> {
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

    pub fn global_count(&self) -> u32 {
        self.inner.global_len()
    }

    pub fn global_count_by_module(&self, mod_name: impl AsRef<str>) -> u32 {
        self.inner.reg_global_len(mod_name)
    }

    pub fn globals(&self) -> Result<Vec<Global>> {
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
                            _marker: PhantomData,
                        };
                        globals.push(global);
                    }
                }
            }
        }

        Ok(globals)
    }

    pub fn globals_by_module(&self, _mod_name: impl AsRef<str>) -> Result<Vec<Global>> {
        unimplemented!()
    }

    pub fn globals_by_name(&self, _global_name: impl AsRef<str>) -> Result<Vec<Global>> {
        unimplemented!()
    }

    pub fn global(&self, global_name: impl AsRef<str>, mod_name: Option<&str>) -> Result<Global> {
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
    use crate::{
        wasmedge::{Mutability, RefType},
        Config, Func, GlobalType, ImportMod, MemoryType, Module, SignatureBuilder, TableType,
        ValType, Value, Vm,
    };

    #[test]
    fn test_store_instance() {
        // create a Config
        let result = Config::new();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Vm::new(Some(config));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register a wasm module from a file
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("tools/wasmedge/examples/fibonacci.wasm");
        let result = Module::from_file(&vm, file);
        assert!(result.is_ok());
        let module = result.unwrap();
        let result = vm.add_named_module("fib-module", module);
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
            let result = func.ty();
            assert!(result.is_ok());
            let signature = result.unwrap();
            assert_eq!(
                signature,
                SignatureBuilder::new()
                    .with_args([ValType::I32])
                    .with_returns([ValType::I32])
                    .build()
            );

            // run the "fib" function
            let result = func.call(&vm, [Value::from_i32(5)]);
            assert!(result.is_ok());
            let returns = result.unwrap();
            assert_eq!(returns[0].to_i32(), 8);
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

    #[test]
    fn test_store_basic() {
        let module_name = "extern_module";

        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        {
            // get store
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();

            // check the exported instances
            assert_eq!(store.func_count(), 0);
            assert_eq!(store.func_count_by_module(module_name), 0);
            assert_eq!(store.table_count(), 0);
            assert_eq!(store.table_count_by_module(module_name), 0);
            assert_eq!(store.global_count(), 0);
            assert_eq!(store.global_count_by_module(module_name), 0);
            assert_eq!(store.memory_count(), 0);
            assert_eq!(store.memory_count_by_module(module_name), 0);
            assert_eq!(store.mod_count(), 0);
            assert!(store.mod_names().is_none());
        }

        // create an ImportMod instance
        let result = ImportMod::new(module_name);
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add host function
        let sig = SignatureBuilder::new()
            .with_args(vec![ValType::I32; 2])
            .with_returns(vec![ValType::I32])
            .build();
        let result = Func::new(sig, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();
        import.add_func("add", host_func);

        // add table
        let ty = TableType::new(RefType::FuncRef, 5, None);
        let result = import.add_table("table", ty);
        assert!(result.is_ok());

        // add memory
        let ty = MemoryType::new(10, None);
        let result = import.add_memory("mem", ty);
        assert!(result.is_ok());

        // add globals
        let ty = GlobalType::new(ValType::F32, Mutability::Const);
        let result = import.add_global("global", ty, Value::from_f32(3.5));
        assert!(result.is_ok());

        // add the import module into vm
        let result = vm.add_named_import(import);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // get store
        let result = vm.store_mut();
        assert!(result.is_ok());
        let store = result.unwrap();

        // check the exported instances
        assert_eq!(store.mod_count(), 1);
        assert!(store.mod_names().is_some());
        assert_eq!(store.mod_names().unwrap()[0], module_name);
        assert_eq!(store.func_count(), 0);
        assert_eq!(store.func_count_by_module(module_name), 1);
        // assert!(store.functions_by_module(module_name).is_some());
        // assert_eq!(store.functions_by_module(module_name).unwrap()[0], "add");
        assert_eq!(store.table_count(), 0);
        assert_eq!(store.table_count_by_module(module_name), 1);
        // assert!(store.reg_table_names(module_name).is_some());
        // assert_eq!(store.reg_table_names(module_name).unwrap()[0], "table");
        assert_eq!(store.global_count(), 0);
        assert_eq!(store.global_count_by_module(module_name), 1);
        // assert!(store.reg_global_names(module_name).is_some());
        // assert_eq!(store.reg_global_names(module_name).unwrap()[0], "global");
        assert_eq!(store.memory_count(), 0);
        assert_eq!(store.memory_count_by_module(module_name), 1);
        // assert!(store.reg_mem_names(module_name).is_some());
        // assert_eq!(store.reg_mem_names(module_name).unwrap()[0], "mem");

        // check the function list after instantiation
        let result = store.function("add", None);
        assert!(result.is_err());
        let result = store.function("add", Some("extern_module"));
        assert!(result.is_ok());
        let host_func = result.unwrap();
        let result = host_func.call(&vm, vec![Value::from_i32(12), Value::from_i32(21)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 33);

        // check the table list after instantiation
        let result = store.table("table", None);
        assert!(result.is_err());
        let result = store.table("table", Some("extern_module"));
        assert!(result.is_ok());
        let table = result.unwrap();
        assert_eq!(table.name().unwrap(), "table");
        assert!(table.registered());
        assert_eq!(table.mod_name().unwrap(), "extern_module");
        assert_eq!(table.size(), 5);

        // check the memory list after instantiation
        let result = store.memory("mem", None);
        assert!(result.is_err());
        let result = store.memory("mem", Some("extern_module"));
        assert!(result.is_ok());
        let memory = result.unwrap();
        assert_eq!(memory.name().unwrap(), "mem");
        assert!(memory.registered());
        assert_eq!(memory.mod_name().unwrap(), "extern_module");
        assert_eq!(memory.size(), 10);

        // check the global list after instantiation
        let result = store.global("global", None);
        assert!(result.is_err());
        let result = store.global("global", Some("extern_module"));
        assert!(result.is_ok());
        let global = result.unwrap();
        let val = global.get_value();
        assert_eq!(val.to_f32(), 3.5);
    }

    fn real_add(inputs: Vec<Value>) -> std::result::Result<Vec<Value>, u8> {
        if inputs.len() != 2 {
            return Err(1);
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(2);
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(3);
        };

        let c = a + b;

        Ok(vec![Value::from_i32(c)])
    }
}
