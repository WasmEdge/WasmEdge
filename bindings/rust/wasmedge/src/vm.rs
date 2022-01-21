use crate::{error::WasmEdgeResult, wasmedge, Config, ImportObj, Module, Store, Value};
use std::{env::args, marker::PhantomData, path::Path};

// use crate::{config::Config, error::VmError, module::Module, wasi_conf::WasiConf};

#[derive(Debug)]
pub struct VmBuilder<'a> {
    config: Option<&'a Config>,
    store: Option<&'a Store<'a>>,
    vm: Option<wasmedge::Vm>,
}

impl<'a> VmBuilder<'a> {
    pub fn new() -> Self {
        Self {
            vm: None,
            config: None,
            store: None,
        }
    }

    pub fn with_config(self, config: &'a Config) -> Self {
        Self {
            config: Some(config),
            vm: self.vm,
            store: self.store,
        }
    }

    pub fn with_store(self, store: &'a Store) -> Self {
        Self {
            config: self.config,
            store: Some(store),
            vm: self.vm,
        }
    }

    pub fn build(self) -> WasmEdgeResult<Vm> {
        let inner = if self.config.is_some() && self.store.is_some() {
            wasmedge::Vm::create(
                Some(&self.config.unwrap().inner),
                Some(&self.store.unwrap().inner),
            )?
        } else if self.config.is_some() {
            wasmedge::Vm::create(Some(&self.config.unwrap().inner), None)?
        } else if self.store.is_some() {
            wasmedge::Vm::create(None, Some(&self.store.unwrap().inner))?
        } else {
            wasmedge::Vm::create(None, None)?
        };

        Ok(Vm { inner })
    }
}

#[derive(Debug)]
pub struct Vm {
    inner: wasmedge::Vm,
}
impl Vm {
    pub fn store_mut(&self) -> WasmEdgeResult<Store> {
        let inner = self.inner.store_mut()?;
        Ok(Store {
            inner,
            _marker: PhantomData,
        })
    }

    pub fn reset(&mut self) {
        self.inner.reset()
    }

    pub fn register_wasm_from_module(
        self,
        mod_name: impl AsRef<str>,
        module: &mut Module,
    ) -> WasmEdgeResult<Self> {
        let inner = self
            .inner
            .register_wasm_from_module(mod_name.as_ref(), &mut module.inner)?;
        Ok(Self { inner })
    }

    pub fn register_wasm_from_import(self, import_obj: &mut ImportObj) -> WasmEdgeResult<Self> {
        let inner = self
            .inner
            .register_wasm_from_import(&mut import_obj.inner)?;
        Ok(Self { inner })
    }

    pub fn run_wasm_from_file(
        &self,
        file: impl AsRef<Path>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        let returns = self
            .inner
            .run_wasm_from_file(file.as_ref(), func_name.as_ref(), args)?;
        Ok(returns.collect::<Vec<_>>())
    }

    pub fn run_wasm_from_buffer(
        &self,
        buffer: &[u8],
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        let returns = self
            .inner
            .run_wasm_from_buffer(buffer.as_ref(), func_name.as_ref(), args)?;
        Ok(returns.collect::<Vec<_>>())
    }

    pub fn run_wasm_from_module(
        &self,
        module: &mut Module,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        let returns =
            self.inner
                .run_wasm_from_module(&mut module.inner, func_name.as_ref(), args)?;
        Ok(returns.collect::<Vec<_>>())
    }
}

pub trait Engine {
    fn run_func();

    fn run_func_registered();
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{ConfigBuilder, Store};

    #[test]
    fn test_vm_create() {
        {
            let result = VmBuilder::new().build();
            assert!(result.is_ok());
            let vm = result.unwrap();

            let result = vm.store_mut();
            assert!(result.is_ok());
        }

        {
            // create a Config
            let config = ConfigBuilder::new()
                .expect("fail to create a ConfigBuilder")
                .build();

            // create a Vm context
            let result = VmBuilder::new().with_config(&config).build();
            assert!(result.is_ok());
            let vm = result.unwrap();

            let result = vm.store_mut();
            assert!(result.is_ok());
        }

        {
            // create a Store
            let result = Store::new();
            assert!(result.is_ok());
            let store = result.unwrap();

            // create a Vm context
            let result = VmBuilder::new().with_store(&store).build();
            assert!(result.is_ok());
            let vm = result.unwrap();

            let result = vm.store_mut();
            assert!(result.is_ok());
        }

        {
            // create a Config
            let config = ConfigBuilder::new()
                .expect("fail to create a ConfigBuilder")
                .build();

            // create a Store
            let result = Store::new();
            assert!(result.is_ok());
            let store = result.unwrap();

            // create a Vm context
            let result = VmBuilder::new()
                .with_config(&config)
                .with_store(&store)
                .build();
            assert!(result.is_ok());
            let vm = result.unwrap();

            let result = vm.store_mut();
            assert!(result.is_ok());
        }
    }
}
