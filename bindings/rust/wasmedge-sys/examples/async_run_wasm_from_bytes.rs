//! This example demonstrates how to load and run wasm from bytes asynchronously.
//!
//! To run this example, use the following command:
//!
//! ```bash
//! cd <wasmedge-root-dir>/bindings/rust/
//!
//! cargo run -p wasmedge-sys --features async --example async_run_wasm_from_bytes
//! ```

#[cfg(feature = "async")]
use wasmedge_sys::{Config, Store, Vm, WasmValue};
#[cfg(feature = "async")]
use wasmedge_types::wat2wasm;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(feature = "async")]
    {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // run a function from a in-memory wasm bytes
        let result = wat2wasm(
            br#"(module
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

        let fut1 = vm.run_wasm_from_bytes_async(&wasm_bytes, "fib", [WasmValue::from_i32(20)]);
        let fut2 = vm.run_wasm_from_bytes_async(&wasm_bytes, "fib", [WasmValue::from_i32(5)]);

        let (ret1, ret2) = tokio::join!(fut1, fut2);
        let returns1 = ret1?;
        assert_eq!(returns1[0].to_i32(), 10946);
        let returns2 = ret2?;
        assert_eq!(returns2[0].to_i32(), 8);
    }

    Ok(())
}
