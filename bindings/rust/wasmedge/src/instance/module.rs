use crate::{sys, Func, Global, Memory, Table};
use std::marker::PhantomData;

#[derive(Debug)]
pub struct Instance<'store> {
    pub(crate) inner: sys::Instance<'store>,
}
impl<'store> Instance<'store> {
    pub fn name(&self) -> Option<String> {
        self.inner.name()
    }

    pub fn func_count(&self) -> usize {
        self.inner.func_len() as usize
    }

    pub fn func_names(&self) -> Option<Vec<String>> {
        self.inner.func_names()
    }

    pub fn func(&self, name: impl AsRef<str>) -> Option<Func> {
        let inner_func = self.inner.find_func(name.as_ref()).ok();
        if let Some(inner_func) = inner_func {
            return Some(Func {
                inner: inner_func,
                _marker: PhantomData,
            });
        }

        None
    }

    pub fn global_count(&self) -> usize {
        self.inner.global_len() as usize
    }

    pub fn global_names(&self) -> Option<Vec<String>> {
        self.inner.global_names()
    }

    pub fn global(&self, name: impl AsRef<str>) -> Option<Global> {
        let inner_global = self.inner.find_global(name.as_ref()).ok();
        if let Some(inner_global) = inner_global {
            return Some(Global {
                inner: inner_global,
                name: Some(name.as_ref().into()),
                mod_name: self.inner.name(),
                _marker: PhantomData,
            });
        }

        None
    }

    pub fn memory_count(&self) -> usize {
        self.inner.mem_len() as usize
    }

    pub fn memory_names(&self) -> Option<Vec<String>> {
        self.inner.mem_names()
    }

    pub fn memory(&self, name: impl AsRef<str>) -> Option<Memory> {
        let inner_memory = self.inner.find_memory(name.as_ref()).ok();
        if let Some(inner_memory) = inner_memory {
            return Some(Memory {
                inner: inner_memory,
                name: Some(name.as_ref().into()),
                mod_name: self.inner.name(),
                _marker: PhantomData,
            });
        }

        None
    }

    pub fn table_count(&self) -> usize {
        self.inner.table_len() as usize
    }

    pub fn table_names(&self) -> Option<Vec<String>> {
        self.inner.table_names()
    }

    pub fn table(&self, name: impl AsRef<str>) -> Option<Table> {
        let inner_table = self.inner.find_table(name.as_ref()).ok();
        if let Some(inner_table) = inner_table {
            return Some(Table {
                inner: inner_table,
                name: Some(name.as_ref().into()),
                mod_name: self.inner.name(),
                _marker: PhantomData,
            });
        }

        None
    }
}

#[cfg(test)]
mod tests {
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        sys::{Mutability, RefType},
        types::Val,
        Executor, GlobalType, ImportModuleBuilder, MemoryType, Module, SignatureBuilder,
        Statistics, Store, TableType, ValType, WasmValue,
    };

    #[test]
    fn test_instance_basic() {
        // create an executor
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::new();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::new(Some(&config), Some(&mut stat));
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        // create a store
        let result = Store::new();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        // check the exported instances
        assert_eq!(store.named_instance_count(), 0);
        assert!(store.instance_names().is_none());

        // create an ImportModule instance
        let result = ImportModuleBuilder::new()
            .with_func(
                "add",
                SignatureBuilder::new()
                    .with_args(vec![ValType::I32; 2])
                    .with_returns(vec![ValType::I32])
                    .build(),
                Box::new(real_add),
            )
            .expect("failed to add host function")
            .with_global(
                "global",
                GlobalType::new(ValType::F32, Mutability::Const),
                Val::F32(3.5),
            )
            .expect("failed to add const global")
            .with_memory("mem", MemoryType::new(10, None))
            .expect("failed to add memory")
            .with_table("table", TableType::new(RefType::FuncRef, 5, None))
            .expect("failed to add table")
            .build("extern-module");
        assert!(result.is_ok());
        let import = result.unwrap();

        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        // add a wasm module from a file
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("tools/wasmedge/examples/fibonacci.wasm");
        let result = Module::from_file(Some(&config), file);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register a module into store as a named module
        let result = store.register_named_module(&mut executor, "fib-module", &module);
        assert!(result.is_ok());

        // check the exported instances
        assert_eq!(store.named_instance_count(), 2);
        assert!(store.instance_names().is_some());
        let mod_names = store.instance_names().unwrap();
        assert_eq!(mod_names[0], "extern-module");
        assert_eq!(mod_names[1], "fib-module");

        // check the module instance named "extern-module"
        {
            assert_eq!(mod_names[0], "extern-module");
            let result = store.named_instance(mod_names[0].as_str());
            assert!(result.is_some());
            let instance = result.unwrap();
            assert!(instance.name().is_some());
            assert_eq!(instance.name().unwrap(), mod_names[0]);

            assert_eq!(instance.func_count(), 1);
            assert_eq!(instance.table_count(), 1);
            assert_eq!(instance.global_count(), 1);
            assert_eq!(instance.memory_count(), 1);

            // check the exported host function
            let result = instance.func("add");
            assert!(result.is_some());
            let host_func = result.unwrap();

            let func_name = host_func.name();
            assert!(func_name.is_some());
            assert_eq!(func_name.unwrap(), "add");
            assert_eq!(host_func.mod_name().unwrap(), "extern-module");
            assert_eq!(
                host_func.signature().unwrap(),
                SignatureBuilder::new()
                    .with_args(vec![ValType::I32; 2])
                    .with_returns(vec![ValType::I32])
                    .build()
            );

            // check the exported table
            let result = instance.table("table");
            assert!(result.is_some());
            let table = result.unwrap();

            let table_name = table.name();
            assert!(table_name.is_some());
            assert_eq!(table.name().unwrap(), "table");
            assert!(table.registered());
            assert_eq!(table.mod_name().unwrap(), "extern-module");
            assert_eq!(table.capacity(), 5);

            // check the exported memory
            let result = instance.memory("mem");
            assert!(result.is_some());
            let memory = result.unwrap();

            assert_eq!(memory.name().unwrap(), "mem");
            assert!(memory.registered());
            assert_eq!(memory.mod_name().unwrap(), "extern-module");
            assert_eq!(memory.size(), 10);

            // check the exported global
            let result = instance.global("global");
            assert!(result.is_some());
            let global = result.unwrap();
            let val = global.get_value();
            if let Val::F32(val) = val {
                assert_eq!(val, 3.5);
            } else {
                assert!(false);
            }
        }

        // check the module instance named "fib-module"
        {
            assert_eq!(mod_names[1], "fib-module");
            let result = store.named_instance(mod_names[1].as_str());
            assert!(result.is_some());
            let instance = result.unwrap();
            assert!(instance.name().is_some());
            assert_eq!(instance.name().unwrap(), mod_names[1]);

            assert_eq!(instance.func_count(), 1);
            assert_eq!(instance.table_count(), 0);
            assert_eq!(instance.global_count(), 0);
            assert_eq!(instance.memory_count(), 0);

            // check the exported host function
            let result = instance.func("fib");
            assert!(result.is_some());
            let host_func = result.unwrap();

            let func_name = host_func.name();
            assert!(func_name.is_some());
            assert_eq!(func_name.unwrap(), "fib");
            assert_eq!(host_func.mod_name().unwrap(), "fib-module");
            assert_eq!(
                host_func.signature().unwrap(),
                SignatureBuilder::new()
                    .with_args(vec![ValType::I32])
                    .with_returns(vec![ValType::I32])
                    .build()
            );
        }
    }

    fn real_add(inputs: Vec<WasmValue>) -> std::result::Result<Vec<WasmValue>, u8> {
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

        Ok(vec![WasmValue::from_i32(c)])
    }
}
