//! This example demonstrates how to call functions asynchronously.
//!
//! To run this example, use the following command:
//!
//! ```bash
//! cd /wasmedge-root-dir/bindings/rust/
//!
//! cargo run -p wasmedge-sys --example async_run_func
//! ```

use wasmedge_sys::{Config, Loader, Store, Vm, WasmValue};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
    let result = Config::create();
    assert!(result.is_ok());
    let mut config = result.unwrap();
    config.bulk_memory_operations(true);
    assert!(config.bulk_memory_operations_enabled());

    // load module from file
    let result = Loader::create(Some(config));
    assert!(result.is_ok());
    let loader = result.unwrap();
    let result = loader.from_file(path);
    assert!(result.is_ok());
    let ast_module = result.unwrap();

    // create Vm instance
    let result = Config::create();
    assert!(result.is_ok());
    let mut config = result.unwrap();
    config.bulk_memory_operations(true);
    assert!(config.bulk_memory_operations_enabled());

    let result = Store::create();
    assert!(result.is_ok());
    let mut store = result.unwrap();

    let result = Vm::create(Some(config), Some(&mut store));
    assert!(result.is_ok());
    let vm = result.unwrap();

    // load wasm module from a ast module instance
    let result = vm.load_wasm_from_module(&ast_module);
    assert!(result.is_ok());

    // validate vm instance
    let result = vm.validate();
    assert!(result.is_ok());

    // instantiate
    let result = vm.instantiate();
    assert!(result.is_ok());

    // async run function
    let fut1 = vm.run_function_async(String::from("fib"), vec![WasmValue::from_i32(20)]);

    let fut2 = vm.run_function_async(String::from("fib"), vec![WasmValue::from_i32(5)]);
    // println!("{:?}", fut1.await);
    let returns = tokio::join!(fut1, fut2);

    let (ret1, ret2) = returns;
    let returns1 = ret1?;
    assert_eq!(returns1[0].to_i32(), 10946);
    let returns2 = ret2?;
    assert_eq!(returns2[0].to_i32(), 8);

    Ok(())
}
