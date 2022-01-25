use crate::{error::WasmEdgeResult, wasmedge, Config, ImportObj, Module, Store, Value};
use std::{marker::PhantomData, path::Path};

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

    // validate + instantiate + register
    pub fn register_wasm_from_module(
        mut self,
        mod_name: impl AsRef<str>,
        module: &mut Module,
    ) -> WasmEdgeResult<Self> {
        self.inner
            .register_wasm_from_module(mod_name.as_ref(), &mut module.inner)?;
        Ok(self)
    }

    // validate + instantiate + register
    pub fn register_wasm_from_import(mut self, import_obj: &mut ImportObj) -> WasmEdgeResult<Self> {
        self.inner
            .register_wasm_from_import(&mut import_obj.inner)?;
        Ok(self)
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
        Ok(returns)
    }

    pub fn run_wasm_from_buffer(
        &self,
        buffer: &[u8],
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        let returns = self
            .inner
            .run_wasm_from_buffer(buffer.as_ref(), func_name.as_ref(), args)?;
        Ok(returns)
    }

    pub fn run_wasm_from_module(
        &self,
        module: &mut Module,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        let returns =
            self.inner
                .run_wasm_from_module(&mut module.inner, func_name.as_ref(), args)?;
        Ok(returns)
    }

    pub fn load_from_file() -> WasmEdgeResult<Self> {
        unimplemented!()
    }

    pub fn load_from_buffer() -> WasmEdgeResult<Self> {
        unimplemented!()
    }

    pub fn load_from_module() -> WasmEdgeResult<Self> {
        unimplemented!()
    }

    pub fn validate() -> WasmEdgeResult<Self> {
        unimplemented!()
    }

    pub fn instantiate() -> WasmEdgeResult<Self> {
        unimplemented!()
    }

    pub fn run_func(
        &self,
        mod_name: Option<&str>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        match mod_name {
            Some(mod_name) => {
                // run a function in the registered module
                // self.inner
                //     .run_registered_function(mod_name, func_name.as_ref(), args)
                todo!()
            }
            None => {
                // run a function in the active module
                // self.inner.run_function(func_name.as_ref(), args)
                todo!()
            }
        }
    }
}

pub trait Engine {
    fn register_wasm_from_module();
    fn register_wasm_from_import();
    fn run_func();
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
