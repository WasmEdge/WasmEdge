//! This example demonstrates how to call registered functions asynchronously.
//!
//! To run this example, use the following command:
//!
//! ```bash
//! cd <wasmedge-root-dir>/bindings/rust/
//!
//! cargo run -p wasmedge-sys --features async --example async_run_registered_func
//! ```

#[cfg(feature = "async")]
use wasmedge_sys::{Config, Store, Vm, WasmValue};

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

        // register a wasm module from a buffer
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = std::fs::read(path);
        assert!(result.is_ok());
        let buffer = result.unwrap();
        let result = vm.register_wasm_from_bytes("extern", &buffer);
        assert!(result.is_ok());

        // async run function
        let fut1 = vm.run_registered_function_async("extern", "fib", vec![WasmValue::from_i32(20)]);

        let fut2 = vm.run_registered_function_async("extern", "fib", vec![WasmValue::from_i32(5)]);

        let returns = tokio::join!(fut1, fut2);

        let (ret1, ret2) = returns;
        let returns1 = ret1?;
        assert_eq!(returns1[0].to_i32(), 10946);
        let returns2 = ret2?;
        assert_eq!(returns2[0].to_i32(), 8);
    }

    Ok(())
}
