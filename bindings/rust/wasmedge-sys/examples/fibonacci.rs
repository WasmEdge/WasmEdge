//! To run this example, use the following command:
//!
//! ```bash
//! cd /wasmedge-root-dir/bindings/rust/
//!
//! cargo run -p wasmedge-sys --example fibonacci -- --nocapture
//! ```
//!

use wasmedge_sys::{Executor, Loader, Store, Validator, WasmValue};
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

    // create an executor
    let mut executor = Executor::create(None, None)?;

    // create a store
    let mut store = Store::create()?;

    let module = Loader::create(None)?.from_bytes(wasm_bytes)?;
    Validator::create(None)?.validate(&module)?;

    let active_instance = executor.register_active_module(&mut store, &module)?;
    let fib = active_instance.get_func("fib")?;

    let returns = executor.call_func(&fib, vec![WasmValue::from_i32(5)])?;
    assert_eq!(returns.len(), 1);
    assert_eq!(returns[0].to_i32(), 8);

    Ok(())
}
