//! This example demonstrates the concurrent execution of multiple host functions with `Vm::run_func_async` API.
//!
//! To run this example, use the following command:
//! ```bash
//! cd <wasmedge-root-dir>/bindings/rust/
//! cargo run -p wasmedge-sdk --features async --example async_run_func
//! ```
#[cfg(feature = "async")]
use wasmedge_sdk::{
    config::{CommonConfigOptions, ConfigBuilder},
    params, VmBuilder, WasmVal,
};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(feature = "async")]
    {
        let wasm_file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sdk/examples/data/fibonacci.wat");

        let config = ConfigBuilder::new(CommonConfigOptions::default()).build()?;
        assert!(config.bulk_memory_operations_enabled());

        // create Vm instance
        let vm = VmBuilder::new()
            .with_config(config)
            .build()?
            .register_module_from_file("extern", wasm_file)?;

        // async run function
        let fut1 = vm.run_func_async(Some("extern"), "fib", params!(20));

        let fut2 = vm.run_func_async(Some("extern"), "fib", params!(5));

        let (ret1, ret2) = tokio::join!(fut1, fut2);

        let returns1 = ret1?;
        assert_eq!(returns1[0].to_i32(), 10946);
        println!("fib(20) = {}", returns1[0].to_i32());
        let returns2 = ret2?;
        assert_eq!(returns2[0].to_i32(), 8);
        println!("fib(5) = {}", returns2[0].to_i32());
    }

    Ok(())
}
