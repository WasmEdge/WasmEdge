//! This example is to demonstrate computing multiple Fibonacci numbers concurrently.
//!
//! The main thread is responsible for computing `Fib(4), while a child thread is taking care of `Fib(5)`. Finally,
//! `Fib(6)` can be computed with the results of `Fib(5)` and `Fib(4)`.
//!
//! To run this example, follow the commands below:
//!
//! ```bash
//! // go into the directory: bindings/rust
//! cargo run -p wasmedge-sys --example threads -- --nocapture
//! ```

use std::{
    sync::{Arc, Mutex},
    thread,
};
use wasmedge_sys::{Config, Executor, Loader, Store, Validator, WasmValue};
use wasmedge_types::wat2wasm;

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // create a Config context
    let mut config = Config::create()?;
    config.bulk_memory_operations(true);

    // create an executor with the given config
    let mut executor = Executor::create(Some(&config), None)?;

    // create a store
    let mut store = Store::create()?;

    // register a wasm module from a wasm file
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
    let extern_module = Loader::create(Some(&config))?.from_bytes(wasm_bytes)?;
    Validator::create(Some(&config))?.validate(&extern_module)?;
    executor.register_named_module(&mut store, &extern_module, "extern")?;

    let exec = Arc::new(Mutex::new(executor));
    let stor = Arc::new(Mutex::new(store));

    // compute fib(4) by a child thread
    let exec_cloned = Arc::clone(&exec);
    let stor_cloned = Arc::clone(&stor);
    let handle_a = thread::spawn(move || {
        let exec_child_thread = exec_cloned.lock().expect("fail to lock executor");
        let stor_child_thread = stor_cloned.lock().expect("fail to lock store");

        let fib = stor_child_thread
            .module("extern")
            .expect("fail to get instance named 'extern'")
            .get_func("fib")
            .expect("fail to get host func 'fib'");
        let returns = exec_child_thread
            .call_func(&fib, [WasmValue::from_i32(4)])
            .expect("fail to compute fib(4)");

        let fib4 = returns[0].to_i32();
        println!("fib(4) by child thread: {fib4}");

        fib4
    });

    // compute fib(5) by a child thread
    let exec_cloned = Arc::clone(&exec);
    let stor_cloned = Arc::clone(&stor);
    let handle_b = thread::spawn(move || {
        let exec_child_thread = exec_cloned.lock().expect("fail to lock executor");
        let stor_child_thread = stor_cloned.lock().expect("fail to lock store");

        let fib = stor_child_thread
            .module("extern")
            .expect("fail to get instance named 'extern'")
            .get_func("fib")
            .expect("fail to get host func 'fib'");
        let returns = exec_child_thread
            .call_func(&fib, [WasmValue::from_i32(5)])
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
