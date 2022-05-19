use crate::{
    error::Result, Config, ImportObject, Module, Value, WasiImportMod, WasmEdgeProcessImportMod,
};
use std::{collections::HashMap, path::Path};
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
    pub fn new(config: Option<Config>) -> Result<Self> {
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

    // validate + instantiate + register
    pub fn register_import_module(mut self, import: ImportObject) -> Result<Self> {
        self.inner.register_wasm_from_import(import.0)?;

        Ok(self)
    }

    // register a named or active module that is already validated.
    pub fn register_module(mut self, module: Module, mod_name: Option<&str>) -> Result<Self> {
        match mod_name {
            Some(name) => self.register_named_module(module, name),
            None => self.register_active_module(module),
        }
    }

    fn register_named_module(mut self, module: Module, mod_name: impl AsRef<str>) -> Result<Self> {
        self.inner
            .register_wasm_from_module(mod_name.as_ref(), module.inner)?;

        Ok(self)
    }

    fn register_active_module(mut self, module: Module) -> Result<Self> {
        self.active_module = Some(module);
        self.inner
            .load_wasm_from_module(&self.active_module.as_ref().unwrap().inner)?;
        self.inner.validate()?;
        self.inner.instantiate()?;

        Ok(self)
    }

    pub fn run_func(
        &mut self,
        mod_name: Option<&str>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> Result<Vec<Value>> {
        let returns = match mod_name {
            Some(mod_name) => {
                // run a function in the registered module
                self.inner_executor.run_func_registered(
                    &mut self.inner_store,
                    mod_name,
                    func_name.as_ref(),
                    args,
                )?
            }
            None => {
                // run a function in the active module
                self.inner_executor
                    .run_func(&mut self.inner_store, func_name.as_ref(), args)?
            }
        };

        Ok(returns)
    }

    // pub fn reset(&mut self) {
    //     self.inner.reset()
    // }

    pub fn run_hostfunc_from_file(
        &mut self,
        file: impl AsRef<Path>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> Result<Vec<Value>> {
        // load module from file
        let module = self.inner_loader.from_file(file)?;

        // validate module
        self.inner_validator.validate(&module)?;

        // instantiate the module
        self.inner_executor
            .register_active_module(&mut self.inner_store, &module)?;

        // run function
        let returns = self
            .inner_executor
            .run_func(&mut self.inner_store, func_name, args)?;

        Ok(returns)
    }

    pub fn run_hostfunc_from_buffer(
        mut self,
        buffer: &[u8],
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> Result<Vec<Value>> {
        // load module from buffer
        let module = self.inner_loader.from_buffer(buffer)?;

        // validate module
        self.inner_validator.validate(&module)?;

        // instantiate the module
        self.inner_executor
            .register_active_module(&mut self.inner_store, &module)?;

        // run function
        let returns = self
            .inner_executor
            .run_func(&mut self.inner_store, func_name, args)?;

        Ok(returns)
    }

    // run a function in the given module. The module must be validated.
    pub fn run_hostfunc_from_module(
        mut self,
        module: Module,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> Result<Vec<Value>> {
        // instantiate the module
        self.inner_executor
            .register_active_module(&mut self.inner_store, &module.inner)?;

        // run function
        let returns = self
            .inner_executor
            .run_func(&mut self.inner_store, func_name, args)?;

        Ok(returns)
    }

    /// Returns the number of the named [module instances](crate::Instance) in this [store](crate::Store).
    pub fn named_instance_count(&self) -> u32 {
        self.inner.module_len()
    }

    /// Returns the names of all registered named [module instances](crate::Instance).
    pub fn instance_names(&self) -> Option<Vec<String>> {
        self.inner.module_names()
    }

    /// Returns the named [module instance](crate::Instance) with the given name.
    ///
    /// # Argument
    ///
    /// * `name` - The name of the target [module instance](crate::Instance) to be returned.
    pub fn module_instance(&mut self, name: impl AsRef<str>) -> Option<Instance> {
        let inner_instance = self.inner.module(name.as_ref()).ok();
        if let Some(inner_instance) = inner_instance {
            return Some(Instance {
                inner: inner_instance,
            });
        }

        None
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
    use wasmedge::ValType;

    #[test]
    fn test_vm_create() {
        {
            let result = Vm::new(None);
            assert!(result.is_ok());
        }

        {
            // create a Config
            let result = Config::new();
            assert!(result.is_ok());
            let config = result.unwrap();

            // create a Vm context
            let result = Vm::new(Some(config));
            assert!(result.is_ok());
        }
    }

    #[test]
    fn test_vm_statistics() {
        // create a Config
        let result = Config::new();
        assert!(result.is_ok());

        // enable the configuration options for statistics
        let mut config = result.unwrap();
        config.aot_cost_measuring(true);
        config.aot_time_measuring(true);
        config.aot_instr_counting(true);

        // create a Vm context
        let result = Vm::new(Some(config));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // get the statistics
        // let _stat = vm.statistics_mut();
    }

    #[test]
    fn test_vm_add_named_module() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = Module::from_file(&vm, file);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register the wasm module into vm
        let result = vm.register_module(module, Some("extern"));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // check the exported functions in the "extern" module
        let store = vm.store_mut();
        let result = store.named_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        assert_eq!(instance.func_count(), 1);
        let result = instance.func_names();
        assert!(result.is_some());
        let func_names = result.unwrap();
        assert_eq!(func_names, ["fib"]);

        // get host_func
        let result = instance.func("fib");
        assert!(result.is_some());
        let host_func = result.unwrap();

        // check the type of host_func
        let result = host_func.signature();
        assert!(result.is_ok());
        let signature = result.unwrap();
        assert!(signature.args().is_some());
        assert_eq!(signature.args().unwrap(), [ValType::I32]);
        assert!(signature.returns().is_some());
        assert_eq!(signature.returns().unwrap(), [ValType::I32]);
    }
}
