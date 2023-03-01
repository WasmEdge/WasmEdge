//! This example demonstrates how to call functions asynchronously.
//!
//! To run this example, use the following command:
//!
//! ```bash
//! cd <wasmedge-root-dir>/bindings/rust/
//!
//! cargo run -p wasmedge-sys --features async --example async_run_func
//! ```

#[cfg(feature = "async")]
use wasmedge_sys::{Config, Executor, Loader, Store, WasmValue};
#[cfg(feature = "async")]
use wasmedge_types::wat2wasm;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(feature = "async")]
    {
        let wasm_bytes = wat2wasm(
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
        )?;

        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // load module from file
        let result = Loader::create(Some(&config));
        assert!(result.is_ok());
        let loader = result.unwrap();
        let result = loader.from_bytes(&wasm_bytes);
        assert!(result.is_ok());
        let ast_module = result.unwrap();

        // create config
        let mut config = Config::create()?;
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create an executor
        let mut executor = Executor::create(Some(&config), None)?;

        // create a store
        let mut store = Store::create()?;

        // register module
        let fib = executor
            .register_active_module(&mut store, &ast_module)?
            .get_func("fib")?;

        // async run function
        let fut1 = executor.call_func_async(&fib, vec![WasmValue::from_i32(20)]);
        let fut2 = executor.call_func_async(&fib, vec![WasmValue::from_i32(5)]);

        let returns = tokio::join!(fut1, fut2);

        let (ret1, ret2) = returns;
        let returns1 = ret1?;
        assert_eq!(returns1[0].to_i32(), 10946);
        let returns2 = ret2?;
        assert_eq!(returns2[0].to_i32(), 8);
    }

    Ok(())
}
