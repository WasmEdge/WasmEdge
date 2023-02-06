//! This example demonstrates how to load and run wasm from file asynchronously.
//!
//! To run this example, use the following command:
//!
//! ```bash
//! cd <wasmedge-root-dir>/bindings/rust/
//!
//! cargo run -p wasmedge-sys --features async,aot --example async_run_wasm_from_file
//! ```

#[cfg(all(feature = "async", feature = "aot"))]
use wasmedge_sys::{Config, Vm, WasmValue};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(all(feature = "async", feature = "aot"))]
    {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());
        config.interruptible(true);
        assert!(config.interruptible_enabled());

        // create a Vm context with the given Config and Store
        let mut vm = Vm::create(Some(config))?;
        // run a function from a wasm file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wat");

        let returns = vm
            .run_wasm_from_file_async(&path, "fib", [WasmValue::from_i32(20)])
            .await?;
        assert_eq!(returns[0].to_i32(), 10946);
    }

    Ok(())
}
