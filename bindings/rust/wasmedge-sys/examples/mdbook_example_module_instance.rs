#[cfg(all(not(feature = "static"), target_os = "linux"))]
use wasmedge_sys::{Config, Executor, Loader, Store, Validator};
#[cfg(all(not(feature = "static"), target_os = "linux"))]
use wasmedge_types::wat2wasm;

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(all(not(feature = "static"), target_os = "linux"))]
    executor_apis()?;

    Ok(())
}

#[cfg(all(not(feature = "static"), target_os = "linux"))]
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
        let loader = Loader::create(Some(&config))?;
        let module = loader.from_bytes(wasm_bytes)?;

        // validate module
        let config = Config::create()?;
        let validator = Validator::create(Some(&config))?;
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
        let loader = Loader::create(Some(&config))?;
        let module = loader.from_bytes(wasm_bytes)?;

        // validate module
        let config = Config::create()?;
        let validator = Validator::create(Some(&config))?;
        validator.validate(&module)?;

        // register a wasm module as an active module
        let active_instance = executor.register_active_module(&mut store, &module)?;
        assert!(active_instance.get_func("fib").is_ok());
    }
    Ok(())
}
