use crate::{
    error::Result, wasmedge, Config, ImportMod, Module, Statistics, Store, Value, WasiImportMod,
    WasmEdgeProcessImportMod,
};
use std::{collections::HashMap, path::Path};

#[derive(Debug)]
pub struct Vm {
    _inner_config: wasmedge::Config,
    inner_store: wasmedge::Store,
    pub(crate) inner_loader: wasmedge::Loader,
    pub(crate) inner_validator: wasmedge::Validator,
    inner_executor: wasmedge::Executor,
    inner_statistics: wasmedge::Statistics,
    imports: HashMap<wasmedge::types::HostRegistration, ImportMod>,
}
impl Vm {
    pub fn new(config: Option<Config>) -> Result<Self> {
        let inner_config = match config {
            Some(config) => config.inner,
            None => wasmedge::Config::create()?,
        };

        // create an inner store
        let mut inner_store = wasmedge::Store::create()?;

        // create an inner loader
        let inner_config_copied = wasmedge::Config::copy_from(&inner_config)?;
        let inner_loader = wasmedge::Loader::create(Some(inner_config_copied))?;

        // create an inner validator
        let inner_config_copied = wasmedge::Config::copy_from(&inner_config)?;
        let inner_validator = wasmedge::Validator::create(Some(inner_config_copied))?;

        // create an inner statistics
        let mut inner_statistics = wasmedge::Statistics::create()?;

        // create an inner executor
        let inner_config_copied = wasmedge::Config::copy_from(&inner_config)?;
        let mut inner_executor =
            wasmedge::Executor::create(Some(inner_config_copied), Some(&mut inner_statistics))?;

        // init vm
        let mut imports = HashMap::new();
        if inner_config.wasi_enabled() {
            let wasi_mod = ImportMod::new_wasi(None, None, None)?;
            inner_executor.register_import_object(&mut inner_store, &wasi_mod.inner)?;
            imports.insert(wasmedge::types::HostRegistration::Wasi, wasi_mod);
        }
        if inner_config.wasmedge_process_enabled() {
            let proc_mod = ImportMod::new_wasmedge_process(None, false)?;
            inner_executor.register_import_object(&mut inner_store, &proc_mod.inner)?;
            imports.insert(wasmedge::types::HostRegistration::WasmEdgeProcess, proc_mod);
        }

        Ok(Self {
            _inner_config: inner_config,
            inner_store,
            inner_loader,
            inner_validator,
            inner_executor,
            inner_statistics,
            imports,
        })
    }

    pub fn store_mut(&mut self) -> Store {
        Store {
            inner: &mut self.inner_store,
        }
    }

    pub fn statistics_mut(&mut self) -> Statistics {
        Statistics {
            inner: &mut self.inner_statistics,
        }
    }

    pub fn wasmedge_process_module(&mut self) -> Option<WasmEdgeProcessImportMod> {
        self.imports
            .get_mut(&wasmedge::types::HostRegistration::WasmEdgeProcess)
            .map(|import| WasmEdgeProcessImportMod {
                inner: &mut import.inner,
            })
    }

    pub fn wasi_module(&mut self) -> Option<WasiImportMod> {
        self.imports
            .get_mut(&wasmedge::types::HostRegistration::Wasi)
            .map(|import| WasiImportMod {
                inner: &mut import.inner,
            })
    }

    // validate + instantiate + register
    pub fn add_import(mut self, import: &ImportMod) -> Result<Self> {
        self.inner_executor
            .register_import_object(&mut self.inner_store, &import.inner)?;

        Ok(self)
    }

    // register a named or active module that is already validated.
    pub fn add_module(mut self, module: Module, name: Option<&str>) -> Result<Self> {
        let mod_name = match name {
            Some(name) => name,
            None => "",
        };

        self.inner_executor
            .register_module(&mut self.inner_store, &module.inner, mod_name)?;

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
            .instantiate(&mut self.inner_store, &module)?;

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
            .instantiate(&mut self.inner_store, &module)?;

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
            .instantiate(&mut self.inner_store, &module.inner)?;

        // run function
        let returns = self
            .inner_executor
            .run_func(&mut self.inner_store, func_name, args)?;

        Ok(returns)
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
        let _stat = vm.statistics_mut();
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
        let result = vm.add_module(module, Some("extern"));
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
        let result = host_func.ty();
        assert!(result.is_ok());
        let signature = result.unwrap();
        assert!(signature.args().is_some());
        assert_eq!(signature.args().unwrap(), [ValType::I32]);
        assert!(signature.returns().is_some());
        assert_eq!(signature.returns().unwrap(), [ValType::I32]);
    }
}
