//! Defines WasmEdge Instance.

use crate::{Func, Global, Memory, Table};
use std::marker::PhantomData;
use wasmedge_sys as sys;

/// Struct of WasmEdge Instance.
///
/// An [Instance] represents an instantiated module. In the instantiation process, A [module instance](crate::Instance) is created based on a [compiled module](crate::Module). From a [module instance] the exported [host function](crate::Func), [table](crate::Table), [memory](crate::Memory), and [global](crate::Global) instances can be fetched.
#[derive(Debug)]
pub struct Instance {
    pub(crate) inner: sys::Instance,
}
impl Instance {
    /// Returns the name of this exported [module instance](crate::Instance).
    ///
    /// If this [module instance](crate::Instance) is an active [instance](crate::Instance), return None.
    pub fn name(&self) -> Option<String> {
        self.inner.name()
    }

    /// Returns the count of the exported [function instances](crate::Func) in this [module instance](crate::Instance).
    pub fn func_count(&self) -> usize {
        self.inner.func_len() as usize
    }

    /// Returns the names of the exported [function instances](crate::Func) in this [module instance](crate::Instance).
    pub fn func_names(&self) -> Option<Vec<String>> {
        self.inner.func_names()
    }

    /// Returns the exported [function instance](crate::Func) in this [module instance](crate::Instance) by the given function name.
    ///
    /// # Argument
    ///
    /// * `name` - the name of the target exported [function instance](crate::Func).
    pub fn func(&self, name: impl AsRef<str>) -> Option<Func> {
        let inner_func = self.inner.get_func(name.as_ref()).ok();
        if let Some(inner_func) = inner_func {
            return Some(Func { inner: inner_func });
        }

        None
    }

    /// Returns the count of the exported [global instances](crate::Global) in this [module instance](crate::Instance).
    pub fn global_count(&self) -> usize {
        self.inner.global_len() as usize
    }

    /// Returns the names of the exported [global instances](crate::Global) in this [module instance](crate::Instance).
    pub fn global_names(&self) -> Option<Vec<String>> {
        self.inner.global_names()
    }

    /// Returns the exported [global instance](crate::Global) in this [module instance](crate::Instance) by the given global name.
    ///
    /// # Argument
    ///
    /// * `name` - the name of the target exported [global instance](crate::Global).
    pub fn global(&self, name: impl AsRef<str>) -> Option<Global> {
        let inner_global = self.inner.get_global(name.as_ref()).ok();
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

    /// Returns the count of the exported [memory instances](crate::Memory) in this [module instance](crate::Instance).
    pub fn memory_count(&self) -> usize {
        self.inner.mem_len() as usize
    }

    /// Returns the names of the exported [memory instances](crate::Memory) in this [module instance](crate::Instance).
    pub fn memory_names(&self) -> Option<Vec<String>> {
        self.inner.mem_names()
    }

    /// Returns the exported [memory instance](crate::Memory) in this [module instance](crate::Instance) by the given memory name.
    ///
    /// # Argument
    ///
    /// * `name` - the name of the target exported [memory instance](crate::Memory).
    pub fn memory(&self, name: impl AsRef<str>) -> Option<Memory> {
        let inner_memory = self.inner.get_memory(name.as_ref()).ok();
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

    /// Returns the count of the exported [table instances](crate::Table) in this [module instance](crate::Instance).
    pub fn table_count(&self) -> usize {
        self.inner.table_len() as usize
    }

    /// Returns the names of the exported [table instances](crate::Table) in this [module instance](crate::Instance).
    pub fn table_names(&self) -> Option<Vec<String>> {
        self.inner.table_names()
    }

    /// Returns the exported [table instance](crate::Table) in this [module instance](crate::Instance) by the given table name.
    ///
    /// # Argument
    ///
    /// * `name` - the name of the target exported [table instance](crate::Table).
    pub fn table(&self, name: impl AsRef<str>) -> Option<Table> {
        let inner_table = self.inner.get_table(name.as_ref()).ok();
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
        types::Val,
        Executor, FuncTypeBuilder, ImportModuleBuilder, Module, Statistics, Store, WasmValue,
    };
    use wasmedge_types::{GlobalType, MemoryType, Mutability, RefType, TableType, ValType};

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
                FuncTypeBuilder::new()
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

        let result = store.register_import_module(&mut executor, import);
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
            let result = store.module_instance(mod_names[0].as_str());
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
                host_func.ty().unwrap(),
                FuncTypeBuilder::new()
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
            assert_eq!(table.mod_name().unwrap(), "extern-module");
            assert_eq!(table.size(), 5);

            // check the exported memory
            let result = instance.memory("mem");
            assert!(result.is_some());
            let memory = result.unwrap();

            assert_eq!(memory.name().unwrap(), "mem");
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
            let result = store.module_instance(mod_names[1].as_str());
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
                host_func.ty().unwrap(),
                FuncTypeBuilder::new()
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
