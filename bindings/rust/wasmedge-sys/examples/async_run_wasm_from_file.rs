use wasmedge_sys::{Config, Vm, WasmValue};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // create a Config context
    let result = Config::create();
    assert!(result.is_ok());
    let mut config = result.unwrap();
    config.bulk_memory_operations(true);
    assert!(config.bulk_memory_operations_enabled());
    config.interruptible(true);
    assert!(config.interruptible_enabled());

    // create a Vm context with the given Config and Store
    let result = Vm::create(Some(config), None);
    assert!(result.is_ok());
    let mut vm = result.unwrap();

    // run a function from a wasm file
    let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");

    let fut1 = vm.run_wasm_from_file_async2(&path, "fib", [WasmValue::from_i32(20)]);
    let fut2 = vm.run_wasm_from_file_async2(&path, "fib", [WasmValue::from_i32(5)]);

    let (ret1, ret2) = tokio::join!(fut1, fut2);
    let returns1 = ret1?;
    assert_eq!(returns1[0].to_i32(), 10946);
    let returns2 = ret2?;
    assert_eq!(returns2[0].to_i32(), 8);

    Ok(())
}
