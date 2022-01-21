use crate::{error::WasmEdgeResult, wasmedge, Config, Store};
use std::{marker::PhantomData, path::Path};

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
    pub fn register_from_file(
        self,
        mod_name: impl AsRef<str>,
        path: impl AsRef<Path>,
    ) -> WasmEdgeResult<Self> {
        let inner = self
            .inner
            .register_wasm_from_file(mod_name.as_ref(), path.as_ref())?;
        Ok(Self { inner })
    }

    pub fn store_mut(&self) -> WasmEdgeResult<Store> {
        let inner = self.inner.store_mut()?;
        Ok(Store {
            inner,
            _marker: PhantomData,
        })
    }
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

    #[test]
    fn test_vm_register_wasm_from_file() {
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

        // register a wasm module from a file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("tools/wasmedge/examples/fibonacci.wasm");
        let result = vm.register_from_file("fib-module", path);
        assert!(result.is_ok());
    }
}
