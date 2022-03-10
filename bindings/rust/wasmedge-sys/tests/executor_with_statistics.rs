mod common;
use wasmedge_sys::{
    error::{CoreError, CoreExecutionError},
    Config, Executor, Loader, Statistics, Store, StoreError, Validator, Value, WasmEdgeError,
};

#[warn(unused_assignments)]
#[test]
fn test_executor_with_statistics() {
    // create a Config context
    let result = Config::create();
    assert!(result.is_ok());
    let mut config = result.unwrap();
    // enable Statistics
    config.aot_count_instructions(true);
    config.aot_measure_time(true);
    config.aot_measure_cost(true);

    // create a Statistics context
    let result = Statistics::create();
    assert!(result.is_ok());
    let mut stat = result.unwrap();
    // set cost table
    stat.set_cost_table(&mut []);
    let mut cost_table = vec![20u64; 512];
    stat.set_cost_table(&mut cost_table);
    // set cost limit
    stat.set_cost_limit(100_000_000_000_000);

    // create an Executor context
    let result = Executor::create(Some(config), Some(&mut stat));
    assert!(result.is_ok());
    let executor = result.unwrap();

    // create an ImportObj module
    let import_obj = common::create_extern_module("extern");

    // create a Store context
    let result = Store::create();
    assert!(result.is_ok());
    let mut store = result.unwrap();

    // register the import_obj module into the store context
    let result = executor.register_import_object(&mut store, &import_obj);
    assert!(result.is_ok());
    let executor = result.unwrap();

    // load module from a wasm file
    let result = Config::create();
    assert!(result.is_ok());
    let config = result.unwrap();
    let result = Loader::create(Some(config));
    assert!(result.is_ok());
    let loader = result.unwrap();
    let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/tests/data/test.wasm");
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
    let result = executor.register_module(&mut store, &module, "module");
    assert!(result.is_ok());
    let executor = result.unwrap();

    // load module from a wasm file
    let result = Config::create();
    assert!(result.is_ok());
    let config = result.unwrap();
    let result = Loader::create(Some(config));
    assert!(result.is_ok());
    let loader = result.unwrap();
    let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/tests/data/test.wasm");
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

    // instantiate wasm module
    let result = executor.instantiate(&mut store, &module);
    assert!(result.is_ok());
    let mut executor = result.unwrap();

    // invoke the registered function in the module
    let result = executor.run_func(
        &mut store,
        "func-mul-2",
        [Value::from_i32(123), Value::from_i32(456)],
    );
    assert!(result.is_ok());
    let returns = result.unwrap();
    let returns = returns.iter().map(|x| x.to_i32()).collect::<Vec<_>>();
    assert_eq!(returns, vec![246, 912]);
    // function type mismatched
    let result = executor.run_func(&mut store, "func-mul-2", []);
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
    );
    // function type mismatched
    let result = executor.run_func(
        &mut store,
        "func-mul-2",
        [Value::from_i64(123), Value::from_i32(456)],
    );
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
    );
    // function not found
    let result = executor.run_func(
        &mut store,
        "func-mul-3",
        [Value::from_i32(123), Value::from_i32(456)],
    );
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        WasmEdgeError::Store(StoreError::NotFoundFunc("func-mul-3".into()))
    );

    // call host function by using external reference
    let result = store.find_table("tab-ext");
    assert!(result.is_ok());
    let mut table = result.unwrap();

    let mut test_value = 0u32;
    let test_value_ref = &mut test_value;

    let data = Value::from_extern_ref(test_value_ref);
    let result = table.set_data(data, 0);
    assert!(result.is_ok());
    let result = table.set_data(data, 1);
    assert!(result.is_ok());
    let result = table.set_data(data, 2);
    assert!(result.is_ok());
    let result = table.set_data(data, 3);
    assert!(result.is_ok());

    // Call add: (777) + (223)
    test_value = 777;
    let result = executor.run_func(&mut store, "func-host-add", [Value::from_i32(223)]);
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), 1000);

    // Call sub: (123) - (456)
    test_value = 123;
    let result = executor.run_func(&mut store, "func-host-sub", [Value::from_i32(456)]);
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), -333);

    // Call mul: (-30) * (-66)
    test_value = -30i32 as u32;
    let result = executor.run_func(&mut store, "func-host-mul", [Value::from_i32(-66)]);
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), 1980);

    // Call div: (-9999) / (1234)
    test_value = -9999i32 as u32;
    let result = executor.run_func(&mut store, "func-host-div", [Value::from_i32(1234)]);
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), -8);

    // Invoke the functions in the registered module
    test_value = 5000;
    let result = executor.run_func_registered(
        &mut store,
        "extern",
        "func-add",
        [
            Value::from_extern_ref(&mut test_value),
            Value::from_i32(1500),
        ],
    );
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), 6500);
    // Function type mismatch
    let result = executor.run_func_registered(&mut store, "extern", "func-add", []);
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
    );
    // Function type mismatch
    let result = executor.run_func_registered(
        &mut store,
        "extern",
        "func-add",
        [
            Value::from_extern_ref(&mut test_value),
            Value::from_i64(1500),
        ],
    );
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
    );
    // Module not found
    let result = executor.run_func_registered(
        &mut store,
        "error-name",
        "func-add",
        [
            Value::from_extern_ref(&mut test_value),
            Value::from_i32(1500),
        ],
    );
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        WasmEdgeError::Store(StoreError::NotFoundModule("error-name".into()))
    );
    // Function not found
    let result = executor.run_func_registered(
        &mut store,
        "extern",
        "func-add2",
        [
            Value::from_extern_ref(&mut test_value),
            Value::from_i32(1500),
        ],
    );
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        WasmEdgeError::Store(StoreError::NotFoundFuncRegistered {
            func_name: "func-add2".into(),
            mod_name: "extern".into(),
        })
    );

    // Invoke host function to terminate or fail execution
    let result = executor.run_func_registered(&mut store, "extern", "func-term", []);
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), 1234);
    let result = executor.run_func_registered(&mut store, "extern", "func-fail", []);
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::ExecutionFailed))
    );
}
