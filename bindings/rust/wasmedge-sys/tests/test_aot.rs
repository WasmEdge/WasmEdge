#[cfg(feature = "aot")]
use wasmedge_sys::{
    Compiler, CompilerOptimizationLevel, CompilerOutputFormat, Config, FuncType, Function,
    ImportObject, Value, Vm,
};

#[cfg(feature = "aot")]
#[test]
fn test_aot() {
    // create a Config context
    let result = Config::create();
    assert!(result.is_ok());
    let config = result.unwrap();
    // enable options
    let config = config
        .tail_call(true)
        .annotations(true)
        .memory64(true)
        .threads(true)
        .exception_handling(true)
        .function_references(true);

    // create a Vm context
    let result = Vm::create(Some(config), None);
    assert!(result.is_ok());
    let mut vm = result.unwrap();
    let import_obj = create_spec_test_module();
    let result = vm.register_wasm_from_import(import_obj);
    assert!(result.is_ok());

    // set the AOT compiler options
    let result = Config::create();
    assert!(result.is_ok());
    let config = result.unwrap();
    let config = config
        .set_optimization_level(CompilerOptimizationLevel::O0)
        .set_compiler_output_format(CompilerOutputFormat::Native);
    let result = Compiler::create(Some(config));
    assert!(result.is_ok());
    let compiler = result.unwrap();

    // compile a file for universal WASM output format
    let in_path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
    let out_path = std::path::PathBuf::from("fibonacci_aot.wasm");
    assert!(!out_path.exists());
    let result = compiler.compile(&in_path, &out_path);
    assert!(result.is_ok());
    assert!(out_path.exists());

    {
        // register the wasm module from the generated wasm file
        let result = vm.register_wasm_from_file("extern", &out_path);
        assert!(result.is_ok());

        let result = vm.contains_reg_func_name("extern", "fib");
        assert!(result.is_ok());

        let result = vm.run_registered_function("extern", "fib", [Value::from_i32(5)]);
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

        let result = vm.run_function("fib", [Value::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);
    }

    // remove the wasm file by the compiler
    assert!(std::fs::remove_file(&out_path).is_ok());
}

fn create_spec_test_module() -> ImportObject {
    // create an ImportObj module
    let result = ImportObject::create("spectest");
    assert!(result.is_ok());
    let mut import_obj = result.unwrap();

    // create a host function
    let result = FuncType::create([], []);
    assert!(result.is_ok());
    let func_ty = result.unwrap();
    let result = Function::create(func_ty, Box::new(spec_test_print), 0);
    assert!(result.is_ok());
    let host_func = result.unwrap();
    // add host function "print"
    import_obj.add_func("print", host_func);
    import_obj
}

fn spec_test_print(_inputs: Vec<Value>) -> Result<Vec<Value>, u8> {
    Ok(vec![])
}
