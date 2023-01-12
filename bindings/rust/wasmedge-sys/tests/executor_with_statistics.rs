mod common;
use wasmedge_sys::{
    Config, Engine, Executor, ImportObject, Loader, Statistics, Store, Validator, WasmValue,
};
use wasmedge_types::error::{
    CoreError, CoreExecutionError, InstanceError, StoreError, WasmEdgeError,
};

#[warn(unused_assignments)]
#[test]
fn test_executor_with_statistics() {
    // create a Config context
    let result = Config::create();
    assert!(result.is_ok());
    let mut config = result.unwrap();
    // enable Statistics
    config.count_instructions(true);
    config.measure_time(true);
    config.measure_cost(true);

    // create a Statistics context
    let result = Statistics::create();
    assert!(result.is_ok());
    let mut stat = result.unwrap();
    // set cost table
    stat.set_cost_table([]);
    let mut cost_table = vec![20u64; 512];
    stat.set_cost_table(&mut cost_table);
    // set cost limit
    stat.set_cost_limit(100_000_000_000_000);

    // create an Executor context
    let result = Executor::create(Some(config), Some(&mut stat));
    assert!(result.is_ok());
    let mut executor = result.unwrap();

    // create an ImportObj module
    let import = common::create_extern_module("extern");

    // create a Store context
    let result = Store::create();
    assert!(result.is_ok());
    let mut store = result.unwrap();

    // register the import_obj module into the store context
    let import = ImportObject::Import(import);
    let result = executor.register_import_object(&mut store, &import);
    assert!(result.is_ok());

    // load module from a wasm file
    let result = Config::create();
    assert!(result.is_ok());
    let config = result.unwrap();
    let result = Loader::create(Some(config));
    assert!(result.is_ok());
    let loader = result.unwrap();
    let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/tests/data/test.wat");
    let result = loader.from_file(path);
    assert!(result.is_ok());
    let module = result.unwrap();

    // validate module
    let result = Config::create();
    assert!(result.is_ok());
    let config = result.unwrap();
    let result = Validator::create(Some(config));
    assert!(result.is_ok());
    let validator = result.unwrap();
    let result = validator.validate(&module);
    assert!(result.is_ok());

    // register a wasm module into the store context
    let result = executor.register_named_module(&mut store, &module, "module");
    assert!(result.is_ok());

    // load module from a wasm file
    let result = Config::create();
    assert!(result.is_ok());
    let config = result.unwrap();
    let result = Loader::create(Some(config));
    assert!(result.is_ok());
    let loader = result.unwrap();
    let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/tests/data/test.wat");
    let result = loader.from_file(path);
    assert!(result.is_ok());
    let module = result.unwrap();

    // validate module
    let result = Config::create();
    assert!(result.is_ok());
    let config = result.unwrap();
    let result = Validator::create(Some(config));
    assert!(result.is_ok());
    let validator = result.unwrap();
    let result = validator.validate(&module);
    assert!(result.is_ok());

    // register a wasm module as an active module
    let result = executor.register_active_module(&mut store, &module);
    assert!(result.is_ok());
    let active_instance = result.unwrap();

    // get the exported functions from the active module
    let result = active_instance.get_func("func-mul-2");
    assert!(result.is_ok());
    let func_mul_2 = result.unwrap();
    let result = executor.run_func(
        &func_mul_2,
        [WasmValue::from_i32(123), WasmValue::from_i32(456)],
    );
    assert!(result.is_ok());
    let returns = result.unwrap();
    let returns = returns.iter().map(|x| x.to_i32()).collect::<Vec<_>>();
    assert_eq!(returns, vec![246, 912]);

    // function type mismatched
    let result = executor.run_func(&func_mul_2, []);
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::FuncTypeMismatch
        )))
    );

    // function type mismatched
    let result = executor.run_func(
        &func_mul_2,
        [WasmValue::from_i64(123), WasmValue::from_i32(456)],
    );
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::FuncTypeMismatch
        )))
    );

    // try to get non-existent exported function
    let result = active_instance.get_func("func-mul-3");
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        Box::new(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
            "func-mul-3".into()
        )))
    );

    // call host function by using external reference
    let result = active_instance.get_table("tab-ext");
    assert!(result.is_ok());
    let mut table = result.unwrap();

    let mut test_value = 0u32;
    let test_value_ref = &mut test_value;

    let data = WasmValue::from_extern_ref(test_value_ref);
    let result = table.set_data(data, 0);
    assert!(result.is_ok());
    let result = table.set_data(data, 1);
    assert!(result.is_ok());
    let result = table.set_data(data, 2);
    assert!(result.is_ok());
    let result = table.set_data(data, 3);
    assert!(result.is_ok());

    // get the exported host function named "func-host-add"
    let result = active_instance.get_func("func-host-add");
    assert!(result.is_ok());
    let func_host_add = result.unwrap();
    // Call add: (777) + (223)
    test_value = 777;
    let result = executor.run_func(&func_host_add, [WasmValue::from_i32(223)]);
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), 1000);

    // get the exported host function named "func-host-add"
    let result = active_instance.get_func("func-host-sub");
    assert!(result.is_ok());
    let func_host_sub = result.unwrap();
    // Call sub: (123) - (456)
    test_value = 123;
    let result = executor.run_func(&func_host_sub, [WasmValue::from_i32(456)]);
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), -333);

    // get the exported host function named "func-host-add"
    let result = active_instance.get_func("func-host-mul");
    assert!(result.is_ok());
    let func_host_mul = result.unwrap();
    // Call mul: (-30) * (-66)
    test_value = -30i32 as u32;
    let result = executor.run_func(&func_host_mul, [WasmValue::from_i32(-66)]);
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), 1980);

    // get the exported host function named "func-host-add"
    let result = active_instance.get_func("func-host-div");
    assert!(result.is_ok());
    let func_host_div = result.unwrap();
    // Call div: (-9999) / (1234)
    test_value = -9999i32 as u32;
    let result = executor.run_func(&func_host_div, [WasmValue::from_i32(1234)]);
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), -8);

    // get the module instance named "extern"
    let result = store.module("extern");
    assert!(result.is_ok());
    let extern_instance = result.unwrap();

    // get the exported host function named "func-add"
    let result = extern_instance.get_func("func-add");
    assert!(result.is_ok());
    let func_add = result.unwrap();
    // Invoke the functions in the registered module
    test_value = 5000;
    let result = executor.run_func(
        &func_add,
        [
            WasmValue::from_extern_ref(&mut test_value),
            WasmValue::from_i32(1500),
        ],
    );
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), 6500);
    // Function type mismatch
    let result = executor.run_func(&func_add, []);
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::FuncTypeMismatch
        )))
    );
    // Function type mismatch
    let result = executor.run_func(
        &func_add,
        [
            WasmValue::from_extern_ref(&mut test_value),
            WasmValue::from_i64(1500),
        ],
    );
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::FuncTypeMismatch
        )))
    );
    // Module not found
    let result = store.module("error-name");
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        Box::new(WasmEdgeError::Store(StoreError::NotFoundModule(
            "error-name".into()
        )))
    );
    // Function not found
    let result = extern_instance.get_func("func-add2");
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        Box::new(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
            "func-add2".into()
        )))
    );

    // get the exported host function named "func-term"
    let result = extern_instance.get_func("func-term");
    assert!(result.is_ok());
    let func_term = result.unwrap();
    // Invoke host function to terminate execution
    let result = executor.run_func(&func_term, []);
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), 1234);

    // get the exported host function named "func-term"
    let result = extern_instance.get_func("func-fail");
    assert!(result.is_ok());
    let func_fail = result.unwrap();
    // Invoke host function to fail execution
    let result = executor.run_func(&func_fail, []);
    assert!(result.is_err());
    assert_eq!(result.unwrap_err(), Box::new(WasmEdgeError::User(2)));
}
