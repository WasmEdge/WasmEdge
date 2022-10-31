use wasmedge_sdk::{
    config::{CommonConfigOptions, ConfigBuilder},
    params, Executor, Module, Store, WasmVal,
};
use wasmedge_types::wat2wasm;

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
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

    // load and validate module from in-memory wasm bytes
    let config = ConfigBuilder::new(CommonConfigOptions::default()).build()?;
    let module = Module::from_bytes(Some(&config), wasm_bytes)?;

    // create an Executor context
    let mut executor = Executor::new(None, None)?;

    // create a Store context
    let mut store = Store::new()?;

    // register a wasm module as an active module into store.
    let active_instance = store.register_active_module(&mut executor, &module)?;

    // get the wasm function "fib"
    let fib = active_instance
        .func("fib")
        .expect("Not found the wasm function named 'fib'.");

    // call the wasm function
    let returns = fib.call(&mut executor, params!(5))?;
    assert_eq!(returns.len(), 1);
    assert_eq!(returns[0].to_i32(), 8);

    Ok(())
}
