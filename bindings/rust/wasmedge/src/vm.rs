use crate::{
    error::Result, wasmedge, Config, ImportMod, Module, Statistics, Store, Value, WasiImportMod,
    WasmEdgeProcessImportMod,
};
use std::{marker::PhantomData, path::Path};

#[derive(Debug)]
pub struct Vm {
    pub(crate) inner: wasmedge::Vm,
    pub(crate) config: Option<Config>,
}
impl Vm {
    pub fn new(config: Option<Config>) -> Result<Self> {
        let mut config_copied = None;
        let config = match config {
            Some(config) => {
                config_copied = Some(Config::copy_from(&config)?);
                Some(config.inner)
            }
            None => None,
        };

        let inner = wasmedge::Vm::create(config, None)?;
        Ok(Self {
            inner,
            config: config_copied,
        })
    }

    pub fn store_mut(&self) -> Result<Store> {
        let inner = self.inner.store_mut()?;
        Ok(Store {
            inner,
            _marker: PhantomData,
        })
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

    pub fn statistics_mut(&self) -> Result<Statistics> {
        let inner = self.inner.statistics_mut()?;
        Ok(Statistics {
            inner,
            _marker: PhantomData,
        })
    }

    // validate + instantiate + register
    pub fn register_wasm_from_module(
        mut self,
        mod_name: impl AsRef<str>,
        module: Module,
    ) -> Result<Self> {
        self.inner
            .register_wasm_from_module(mod_name.as_ref(), module.inner)?;
        Ok(self)
    }

    // validate + instantiate + register
    pub fn register_wasm_from_import(mut self, import: ImportMod) -> Result<Self> {
        self.inner.register_wasm_from_import(import.inner)?;
        Ok(self)
    }

    // pub fn load_from_file() -> Result<Self> {
    //     unimplemented!()
    // }

    // pub fn load_from_buffer() -> Result<Self> {
    //     unimplemented!()
    // }

    // /// Loads a module.
    // pub fn load(self, mut module: Module) -> Result<Self> {
    //     self.inner.load_wasm_from_module(&mut module.inner)?;
    //     Ok(self)
    // }

    // pub fn validate(self) -> Result<Self> {
    //     self.inner.validate()?;
    //     Ok(self)
    // }

    // pub fn instantiate(self) -> Result<Self> {
    //     self.inner.instantiate()?;
    //     Ok(self)
    // }

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
            assert!(vm.config.is_none());

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
            assert!(vm.config.is_some());

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
        let config = result
            .unwrap()
            .cost_measuring(true)
            .time_measuring(true)
            .instr_counting(true);

        // create a Vm context
        let result = Vm::new(Some(config));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // get the statistics
        let result = vm.statistics_mut();
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_register_wasm_from_module() {
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
        let result = vm.register_wasm_from_module("extern", module);
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
