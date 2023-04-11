# Introduction to WasmEdge module instance

> The code in the following examples are verified on
>
> * wasmedge-sys v0.10.0
> * wasmedge-types v0.3.0

## Example 1

In this example, we'll demonstrate how to use the APIs of `Vm` to

* Create Wasi and WasmEdgeProcess module instances implicitly by using a `Config` while creating a `Vm`.

    ```rust
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

    ```

* Retrieve the Wasi and WasmEdgeProcess module instances from the `Vm`.

    ```rust
    // get the default Wasi module
    let wasi_instance = vm.wasi_module_mut()?;
    assert_eq!(wasi_instance.name(), "wasi_snapshot_preview1");
    // get the default WasmEdgeProcess module instance
    let wasmedge_process_instance = vm.wasmedge_process_module_mut()?;
    assert_eq!(wasmedge_process_instance.name(), "wasmedge_process");

    ```

* Register an import module as a named module into the `Vm`.

    ```rust
    // create ImportModule instance
    let module_name = "extern_module";
    let mut import = ImportModule::create(module_name)?;

    // a function to import
    #[sys_host_function]
    fn real_add(_frame: &CallingFrame, inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> {
        if inputs.len() != 2 {
            return Err(1);
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(2);
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(3);
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }

    // add host function
    let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32])?;
    let host_func = Function::create(&func_ty, Box::new(real_add), 0)?;
    import.add_func("add", host_func);

    // add table
    let table_ty = TableType::create(RefType::FuncRef, 0..=u32::MAX)?;
    let table = Table::create(&table_ty)?;
    import.add_table("table", table);

    // add memory
    let mem_ty = MemType::create(0..=u32::MAX)?;
    let memory = Memory::create(&mem_ty)?;
    import.add_memory("mem", memory);

    // add global
    let ty = GlobalType::create(ValType::F32, Mutability::Const)?;
    let global = Global::create(&ty, WasmValue::from_f32(3.5))?;
    import.add_global("global", global);

    // register the import module as a named module
    vm.register_wasm_from_import(ImportObject::Import(import))?;

    ```

* Retrieve the internal `Store` instance from the `Vm`, and retrieve the named module instance from the `Store` instance.

    ```rust
    let mut store = vm.store_mut()?;
    let named_instance = store.module(module_name)?;
    assert!(named_instance.get_func("add").is_ok());
    assert!(named_instance.get_table("table").is_ok());
    assert!(named_instance.get_memory("mem").is_ok());
    assert!(named_instance.get_global("global").is_ok());
    
    ```

* Register an active module into the `Vm`.

    ```rust
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

    // load a wasm module from a in-memory bytes, and the loaded wasm module works as an anonymous
    // module (aka. active module in WasmEdge terminology)
    vm.load_wasm_from_bytes(&wasm_bytes)?;

    // validate the loaded active module
    vm.validate()?;

    // instantiate the loaded active module
    vm.instantiate()?;

    // get the active module instance
    let active_instance = vm.active_module()?;
    assert!(active_instance.get_func("fib").is_ok());
        
    ```

* Retrieve the active module from the `Vm`.

    ```rust
    // get the active module instance
    let active_instance = vm.active_module()?;
    assert!(active_instance.get_func("fib").is_ok());
    
    ```

The complete code in this demo can be found on [WasmEdge Github](https://github.com/WasmEdge/WasmEdge/blob/master/bindings/rust/wasmedge-sys/examples/mdbook_example_module_instance.rs).

## Example 2

In this example, we'll demonstrate how to use the APIs of `Executor` to

* Create an `Executor` and a `Store`.

    ```rust
    // create an Executor context
    let mut executor = Executor::create(None, None)?;

    // create a Store context
    let mut store = Store::create()?;

    ```

* Register an import module into the `Executor`.

    ```rust
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
    let module = loader.from_bytes(&wasm_bytes)?;

    // validate module
    let config = Config::create()?;
    let validator = Validator::create(Some(config))?;
    validator.validate(&module)?;

    // register a wasm module into the store context
    let module_name = "extern";
    let named_instance = executor.register_named_module(&mut store, &module, module_name)?;
    assert!(named_instance.get_func("fib").is_ok());

    ```

* Register an active module into the `Executor`.

    ```rust
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
    let module = loader.from_bytes(&wasm_bytes)?;

    // validate module
    let config = Config::create()?;
    let validator = Validator::create(Some(config))?;
    validator.validate(&module)?;

    // register a wasm module as an active module
    let active_instance = executor.register_active_module(&mut store, &module)?;
    assert!(active_instance.get_func("fib").is_ok());

    ```

The complete code in this demo can be found in [mdbook_example_module_instance.rs](https://github.com/WasmEdge/WasmEdge/blob/master/bindings/rust/wasmedge-sys/examples/mdbook_example_module_instance.rs).
