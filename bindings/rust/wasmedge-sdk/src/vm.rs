use crate::{config::Config, ImportObject, Instance, Module, WasmEdgeResult};
use wasmedge_sys as sys;

#[derive(Debug)]
pub struct Vm {
    pub(crate) inner: sys::Vm,
    // _inner_config: wasmedge::Config,
    // inner_store: wasmedge::Store,
    // pub(crate) inner_loader: wasmedge::Loader,
    // pub(crate) inner_validator: wasmedge::Validator,
    // inner_executor: wasmedge::Executor,
    // inner_statistics: wasmedge::Statistics,
    // imports: HashMap<wasmedge::types::HostRegistration, ImportMod>,
    active_module: Option<Module>,
}
impl Vm {
    /// Creates a new [Vm] to be associated with the given [configuration](crate::Config).
    ///
    /// # Arguments
    ///
    /// * `config` - A optional [configuration](crate::config::Config) for the new [Vm].
    ///
    /// # Error
    ///
    /// If fail to create, then an error is returned.
    pub fn new(config: Option<Config>) -> WasmEdgeResult<Self> {
        let inner_config = config.map(|c| c.inner);
        let inner = sys::Vm::create(inner_config, None)?;
        Ok(Self {
            inner,
            active_module: None,
        })
    }

    // pub fn store_mut(&mut self) -> Store {
    //     Store {
    //         inner: &mut self.inner_store,
    //     }
    // }

    // pub fn statistics_mut(&mut self) -> Statistics {
    //     Statistics {
    //         inner: self.inner_statistics,
    //     }
    // }

    // pub fn wasmedge_process_module(&mut self) -> Option<WasmEdgeProcessImportMod> {
    //     self.imports
    //         .get_mut(&wasmedge::types::HostRegistration::WasmEdgeProcess)
    //         .map(|import| WasmEdgeProcessImportMod {
    //             inner: &mut import.inner,
    //         })
    // }

    // pub fn wasi_module(&mut self) -> Option<WasiImportMod> {
    //     self.imports
    //         .get_mut(&wasmedge::types::HostRegistration::Wasi)
    //         .map(|import| WasiImportMod {
    //             inner: &mut import.inner,
    //         })
    // }

    /// Registers and instantiates a WasmEdge [import object](crate::ImportObject) into this [vm](crate::Vm).
    ///
    /// # Arguments
    ///
    /// * `import` - The WasmEdge [import object](crate::ImportObject) to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the given [import object](crate::ImportObject), then an error is returned.
    pub fn register_import_module(mut self, import: ImportObject) -> WasmEdgeResult<Self> {
        self.inner.register_wasm_from_import(import.0)?;

        Ok(self)
    }

    /// Registers and instantiates a WasmEdge [compiled module](crate::Module) into this [vm](crate::Vm) as a named or active [module instance](crate::Instance).
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The exported name of the registered [module](crate::Module). If `None`, then the [module](crate::Module) is registered as an active [module instance](crate::Instance).
    ///
    /// * `module` - The validated [module](crate::Module) to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the given [module](crate::Module), then an error is returned.
    pub fn register_module(self, mod_name: Option<&str>, module: Module) -> WasmEdgeResult<Self> {
        match mod_name {
            Some(name) => self.register_named_module(module, name),
            None => self.register_active_module(module),
        }
    }

    fn register_named_module(
        mut self,
        module: Module,
        mod_name: impl AsRef<str>,
    ) -> WasmEdgeResult<Self> {
        self.inner
            .register_wasm_from_module(mod_name.as_ref(), module.inner)?;

        Ok(self)
    }

    fn register_active_module(mut self, module: Module) -> WasmEdgeResult<Self> {
        self.active_module = Some(module);
        self.inner
            .load_wasm_from_module(&self.active_module.as_ref().unwrap().inner)?;
        self.inner.validate()?;
        self.inner.instantiate()?;

        Ok(self)
    }

    // pub fn run_func(
    //     &mut self,
    //     mod_name: Option<&str>,
    //     func_name: impl AsRef<str>,
    //     args: impl IntoIterator<Item = Value>,
    // ) -> Result<Vec<Value>> {
    //     let returns = match mod_name {
    //         Some(mod_name) => {
    //             // run a function in the registered module
    //             self.inner_executor.run_func_registered(
    //                 &mut self.inner_store,
    //                 mod_name,
    //                 func_name.as_ref(),
    //                 args,
    //             )?
    //         }
    //         None => {
    //             // run a function in the active module
    //             self.inner_executor
    //                 .run_func(&mut self.inner_store, func_name.as_ref(), args)?
    //         }
    //     };

    //     Ok(returns)
    // }

    // pub fn reset(&mut self) {
    //     self.inner.reset()
    // }

    // pub fn run_hostfunc_from_file(
    //     &mut self,
    //     file: impl AsRef<Path>,
    //     func_name: impl AsRef<str>,
    //     args: impl IntoIterator<Item = Value>,
    // ) -> Result<Vec<Value>> {
    //     // load module from file
    //     let module = self.inner_loader.from_file(file)?;

    //     // validate module
    //     self.inner_validator.validate(&module)?;

    //     // instantiate the module
    //     self.inner_executor
    //         .register_active_module(&mut self.inner_store, &module)?;

    //     // run function
    //     let returns = self
    //         .inner_executor
    //         .run_func(&mut self.inner_store, func_name, args)?;

    //     Ok(returns)
    // }

    // pub fn run_hostfunc_from_buffer(
    //     mut self,
    //     buffer: &[u8],
    //     func_name: impl AsRef<str>,
    //     args: impl IntoIterator<Item = Value>,
    // ) -> Result<Vec<Value>> {
    //     // load module from buffer
    //     let module = self.inner_loader.from_buffer(buffer)?;

    //     // validate module
    //     self.inner_validator.validate(&module)?;

    //     // instantiate the module
    //     self.inner_executor
    //         .register_active_module(&mut self.inner_store, &module)?;

    //     // run function
    //     let returns = self
    //         .inner_executor
    //         .run_func(&mut self.inner_store, func_name, args)?;

    //     Ok(returns)
    // }

    // // run a function in the given module. The module must be validated.
    // pub fn run_hostfunc_from_module(
    //     mut self,
    //     module: Module,
    //     func_name: impl AsRef<str>,
    //     args: impl IntoIterator<Item = Value>,
    // ) -> Result<Vec<Value>> {
    //     // instantiate the module
    //     self.inner_executor
    //         .register_active_module(&mut self.inner_store, &module.inner)?;

    //     // run function
    //     let returns = self
    //         .inner_executor
    //         .run_func(&mut self.inner_store, func_name, args)?;

    //     Ok(returns)
    // }

    /// Returns the count of the named [module instances](crate::Instance) in this [store](crate::Store).
    ///
    /// # Error
    ///
    /// If fail to get the count, then an error is returned.
    pub fn named_instance_count(&self) -> WasmEdgeResult<u32> {
        let count = self.inner.store_mut()?.module_len();
        Ok(count)
    }

    /// Returns the names of all registered named [module instances](crate::Instance).
    ///
    /// # Error
    /// If fail to get the instance names, then an error is returned.
    pub fn instance_names(&self) -> WasmEdgeResult<Vec<String>> {
        let names = self.inner.store_mut()?.module_names();
        match names {
            Some(names) => Ok(names),
            None => Ok(vec![]),
        }
    }

    /// Returns the named [module instance](crate::Instance) with the given name.
    ///
    /// # Argument
    ///
    /// * `name` - The name of the target [module instance](crate::Instance) to be returned.
    ///
    /// # Error
    ///
    /// If fail to get the named [module instance](crate::Instance), then an error is returned.
    pub fn named_module(&self, name: impl AsRef<str>) -> WasmEdgeResult<Instance> {
        let mut inner_store = self.inner.store_mut()?;
        let inner_instance = inner_store.module(name.as_ref())?;

        Ok(Instance {
            inner: inner_instance,
        })
    }

    /// Returns the active [module instance](crate::Instance).
    ///
    /// # Error
    ///
    /// If fail to get the active [module instance](crate::Instance), then an error is returned.
    pub fn active_module(&self) -> WasmEdgeResult<Instance> {
        let inner_instance = self.inner.active_module()?;

        Ok(Instance {
            inner: inner_instance,
        })
    }
}
impl Default for Vm {
    fn default() -> Self {
        Self::new(None).unwrap()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder, StatisticsConfigOptions},
        types::Val,
        Global, ImportObjectBuilder, Memory, Table,
    };
    use wasmedge_sys::WasmValue;
    use wasmedge_types::{
        wat2wasm, GlobalType, MemoryType, Mutability, RefType, TableType, ValType,
    };

    #[test]
    fn test_vm_create() {
        {
            let result = Vm::new(None);
            assert!(result.is_ok());
        }

        {
            // create a Config
            let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
            assert!(result.is_ok());
            let config = result.unwrap();

            // create a Vm context
            let result = Vm::new(Some(config));
            assert!(result.is_ok());
        }
    }

    #[test]
    fn test_vm_statistics() {
        // set config options related to Statistics
        let stat_config_options = StatisticsConfigOptions::new()
            .measure_cost(true)
            .measure_time(true)
            .count_instructions(true);
        // create a Config
        let result = ConfigBuilder::new(CommonConfigOptions::default())
            .with_statistics_config(stat_config_options)
            .build();
        assert!(result.is_ok());
        let config = result.unwrap();

        // create a Vm context
        let result = Vm::new(Some(config));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // get the statistics
        // let _stat = vm.statistics_mut();
    }

    #[test]
    fn test_vm_register_import_module() {
        // create a Const global instance
        let result = Global::new(
            GlobalType::new(ValType::F32, Mutability::Const),
            Val::F32(3.5),
        );
        assert!(result.is_ok());
        let global_const = result.unwrap();

        // create a memory instance
        let result = Memory::new(MemoryType::new(10, None));
        assert!(result.is_ok());
        let memory = result.unwrap();

        // create a table instance
        let result = Table::new(TableType::new(RefType::FuncRef, 5, None));
        assert!(result.is_ok());
        let table = result.unwrap();

        // create an ImportModule instance
        let result = ImportObjectBuilder::new()
            .with_func::<(i32, i32), i32>("add", Box::new(real_add))
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

        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register an import module into vm
        let result = vm.register_import_module(import);
        assert!(result.is_ok());
        let vm = result.unwrap();

        assert_eq!(vm.named_instance_count().unwrap(), 1);
        assert!(vm.instance_names().is_ok());
        assert_eq!(vm.instance_names().unwrap(), ["extern-module"]);

        // get active module instance
        let result = vm.named_module("extern-module");
        assert!(result.is_ok());
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
    fn test_vm_register_named_module() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module
        let result = wat2wasm(
            br#"(module
            (export "fib" (func $fib))
            (func $fib (param $n i32) (result i32)
             (if
              (i32.lt_s
               (get_local $n)
               (i32.const 2)
              )
              (return
               (i32.const 1)
              )
             )
             (return
              (i32.add
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 2)
                )
               )
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 1)
                )
               )
              )
             )
            )
           )
        "#,
        );
        assert!(result.is_ok());
        let wasm_bytes = result.unwrap();
        let result = Module::from_bytes(None, wasm_bytes);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register the wasm module into vm
        assert_eq!(vm.named_instance_count().unwrap(), 0);
        let result = vm.register_module(Some("extern"), module);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // check the exported functions in the "extern" module
        assert_eq!(vm.named_instance_count().unwrap(), 1);
        let result = vm.named_module("extern");
        assert!(result.is_ok());
        let instance = result.unwrap();

        assert_eq!(instance.func_count(), 1);
        let result = instance.func_names();
        assert!(result.is_some());
        let func_names = result.unwrap();
        assert_eq!(func_names, ["fib"]);

        // get host_func
        let result = instance.func("fib");
        assert!(result.is_some());
        let fib = result.unwrap();

        // check the type of host_func
        let result = fib.ty();
        assert!(result.is_ok());
        let signature = result.unwrap();
        assert!(signature.args().is_some());
        assert_eq!(signature.args().unwrap(), [ValType::I32]);
        assert!(signature.returns().is_some());
        assert_eq!(signature.returns().unwrap(), [ValType::I32]);
    }

    #[test]
    fn test_vm_register_active_module() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module
        let result = wat2wasm(
            br#"(module
            (export "fib" (func $fib))
            (func $fib (param $n i32) (result i32)
             (if
              (i32.lt_s
               (get_local $n)
               (i32.const 2)
              )
              (return
               (i32.const 1)
              )
             )
             (return
              (i32.add
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 2)
                )
               )
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 1)
                )
               )
              )
             )
            )
           )
        "#,
        );
        assert!(result.is_ok());
        let wasm_bytes = result.unwrap();
        let result = Module::from_bytes(None, wasm_bytes);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register the wasm module into vm
        let result = vm.register_module(None, module);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // check the exported functions in the "extern" module
        let result = vm.active_module();
        assert!(result.is_ok());
        let instance = result.unwrap();

        assert_eq!(instance.func_count(), 1);
        let result = instance.func_names();
        assert!(result.is_some());
        let func_names = result.unwrap();
        assert_eq!(func_names, ["fib"]);

        // get host_func
        let result = instance.func("fib");
        assert!(result.is_some());
        let fib = result.unwrap();

        // check the type of host_func
        let result = fib.ty();
        assert!(result.is_ok());
        let signature = result.unwrap();
        assert!(signature.args().is_some());
        assert_eq!(signature.args().unwrap(), [ValType::I32]);
        assert!(signature.returns().is_some());
        assert_eq!(signature.returns().unwrap(), [ValType::I32]);
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
