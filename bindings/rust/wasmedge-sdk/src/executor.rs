//! Defines Executor struct.

use crate::{config::Config, Engine, Func, FuncRef, Statistics, WasmEdgeResult, WasmValue};
use wasmedge_sys::{self as sys, Engine as SysEngine};

/// Defines an execution environment for both pure WASM and compiled WASM.
#[derive(Debug)]
pub struct Executor {
    pub(crate) inner: sys::Executor,
}
impl Executor {
    /// Creates a new [executor](crate::Executor) to be associated with the given [config](crate::config::Config) and [statistics](crate::Statistics).
    ///
    /// # Arguments
    ///
    /// - `config` specifies the configuration of the new [executor](crate::Executor).
    ///
    /// - `stat` specifies the [statistics](crate::Statistics) needed by the new [executor](crate::Executor).
    ///
    /// # Error
    ///
    /// If fail to create a [executor](crate::Executor), then an error is returned.
    pub fn new(config: Option<&Config>, stat: Option<&mut Statistics>) -> WasmEdgeResult<Self> {
        let inner_executor = match config {
            Some(config) => match stat {
                Some(stat) => {
                    sys::Executor::create(Some(config.inner.clone()), Some(&mut stat.inner))?
                }
                None => sys::Executor::create(Some(config.inner.clone()), None)?,
            },
            None => match stat {
                Some(stat) => sys::Executor::create(None, Some(&mut stat.inner))?,
                None => sys::Executor::create(None, None)?,
            },
        };

        Ok(Self {
            inner: inner_executor,
        })
    }
}
impl Engine for Executor {
    fn run_func(
        &self,
        func: &Func,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        let returns = self.inner.run_func(&func.inner, params)?;
        Ok(returns)
    }

    fn run_func_ref(
        &self,
        func_ref: &FuncRef,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        let returns = self.inner.run_func_ref(&func_ref.inner, params)?;
        Ok(returns)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        params, wat2wasm, Module, Statistics, Store, WasmVal,
    };

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_executor_create() {
        {
            let result = Executor::new(None, None);
            assert!(result.is_ok());
        }

        {
            let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
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
            let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
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
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
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

        // read the wasm bytes of fibonacci.wasm
        let result = wat2wasm(
            br#"
        (module
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
        let result = Module::from_bytes(Some(&config), wasm_bytes);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register a module into store as active module
        let result = store.register_named_module(&mut executor, "extern", &module);
        assert!(result.is_ok());
        let extern_instance = result.unwrap();

        // get the exported function "fib"
        let result = extern_instance.func("fib");
        assert!(result.is_some());
        let fib = result.unwrap();

        // run the exported host function
        let result = executor.run_func(&fib, params!(5));
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns.len(), 1);
        assert_eq!(returns[0].to_i32(), 8);
    }
}
