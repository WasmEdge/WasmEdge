use crate::{error::Result, wasmedge, Config, Statistics, Store, Value};

#[derive(Debug)]
pub struct Executor {
    pub(crate) inner: wasmedge::Executor,
}
impl Executor {
    pub fn new(config: Option<&Config>, stat: Option<&mut Statistics>) -> Result<Self> {
        let inner_config = match config {
            Some(config) => Some(Config::copy_from(config)?.inner),
            None => None,
        };

        let inner_executor = match stat {
            Some(stat) => wasmedge::Executor::create(inner_config, Some(&mut stat.inner))?,
            None => wasmedge::Executor::create(inner_config, None)?,
        };

        Ok(Self {
            inner: inner_executor,
        })
    }

    pub fn run_func(
        &mut self,
        store: &mut Store,
        mod_name: Option<&str>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = Value>,
    ) -> Result<Vec<Value>> {
        let returns = match mod_name {
            Some(mod_name) => {
                // run a function in the registered module
                self.inner.run_func_registered(
                    &mut store.inner,
                    mod_name,
                    func_name.as_ref(),
                    args,
                )?
            }
            None => {
                // run a function in the active module
                self.inner
                    .run_func(&mut store.inner, func_name.as_ref(), args)?
            }
        };

        Ok(returns)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{Config, Module, Statistics};

    #[test]
    fn test_executor_create() {
        {
            let result = Executor::new(None, None);
            assert!(result.is_ok());
        }

        {
            let result = Config::new();
            assert!(result.is_ok());
            let config = result.unwrap();

            let result = Executor::new(Some(&config), None);
            assert!(result.is_ok());

            assert!(config.bulk_memory_operations_enabled());
        }

        {
            let result = Statistics::new();
            assert!(result.is_ok());
            let mut stat = result.unwrap();

            let result = Executor::new(None, Some(&mut stat));
            assert!(result.is_ok());

            assert_eq!(stat.cost_in_total(), 0);
        }

        {
            let result = Config::new();
            assert!(result.is_ok());
            let config = result.unwrap();

            let result = Statistics::new();
            assert!(result.is_ok());
            let mut stat = result.unwrap();

            let result = Executor::new(Some(&config), Some(&mut stat));
            assert!(result.is_ok());

            assert!(config.bulk_memory_operations_enabled());
            assert_eq!(stat.cost_in_total(), 0);
        }
    }

    #[test]
    fn test_executor_run_func() {
        // create an executor
        let result = Config::new();
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
        let result = store.register_named_module(&mut executor, "extern", &module);
        assert!(result.is_ok());

        // run the exported host function
        let result = executor.run_func(&mut store, Some("extern"), "fib", [Value::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns, [Value::from_i32(8)]);
    }
}
