//! This example demonstrates how to load and run wasm from file asynchronously.
//!
//! To run this example, use the following command:
//!
//! ```bash
//! cd <wasmedge-root-dir>/bindings/rust/
//!
//! cargo run -p wasmedge-sys --features async --example async_run_wasm_from_file
//! ```

#[cfg(feature = "async")]
use wasmedge_sys::{Config, Vm, WasmValue};

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
        config.interruptible(true);
        assert!(config.interruptible_enabled());

        // create a Vm context with the given Config and Store
        let vm = Vm::create(Some(config), None)?;
        // run a function from a wasm file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wat");

        let fut1 = vm.run_wasm_from_file_async(&path, "fib", [WasmValue::from_i32(20)]);
        let fut2 = vm.run_wasm_from_file_async(&path, "fib", [WasmValue::from_i32(5)]);

        let (ret1, ret2) = tokio::join!(fut1, fut2);
        let returns1 = ret1?;
        assert_eq!(returns1[0].to_i32(), 10946);
        let returns2 = ret2?;
        assert_eq!(returns2[0].to_i32(), 8);
    }

    Ok(())
}
