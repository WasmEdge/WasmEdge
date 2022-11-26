//! This example is to demonstrate computing multiple Fibonacci numbers concurrently.
//!
//! The main thread is responsible for computing `Fib(4), while a child thread is taking care of `Fib(5)`. Finally,
//! `Fib(6)` can be computed with the results of `Fib(5)` and `Fib(4)`.

use std::{
    sync::{Arc, Mutex},
    thread,
};
use wasmedge_sys::{Config, Store, Vm, WasmValue};
use wasmedge_types::wat2wasm;

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // create a Config context
    let mut config = Config::create()?;
    config.bulk_memory_operations(true);

    // create a Store context
    let mut store = Store::create()?;

    // create a Vm context with the given Config and Store
    let vm = Vm::create(Some(config), Some(&mut store))?;

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
    vm.register_wasm_from_bytes("extern", &wasm_bytes)?;

    let vm = Arc::new(Mutex::new(vm));

    // compute fib(4) by a child thread
    let vm_cloned = Arc::clone(&vm);
    let handle_a = thread::spawn(move || {
        let vm_child_thread = vm_cloned.lock().expect("fail to lock vm");
        let returns = vm_child_thread
            .run_registered_function("extern", "fib", [WasmValue::from_i32(4)])
            .expect("fail to compute fib(4)");

        let fib4 = returns[0].to_i32();
        println!("fib(4) by child thread: {fib4}");

        fib4
    });

    // compute fib(5) by a child thread
    let vm_cloned = Arc::clone(&vm);
    let handle_b = thread::spawn(move || {
        let vm_child_thread = vm_cloned.lock().expect("fail to lock vm");
        let returns = vm_child_thread
            .run_registered_function("extern", "fib", [WasmValue::from_i32(5)])
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
