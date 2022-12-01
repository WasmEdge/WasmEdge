#[cfg(feature = "aot")]
use wasmedge_sys::{
    AsImport, CallingFrame, Compiler, Config, FuncType, Function, ImportModule, ImportObject, Vm,
    WasmValue,
};
#[cfg(feature = "aot")]
use wasmedge_types::{error::HostFuncError, CompilerOptimizationLevel, CompilerOutputFormat};

#[cfg(feature = "aot")]
#[test]
fn test_aot() {
    // create a Config context
    let result = Config::create();
    assert!(result.is_ok());
    let mut config = result.unwrap();
    // enable options
    config.tail_call(true);
    config.annotations(true);
    config.memory64(true);
    config.threads(true);
    config.exception_handling(true);
    config.function_references(true);

    // create a Vm context
    let result = Vm::create(Some(config), None);
    assert!(result.is_ok());
    let mut vm = result.unwrap();
    let import = create_spec_test_module();
    let result = vm.register_wasm_from_import(ImportObject::Import(import));
    assert!(result.is_ok());

    // set the AOT compiler options
    let result = Config::create();
    assert!(result.is_ok());
    let mut config = result.unwrap();
    config.set_aot_optimization_level(CompilerOptimizationLevel::O0);
    config.set_aot_compiler_output_format(CompilerOutputFormat::Native);
    config.interruptible(true);
    let result = Compiler::create(Some(config));
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
        // register the wasm module from the generated wasm file
        let result = vm.register_wasm_from_file("extern", &out_path);
        assert!(result.is_ok());

        let result = vm.run_registered_function("extern", "fib", [WasmValue::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);
    }

    {
        let result = vm.load_wasm_from_file(&out_path);
        assert!(result.is_ok());

        let result = vm.validate();
        assert!(result.is_ok());

        let result = vm.instantiate();
        assert!(result.is_ok());

        let result = vm.run_function("fib", [WasmValue::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);
    }

    // remove the wasm file by the compiler
    assert!(std::fs::remove_file(&out_path).is_ok());
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
