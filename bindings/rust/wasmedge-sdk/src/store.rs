//! Defines WasmEdge Store struct.

use crate::{Executor, ImportObject, Instance, Module, WasmEdgeResult};
use wasmedge_sys as sys;

/// Represents all global state that can be manipulated by WebAssembly programs. A [store](crate::Store) consists of the runtime representation of all instances of [functions](crate::Func), [tables](crate::Table), [memories](crate::Memory), and [globals](crate::Global).
#[derive(Debug)]
pub struct Store {
    pub(crate) inner: sys::Store,
}
impl Store {
    /// Creates a new [Store].
    ///
    /// # Error
    ///
    /// If fail to create a new [Store], then an error is returned.
    pub fn new() -> WasmEdgeResult<Self> {
        let inner = sys::Store::create()?;
        Ok(Self { inner })
    }

    /// Registers and instantiates a WasmEdge [import object](crate::ImportObject) into this [store](crate::Store).
    ///
    /// # Arguments
    ///
    /// * `executor` - The [executor](crate::Executor) that runs the host functions in this [store](crate::Store).
    ///
    /// * `import` - The WasmEdge [import object](crate::ImportObject) to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the given [import object](crate::ImportObject), then an error is returned.
    pub fn register_import_module(
        &mut self,
        executor: &mut Executor,
        import: &ImportObject,
    ) -> WasmEdgeResult<()> {
        executor
            .inner
            .register_import_object(&mut self.inner, import.inner_ref())
    }

    /// Registers and instantiates a WasmEdge [compiled module](crate::Module) into this [store](crate::Store) as a named [module instance](crate::Instance), and returns the module instance.
    ///
    /// Instantiates the given WasmEdge [compiled module](crate::Module), including the [functions](crate::Func), [memories](crate::Memory), [tables](crate::Table), and [globals](crate::Global) it hosts; and then, registers the [module instance](crate::Instance) into the [store](crate::Store) with the given name.
    ///
    /// # Arguments
    ///
    /// * `executor` - The [executor](crate::Executor) that runs the host functions in this [store](crate::Store).
    ///
    /// * `mod_name` - The exported name of the registered [module](crate::Module).
    ///
    /// * `module` - The validated [module](crate::Module) to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the given [module](crate::Module), then an error is returned.
    pub fn register_named_module(
        &mut self,
        executor: &mut Executor,
        mod_name: impl AsRef<str>,
        module: &Module,
    ) -> WasmEdgeResult<Instance> {
        let inner_instance = executor.inner.register_named_module(
            &mut self.inner,
            &module.inner,
            mod_name.as_ref(),
        )?;
        Ok(Instance {
            inner: inner_instance,
        })
    }

    /// Registers and instantiates a WasmEdge [compiled module](crate::Module) into this [store](crate::Store) as an anonymous active [module instance](crate::Instance), and returns the module instance.
    ///
    /// # Arguments
    ///
    /// * `executor` - The [executor](crate::Executor) that runs the host functions in this [store](crate::Store).
    ///
    /// * `module` - The validated [module](crate::Module) to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the given [module](crate::Module), then an error is returned.
    pub fn register_active_module(
        &mut self,
        executor: &mut Executor,
        module: &Module,
    ) -> WasmEdgeResult<Instance> {
        let inner = executor
            .inner
            .register_active_module(&mut self.inner, &module.inner)?;

        Ok(Instance { inner })
    }

    /// Returns the number of the named [module instances](crate::Instance) in this [store](crate::Store).
    pub fn named_instance_count(&self) -> u32 {
        self.inner.module_len()
    }

    /// Returns the names of all registered named [module instances](crate::Instance).
    pub fn instance_names(&self) -> Vec<String> {
        match self.inner.module_names() {
            Some(names) => names,
            None => vec![],
        }
    }

    /// Returns the named [module instance](crate::Instance) with the given name.
    ///
    /// # Argument
    ///
    /// * `name` - The name of the target [module instance](crate::Instance) to be returned.
    pub fn named_instance(&mut self, name: impl AsRef<str>) -> WasmEdgeResult<Instance> {
        let inner_instance = self.inner.module(name.as_ref())?;

        Ok(Instance {
            inner: inner_instance,
        })
    }

    /// Checks if the [store](crate::Store) contains a named module instance.
    ///
    /// # Argument
    ///
    /// * `mod_name` - The name of the named module.
    ///
    pub fn contains(&self, mod_name: impl AsRef<str>) -> bool {
        self.inner.contains(mod_name.as_ref())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        error::HostFuncError,
        types::Val,
        CallingFrame, Executor, Global, GlobalType, ImportObjectBuilder, Memory, MemoryType,
        Module, Mutability, RefType, Statistics, Table, TableType, ValType, WasmValue,
    };

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_store_create() {
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::new();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::new(Some(&config), Some(&mut stat));
        assert!(result.is_ok());

        let result = Store::new();
        assert!(result.is_ok());
        let store = result.unwrap();

        assert_eq!(store.named_instance_count(), 0);
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_store_register_import_module() {
        // create a Const global instance
        let result = Global::new(
            GlobalType::new(ValType::F32, Mutability::Const),
            Val::F32(3.5),
        );
        assert!(result.is_ok());
        let global_const = result.unwrap();

        // create a memory instance
        let result = MemoryType::new(10, None, false);
        assert!(result.is_ok());
        let memory_type = result.unwrap();
        let result = Memory::new(memory_type);
        assert!(result.is_ok());
        let memory = result.unwrap();

        // create a table instance
        let result = Table::new(TableType::new(RefType::FuncRef, 5, None));
        assert!(result.is_ok());
        let table = result.unwrap();

        // create an ImportModule instance
        let result = ImportObjectBuilder::new()
            .with_func::<(i32, i32), i32>("add", real_add)
            .expect("failed to add host function")
            .with_global("global", global_const)
            .expect("failed to add const global")
            .with_memory("mem", memory)
            .expect("failed to add memory")
            .with_table("table", table)
            .expect("failed to add table")
            .build("extern-module");
        assert!(result.is_ok());
        let import = result.unwrap();

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

        // register an import module into store
        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        assert_eq!(store.named_instance_count(), 1);
        assert_eq!(store.instance_names(), ["extern-module"]);

        // get active module instance
        let result = store.named_instance("extern-module");
        assert!(result.is_ok());
        let instance = result.unwrap();
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), "extern-module");

        let result = instance.global("global");
        assert!(result.is_ok());
        let global = result.unwrap();
        let ty = global.ty();
        assert_eq!(*ty, GlobalType::new(ValType::F32, Mutability::Const));
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_store_register_named_module() {
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

        // load wasm module
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sdk/examples/data/fibonacci.wat");

        let result = Module::from_file(Some(&config), file);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register a module into store as a named module
        let result = store.register_named_module(&mut executor, "extern-module", &module);
        assert!(result.is_ok());

        assert_eq!(store.named_instance_count(), 1);
        assert_eq!(store.instance_names(), ["extern-module"]);

        // get active module instance
        let result = store.named_instance("extern-module");
        assert!(result.is_ok());
        let instance = result.unwrap();
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), "extern-module");
    }

    #[test]
    fn test_store_register_active_module() {
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

        // load wasm module
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sdk/examples/data/fibonacci.wat");

        let result = Module::from_file(Some(&config), file);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register a module into store as active module
        let result = store.register_active_module(&mut executor, &module);
        assert!(result.is_ok());
        let active_instance = result.unwrap();
        assert!(active_instance.name().is_none());
        let result = active_instance.func("fib");
        assert!(result.is_ok());

        assert_eq!(store.named_instance_count(), 0);
        assert_eq!(store.instance_names().len(), 0);
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_store_basic() {
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

        // create a Const global instance
        let result = Global::new(
            GlobalType::new(ValType::F32, Mutability::Const),
            Val::F32(3.5),
        );
        assert!(result.is_ok());
        let global_const = result.unwrap();

        // create a memory instance
        let result = MemoryType::new(10, None, false);
        assert!(result.is_ok());
        let memory_type = result.unwrap();
        let result = Memory::new(memory_type);
        assert!(result.is_ok());
        let memory = result.unwrap();

        // create a table instance
        let result = Table::new(TableType::new(RefType::FuncRef, 5, None));
        assert!(result.is_ok());
        let table = result.unwrap();

        // create an ImportModule instance
        let result = ImportObjectBuilder::new()
            .with_func::<(i32, i32), i32>("add", real_add)
            .expect("failed to add host function")
            .with_global("global", global_const)
            .expect("failed to add const global")
            .with_memory("mem", memory)
            .expect("failed to add memory")
            .with_table("table", table)
            .expect("failed to add table")
            .build("extern-module");
        assert!(result.is_ok());
        let import = result.unwrap();

        // register a module into store as a named module
        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        // add a wasm module from a file
        let file =
            std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("examples/wasm/fibonacci.wasm");
        let result = Module::from_file(Some(&config), file);
        assert!(result.is_ok());
        let module = result.unwrap();

        let result = store.register_named_module(&mut executor, "fib-module", &module);
        assert!(result.is_ok());

        // check the exported instances
        assert_eq!(store.named_instance_count(), 2);
        let mod_names = store.instance_names();
        assert_eq!(mod_names[0], "extern-module");
        assert_eq!(mod_names[1], "fib-module");

        assert_eq!(mod_names[0], "extern-module");
        let result = store.named_instance(&mod_names[0]);
        assert!(result.is_ok());
        let instance = result.unwrap();
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), mod_names[0]);

        assert_eq!(mod_names[1], "fib-module");
        let result = store.named_instance(&mod_names[1]);
        assert!(result.is_ok());
        let instance = result.unwrap();
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), mod_names[1]);
    }

    fn real_add(
        _frame: CallingFrame,
        inputs: Vec<WasmValue>,
    ) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
        if inputs.len() != 2 {
            return Err(HostFuncError::User(1));
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(HostFuncError::User(2));
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(HostFuncError::User(3));
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }
}
