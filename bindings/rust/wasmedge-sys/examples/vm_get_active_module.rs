//!
//! To run this example, follow the commands below:
//!
//! ```bash
//! // go into the directory: bindings/rust
//! cargo run -p wasmedge-sys --example vm_get_active_module -- --nocapture
//! ```

use wasmedge_sys::Vm;
use wasmedge_types::wat2wasm;

#[cfg_attr(test, test)]
#[allow(clippy::assertions_on_result_states)]
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

    // create a VM context
    let mut vm = Vm::create(None)?;

    // register a wasm module into the vm as an anonymous module
    vm.register_active_instance_from_bytes(&wasm_bytes)?;

    // get the active module instance
    let active_instance = vm.active_instance().unwrap();

    assert!(active_instance.get_func("fib").is_ok());

    Ok(())
}
