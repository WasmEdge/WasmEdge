use crate::{error::WasmEdgeResult, wasmedge, Config, Store};

// use crate::{config::Config, error::VmError, module::Module, wasi_conf::WasiConf};

#[derive(Debug)]
pub struct VmBuilder<'a> {
    config: Option<&'a Config>,
    store: Option<&'a Store>,
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{ConfigBuilder, Store};

    #[test]
    fn test_vm_create() {
        {
            let result = VmBuilder::new().build();
            assert!(result.is_ok());
            dbg!(result.unwrap());
        }

        {
            // create a Config
            let config = ConfigBuilder::new()
                .expect("fail to create a ConfigBuilder")
                .build();

            // create a Vm context
            let result = VmBuilder::new().with_config(&config).build();
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
        }
    }
}
