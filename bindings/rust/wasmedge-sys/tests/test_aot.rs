#[cfg(feature = "aot")]
use wasmedge_sys::{
    AsImport, CallingFrame, Compiler, Config, Executor, FuncType, Function, ImportModule,
    ImportObject, Loader, Store, Validator, WasmValue,
};
#[cfg(feature = "aot")]
use wasmedge_types::{error::HostFuncError, CompilerOptimizationLevel, CompilerOutputFormat};

#[cfg(feature = "aot")]
#[test]
fn test_aot() -> Result<(), Box<dyn std::error::Error>> {
    // create a Config context
    let mut config = Config::create()?;
    // enable options
    config.tail_call(true);
    config.annotations(true);
    config.memory64(true);
    config.threads(true);
    config.exception_handling(true);
    config.function_references(true);
    config.set_aot_optimization_level(CompilerOptimizationLevel::O0);
    config.set_aot_compiler_output_format(CompilerOutputFormat::Native);
    config.interruptible(true);

    // create an executor
    let mut executor = Executor::create(Some(&config), None)?;

    // create a store
    let mut store = Store::create()?;

    let import = create_spec_test_module();
    let import_obj = ImportObject::Import(import);
    executor.register_import_object(&mut store, &import_obj)?;

    // set the AOT compiler options
    let result = Compiler::create(Some(&config));
    assert!(result.is_ok());
    let compiler = result.unwrap();

    // compile a file for universal WASM output format
    let in_path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wat");
    #[cfg(target_os = "macos")]
    let out_path = std::path::PathBuf::from("fibonacci_aot.dylib");
    #[cfg(target_os = "linux")]
    let out_path = std::path::PathBuf::from("fibonacci_aot.so");
    #[cfg(target_os = "windows")]
    let out_path = std::path::PathBuf::from("fibonacci_aot.dll");
    assert!(!out_path.exists());
    let result = compiler.compile_from_file(in_path, &out_path);
    assert!(result.is_ok());
    assert!(out_path.exists());

    {
        // register the wasm module as named module
        let extern_module = Loader::create(Some(&config))?.from_file(&out_path)?;
        Validator::create(Some(&config))?.validate(&extern_module)?;
        let extern_instance =
            executor.register_named_module(&mut store, &extern_module, "extern")?;

        let fib = extern_instance.get_func("fib")?;
        let returns = executor.call_func(&fib, [WasmValue::from_i32(5)])?;
        assert_eq!(returns[0].to_i32(), 8);
    }

    {
        // register the wasm module as active module
        let active_module = Loader::create(Some(&config))?.from_file(&out_path)?;
        Validator::create(Some(&config))?.validate(&active_module)?;
        let active_instance = executor.register_active_module(&mut store, &active_module)?;

        let fib = active_instance.get_func("fib")?;
        let returns = executor.call_func(&fib, [WasmValue::from_i32(5)])?;
        assert_eq!(returns[0].to_i32(), 8);
    }

    // remove the wasm file by the compiler
    assert!(std::fs::remove_file(&out_path).is_ok());

    Ok(())
}

#[cfg(feature = "aot")]
fn create_spec_test_module() -> ImportModule {
    // create an ImportObj module
    let result = ImportModule::create("spectest");
    assert!(result.is_ok());
    let mut import = result.unwrap();

    // create a host function
    let result = FuncType::create([], []);
    assert!(result.is_ok());
    let func_ty = result.unwrap();
    let result = Function::create(&func_ty, Box::new(spec_test_print), 0);
    assert!(result.is_ok());
    let host_func = result.unwrap();
    // add host function "print"
    import.add_func("print", host_func);
    import
}

#[cfg(feature = "aot")]
fn spec_test_print(
    _frame: CallingFrame,
    _inputs: Vec<WasmValue>,
) -> Result<Vec<WasmValue>, HostFuncError> {
    Ok(vec![])
}
