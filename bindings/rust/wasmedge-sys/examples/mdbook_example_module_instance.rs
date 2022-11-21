#[cfg(target_os = "linux")]
use wasmedge_macro::sys_host_function;
#[cfg(target_os = "linux")]
use wasmedge_sys::{
    utils, AsImport, CallingFrame, Config, Executor, FuncType, Function, Global, GlobalType,
    ImportModule, ImportObject, Loader, MemType, Memory, Store, Table, TableType, Validator, Vm,
    WasmValue,
};
#[cfg(target_os = "linux")]
use wasmedge_types::{error::HostFuncError, wat2wasm, Mutability, RefType, ValType};

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(target_os = "linux")]
    vm_apis()?;

    #[cfg(target_os = "linux")]
    executor_apis()?;

    Ok(())
}

#[cfg(target_os = "linux")]
#[allow(clippy::assertions_on_result_states)]
fn vm_apis() -> Result<(), Box<dyn std::error::Error>> {
    // load wasmedge_process plugins
    utils::load_plugin_from_default_paths();

    // create a Config context
    let mut config = Config::create()?;
    config.bulk_memory_operations(true);
    assert!(config.bulk_memory_operations_enabled());
    config.wasi(true);
    assert!(config.wasi_enabled());
    config.wasmedge_process(true);
    assert!(config.wasmedge_process_enabled());

    // create a Vm context with the given Config and Store
    let mut vm = Vm::create(Some(config), None)?;

    // Retrieve the Wasi and WasmEdgeProcess module instances from the Vm.
    {
        // get the default Wasi module
        let wasi_instance = vm.wasi_module_mut()?;
        assert_eq!(wasi_instance.name(), "wasi_snapshot_preview1");
        // get the default WasmEdgeProcess module instance
        let wasmedge_process_instance = vm.wasmedge_process_module_mut()?;
        assert_eq!(wasmedge_process_instance.name(), "wasmedge_process");
    }

    // Register an import module as a named module into the Vm.
    {
        let module_name = "extern_module";

        // create ImportModule instance
        let mut import = ImportModule::create(module_name)?;

        // a function to import
        #[sys_host_function]
        fn real_add(
            _frame: CallingFrame,
            inputs: Vec<WasmValue>,
        ) -> Result<Vec<WasmValue>, HostFuncError> {
            if inputs.len() != 2 {
                return Err(HostFuncError::User(1));
            }

            let a = if inputs[0].ty() == ValType::I32 {
                inputs[0].to_i32()
            } else {
                return Err(HostFuncError::User(2));
            };

            let b = if inputs[1].ty() == ValType::I32 {
                inputs[1].to_i32()
            } else {
                return Err(HostFuncError::User(3));
            };

            let c = a + b;

            Ok(vec![WasmValue::from_i32(c)])
        }

        // add host function
        let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32])?;
        let host_func = Function::create(&func_ty, Box::new(real_add), 0)?;
        import.add_func("add", host_func);

        // add table
        let table_ty = TableType::create(RefType::FuncRef, 0, Some(u32::MAX))?;
        let table = Table::create(&table_ty)?;
        import.add_table("table", table);

        // add memory
        let mem_ty = MemType::create(0, Some(u32::MAX), false)?;
        let memory = Memory::create(&mem_ty)?;
        import.add_memory("mem", memory);

        // add global
        let ty = GlobalType::create(ValType::F32, Mutability::Const)?;
        let global = Global::create(&ty, WasmValue::from_f32(3.5))?;
        import.add_global("global", global);

        // register the import module as a named module
        vm.register_wasm_from_import(ImportObject::Import(import))?;

        // Retrieve the internal Store instance from the Vm, and retrieve the named module instance from the Store instance.
        let mut store = vm.store_mut()?;
        let named_instance = store.module(module_name)?;
        assert!(named_instance.get_func("add").is_ok());
        assert!(named_instance.get_table("table").is_ok());
        assert!(named_instance.get_memory("mem").is_ok());
        assert!(named_instance.get_global("global").is_ok());
    }

    // Register an active module into the Vm.
    {
        // read the wasm bytes
        let wasm_bytes = wat2wasm(
            br#"
            (module
                (export "fib" (func $fib))
                (func $fib (param $n i32) (result i32)
                 (if
                  (i32.lt_s
                   (get_local $n)
                   (i32.const 2)
                  )
                  (return
                   (i32.const 1)
                  )
                 )
                 (return
                  (i32.add
                   (call $fib
                    (i32.sub
                     (get_local $n)
                     (i32.const 2)
                    )
                   )
                   (call $fib
                    (i32.sub
                     (get_local $n)
                     (i32.const 1)
                    )
                   )
                  )
                 )
                )
               )
    "#,
        )?;

        // load a wasm module from a in-memory bytes, and the loaded wasm module works as an anoymous
        // module (aka. active module in WasmEdge terminology)
        vm.load_wasm_from_bytes(&wasm_bytes)?;

        // validate the loaded active module
        vm.validate()?;

        // instatiate the loaded active module
        vm.instantiate()?;

        // get the active module instance
        let active_instance = vm.active_module()?;
        assert!(active_instance.get_func("fib").is_ok());
    }

    Ok(())
}

#[cfg(target_os = "linux")]
#[allow(clippy::assertions_on_result_states)]
fn executor_apis() -> Result<(), Box<dyn std::error::Error>> {
    // create an Executor context
    let mut executor = Executor::create(None, None)?;

    // create a Store context
    let mut store = Store::create()?;

    // Register an import module into the Executor
    {
        // read the wasm bytes
        let wasm_bytes = wat2wasm(
            br#"
        (module
            (export "fib" (func $fib))
            (func $fib (param $n i32) (result i32)
             (if
              (i32.lt_s
               (get_local $n)
               (i32.const 2)
              )
              (return
               (i32.const 1)
              )
             )
             (return
              (i32.add
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 2)
                )
               )
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 1)
                )
               )
              )
             )
            )
           )
"#,
        )?;

        // load module from a wasm file
        let config = Config::create()?;
        let loader = Loader::create(Some(config))?;
        let module = loader.from_bytes(wasm_bytes)?;

        // validate module
        let config = Config::create()?;
        let validator = Validator::create(Some(config))?;
        validator.validate(&module)?;

        // register a wasm module into the store context
        let module_name = "extern";
        let named_instance = executor.register_named_module(&mut store, &module, module_name)?;
        assert!(named_instance.get_func("fib").is_ok());
    }

    // Register an active module into the Executor
    {
        // read the wasm bytes
        let wasm_bytes = wat2wasm(
            br#"
        (module
            (export "fib" (func $fib))
            (func $fib (param $n i32) (result i32)
             (if
              (i32.lt_s
               (get_local $n)
               (i32.const 2)
              )
              (return
               (i32.const 1)
              )
             )
             (return
              (i32.add
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 2)
                )
               )
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 1)
                )
               )
              )
             )
            )
           )
"#,
        )?;

        // load module from a wasm file
        let config = Config::create()?;
        let loader = Loader::create(Some(config))?;
        let module = loader.from_bytes(wasm_bytes)?;

        // validate module
        let config = Config::create()?;
        let validator = Validator::create(Some(config))?;
        validator.validate(&module)?;

        // register a wasm module as an active module
        let active_instance = executor.register_active_module(&mut store, &module)?;
        assert!(active_instance.get_func("fib").is_ok());
    }
    Ok(())
}
