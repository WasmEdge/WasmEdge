use crate::{
    error::Result, wasmedge, Config, ImportMod, Module, Statistics, Store, Value, WasiImportMod,
    WasmEdgeProcessImportMod,
};
use std::{marker::PhantomData, path::Path};

#[derive(Debug)]
pub struct Vm {
    // pub(crate) inner: wasmedge::Vm,
    inner_config: wasmedge::Config,
    inner_store: wasmedge::Store,
    inner_loader: wasmedge::Loader,
    inner_validator: wasmedge::Validator,
    inner_executor: wasmedge::Executor,
    inner_statistics: wasmedge::Statistics,
}
impl Vm {
    pub fn new(config: Option<Config>) -> Result<Self> {
        let inner_config = match config {
            Some(config) => config.inner,
            None => wasmedge::Config::create()?,
        };

        // create an inner store
        let inner_store = wasmedge::Store::create()?;

        // create an inner loader
        let inner_config_copied = wasmedge::Config::copy_from(&inner_config)?;
        let inner_loader = wasmedge::Loader::create(Some(inner_config_copied))?;

        // create an inner validator
        let inner_config_copied = wasmedge::Config::copy_from(&inner_config)?;
        let inner_validator = wasmedge::Validator::create(Some(inner_config_copied))?;

        // create an inner statistics
        let inner_statistics = wasmedge::Statistics::create()?;

        // create an inner executor
        let inner_config_copied = wasmedge::Config::copy_from(&inner_config)?;
        let mut inner_executor =
            wasmedge::Executor::create(Some(inner_config_copied), Some(&inner_statistics))?;

        // init vm
        if inner_config.wasi_enabled() {
            let wasi_mod = ImportMod::new_wasi(None, None, None)?;
            inner_executor =
                inner_executor.register_import_object(&mut inner_store, wasi_mod.inner)?;
        }
        if inner_config.wasmedge_process_enabled() {
            let proc_mod = ImportMod::new_wasmedge_process(None, false)?;
            inner_executor =
                inner_executor.register_import_object(&mut inner_store, proc_mod.inner)?;
        }

        Ok(Self {
            inner_config,
            inner_store,
            inner_loader,
            inner_validator,
            inner_executor,
            inner_statistics,
        })
    }

    pub fn store_mut(&self) -> Store {
        Store {
            inner: self.inner_store,
            _marker: PhantomData,
        }
    }

    pub fn statistics_mut(&self) -> Statistics {
        Statistics {
            inner: self.inner_statistics,
            _marker: PhantomData,
        }
    }

    pub fn wasmedge_process_module(&mut self) -> Result<WasmEdgeProcessImportMod> {
        let inner = self.inner.wasmedge_process_import_module_mut()?;
        Ok(WasmEdgeProcessImportMod {
            inner,
            _marker: PhantomData,
        })
    }

    pub fn wasi_module(&mut self) -> Result<WasiImportMod> {
        let inner = self.inner.wasi_import_module_mut()?;
        Ok(WasiImportMod {
            inner,
            _marker: PhantomData,
        })
    }

    // validate + instantiate + register
    pub fn add_import(mut self, import: ImportMod) -> Result<Self> {
        self.inner.register_wasm_from_import(import.inner)?;
        Ok(self)
    }

    // validate + instantiate + register a named module or an active module
    pub fn add_module(mut self, module: Module, name: Option<&str>) -> Result<Self> {
        match name {
            Some(name) => {
                self.inner.register_wasm_from_module(name, module.inner)?;
            }
            None => {
                // load module into vm
                self.inner.load_wasm_from_module(module.inner)?;

                // validate
                self.inner.validate()?;

                // instantiate
                self.inner.instantiate()?;
            }
        }

        Ok(self)
    }

    pub fn run_func(
        &self,
        mod_name: Option<&str>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> Result<Vec<Value>> {
        match mod_name {
            Some(mod_name) => {
                // run a function in the registered module
                let returns =
                    self.inner
                        .run_registered_function(mod_name, func_name.as_ref(), args)?;
                Ok(returns)
            }
            None => {
                // run a function in the active module
                let returns = self.inner.run_function(func_name.as_ref(), args)?;
                Ok(returns)
            }
        }
    }

    pub fn reset(&mut self) {
        self.inner.reset()
    }

    pub fn run_wasm_from_file(
        &mut self,
        file: impl AsRef<Path>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> Result<Vec<Value>> {
        let returns = self
            .inner
            .run_wasm_from_file(file.as_ref(), func_name.as_ref(), args)?;
        Ok(returns)
    }

    pub fn run_wasm_from_buffer(
        &mut self,
        buffer: &[u8],
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> Result<Vec<Value>> {
        let returns = self
            .inner
            .run_wasm_from_buffer(buffer.as_ref(), func_name.as_ref(), args)?;
        Ok(returns)
    }

    pub fn run_wasm_from_module(
        &mut self,
        module: Module,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> Result<Vec<Value>> {
        let returns = self
            .inner
            .run_wasm_from_module(module.inner, func_name.as_ref(), args)?;
        Ok(returns)
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
            let vm = result.unwrap();

            assert!(vm.store_mut().is_ok());
            assert!(vm.statistics_mut().is_ok());
        }

        {
            // create a Config
            let result = Config::new();
            assert!(result.is_ok());
            let config = result.unwrap();

            // create a Vm context
            // let result = VmBuilder::new().with_config(config).build();
            let result = Vm::new(Some(config));
            assert!(result.is_ok());
            let vm = result.unwrap();

            assert!(vm.store_mut().is_ok());
            assert!(vm.statistics_mut().is_ok());
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
        let vm = result.unwrap();

        // get the statistics
        let result = vm.statistics_mut();
        assert!(result.is_ok());
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
        let vm = result.unwrap();

        // show the names of the exported functions in the registered module named "extern"
        let result = vm.store_mut();
        assert!(result.is_ok());
        let store = result.unwrap();
        let result = store.functions_by_module("extern");
        assert!(result.is_ok());
        let funcs = result.unwrap();
        assert_eq!(funcs.len(), 1);
        assert_eq!(funcs[0].name().unwrap(), "fib");
        // check the type of the func
        let result = funcs[0].ty();
        assert!(result.is_ok());
        let signature = result.unwrap();
        assert!(signature.args().is_some());
        assert_eq!(signature.args().unwrap(), [ValType::I32]);
        assert!(signature.returns().is_some());
        assert_eq!(signature.returns().unwrap(), [ValType::I32]);
        // run "fib" func
        let result = funcs[0].call(&vm, [Value::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);
    }
}
