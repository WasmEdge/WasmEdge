//! This example presents how to register a module as named module in a Vm instance and run a target wasm function.

// If the version of rust used is less than v1.63, please uncomment the follow attribute.
// #![feature(explicit_generic_args_with_impl_trait)]

use wasmedge_sdk::{params, VmBuilder, WasmVal};
use wasmedge_types::wat2wasm;

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // create a Vm context
    let vm = VmBuilder::new().build()?;

    // register a wasm module from the given in-memory wasm bytes
    let wasm_bytes = wat2wasm(
        br#"(module
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
    let vm = vm.register_module_from_bytes("extern", wasm_bytes)?;

    // run `fib` function in the named module instance
    let returns = vm.run_func(Some("extern"), "fib", params!(10))?;
    assert_eq!(returns.len(), 1);
    assert_eq!(returns[0].to_i32(), 89);

    Ok(())
}
