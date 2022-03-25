use crate::{error::Result, sys, Executor, ImportModule, Instance, Module};

#[derive(Debug)]
pub struct Store {
    pub(crate) inner: sys::Store,
}
impl Store {
    pub fn new() -> Result<Self> {
        let inner = sys::Store::create()?;
        Ok(Self { inner })
    }

    pub fn register_import_module(
        &mut self,
        executor: &mut Executor,
        import: &ImportModule,
    ) -> Result<()> {
        executor
            .inner
            .register_import_object(&mut self.inner, &import.inner)?;

        Ok(())
    }

    pub fn register_named_module(
        &mut self,
        executor: &mut Executor,
        mod_name: impl AsRef<str>,
        module: &Module,
    ) -> Result<()> {
        executor
            .inner
            .register_named_module(&mut self.inner, &module.inner, mod_name.as_ref())?;

        Ok(())
    }

    pub fn register_active_module(
        &mut self,
        executor: &mut Executor,
        module: &Module,
    ) -> Result<()> {
        executor
            .inner
            .register_active_module(&mut self.inner, &module.inner)?;

        Ok(())
    }

    pub fn named_instance_count(&self) -> u32 {
        self.inner.reg_module_len()
    }

    /// Returns the names of all registered [modules](crate::Module)
    pub fn instance_names(&self) -> Option<Vec<String>> {
        self.inner.reg_module_names()
    }

    /// Returns the [instance](crate::Instance) with the specified name.
    pub fn named_instance(&mut self, name: impl AsRef<str>) -> Option<Instance> {
        let inner_instance = self.inner.named_module(name.as_ref()).ok();
        if let Some(inner_instance) = inner_instance {
            return Some(Instance {
                inner: inner_instance,
            });
        }

        None
    }

    /// Returns the active [instance](crate::Instance).
    pub fn active_instance(&mut self) -> Option<Instance> {
        let inner_instance = self.inner.active_module().ok();
        if let Some(inner_instance) = inner_instance {
            return Some(Instance {
                inner: inner_instance,
            });
        }

        None
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        sys::Mutability,
        types::{RefType, Val, ValType},
        Executor, GlobalType, ImportModuleBuilder, MemoryType, Module, SignatureBuilder,
        Statistics, TableType, WasmValue, WasmValueType,
    };

    #[test]
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
    fn test_store_register_import_module() {
        // create an ImportModule instance
        let result = ImportModuleBuilder::new()
            .with_func(
                "add",
                SignatureBuilder::new()
                    .with_args(vec![WasmValueType::I32; 2])
                    .with_returns(vec![WasmValueType::I32])
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
        assert!(store.instance_names().is_some());
        assert_eq!(store.instance_names().unwrap(), ["extern-module"]);

        // get active module instance
        let result = store.named_instance("extern-module");
        assert!(result.is_some());
        let instance = result.unwrap();
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), "extern-module");

        let result = instance.global("global");
        assert!(result.is_some());
        let global = result.unwrap();
        let result = global.ty();
        assert!(result.is_ok());
    }

    #[test]
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
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");

        let result = Module::from_file(Some(&config), file);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register a module into store as a named module
        let result = store.register_named_module(&mut executor, "extern-module", &module);
        assert!(result.is_ok());

        assert_eq!(store.named_instance_count(), 1);
        assert!(store.instance_names().is_some());
        assert_eq!(store.instance_names().unwrap(), ["extern-module"]);

        // get active module instance
        let result = store.named_instance("extern-module");
        assert!(result.is_some());
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
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");

        let result = Module::from_file(Some(&config), file);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register a module into store as active module
        let result = store.register_active_module(&mut executor, &module);
        assert!(result.is_ok());

        assert_eq!(store.named_instance_count(), 0);
        assert!(store.instance_names().is_none());

        let result = store.active_instance();
        assert!(result.is_some());
        let active_instance = result.unwrap();
        assert!(active_instance.name().is_none());

        let result = active_instance.func("fib");
        assert!(result.is_some());
        let func = result.unwrap();
        assert_eq!(func.name().unwrap(), "fib");
        assert!(func.mod_name().is_none());
    }

    #[test]
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

        // create an ImportModule instance
        let result = ImportModuleBuilder::new()
            .with_func(
                "add",
                SignatureBuilder::new()
                    .with_args(vec![WasmValueType::I32; 2])
                    .with_returns(vec![WasmValueType::I32])
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

        // register a module into store as a named module
        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        // add a wasm module from a file
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("tools/wasmedge/examples/fibonacci.wasm");
        let result = Module::from_file(Some(&config), file);
        assert!(result.is_ok());
        let module = result.unwrap();

        let result = store.register_named_module(&mut executor, "fib-module", &module);
        assert!(result.is_ok());

        // check the exported instances
        assert_eq!(store.named_instance_count(), 2);
        assert!(store.instance_names().is_some());
        let mod_names = store.instance_names().unwrap();
        assert_eq!(mod_names[0], "extern-module");
        assert_eq!(mod_names[1], "fib-module");

        assert_eq!(mod_names[0], "extern-module");
        let result = store.named_instance(mod_names[0].as_str());
        assert!(result.is_some());
        let instance = result.unwrap();
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), mod_names[0]);

        assert_eq!(mod_names[1], "fib-module");
        let result = store.named_instance(mod_names[1].as_str());
        assert!(result.is_some());
        let instance = result.unwrap();
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), mod_names[1]);
    }

    fn real_add(inputs: Vec<WasmValue>) -> std::result::Result<Vec<WasmValue>, u8> {
        if inputs.len() != 2 {
            return Err(1);
        }

        let a = if inputs[0].ty() == WasmValueType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(2);
        };

        let b = if inputs[1].ty() == WasmValueType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(3);
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }
}
