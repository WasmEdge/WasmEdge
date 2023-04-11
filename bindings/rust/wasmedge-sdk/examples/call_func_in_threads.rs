//! This example is to demonstrate computing multiple Fibonacci numbers concurrently.
//!
//! The main thread is responsible for computing `Fib(4), while a child thread is taking care of `Fib(5)`. Finally,
//! `Fib(6)` can be computed with the results of `Fib(5)` and `Fib(4)`.

use std::{
    sync::{Arc, Mutex},
    thread,
};
use wasmedge_sdk::{
    config::{CommonConfigOptions, ConfigBuilder},
    params, Executor, Module, Statistics, Store, WasmVal,
};
use wasmedge_types::wat2wasm;

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // create an executor
    let config = ConfigBuilder::new(CommonConfigOptions::default()).build()?;
    let mut stat = Statistics::new()?;
    let mut executor = Executor::new(Some(&config), Some(&mut stat))?;

    // create a store
    let mut store = Store::new()?;

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
    let module = Module::from_bytes(Some(&config), wasm_bytes)?;

    // register the wasm module into store
    let extern_instance = store.register_named_module(&mut executor, "extern", &module)?;

    let executor = Arc::new(Mutex::new(executor));
    let instance = Arc::new(Mutex::new(extern_instance));

    // compute fib(4) by a child thread
    let executor_cloned = Arc::clone(&executor);
    let instance_cloned = Arc::clone(&instance);
    let handle_a = thread::spawn(move || {
        let executor = executor_cloned.lock().unwrap();
        let instance = instance_cloned.lock().unwrap();

        // get the exported wasm function "fib"
        let fib = instance
            .func("fib")
            .expect("failed to get the exported function 'fib'");

        // compute fib(4)
        let returns = executor
            .run_func(&fib, params!(4))
            .expect("fail to compute fib(4)");

        let fib4 = returns[0].to_i32();
        println!("fib(4) by child thread: {fib4}");

        fib4
    });

    // compute fib(5) by a child thread
    let executor_cloned = Arc::clone(&executor);
    let instance_cloned = Arc::clone(&instance);
    let handle_b = thread::spawn(move || {
        let executor = executor_cloned.lock().unwrap();
        let instance = instance_cloned.lock().unwrap();

        // get the exported wasm function "fib"
        let fib = instance
            .func("fib")
            .expect("failed to get the exported function 'fib'");

        // compute fib(5)
        let returns = executor
            .run_func(&fib, params!(5))
            .expect("fail to compute fib(5)");

        let fib5 = returns[0].to_i32();
        println!("fib(5) by child thread: {fib5}");

        fib5
    });

    let fib4 = handle_a.join().unwrap();
    let fib5 = handle_b.join().unwrap();

    // compute fib(6)
    println!("fib(6) = fib(5) + fib(1) = {}", fib5 + fib4);

    Ok(())
}
