use wasmedge_sys::{Vm, WasmValue};
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

    // create a Vm instance
    let vm = Vm::create(None, None)?;

    // register the wasm bytes
    let module_name = "extern-module";
    vm.register_wasm_from_bytes(module_name, &wasm_bytes)?;

    // run the exported function named "fib"
    let func_name = "fib";
    let result = vm.run_registered_function(module_name, func_name, [WasmValue::from_i32(5)])?;

    assert_eq!(result.len(), 1);
    assert_eq!(result[0].to_i32(), 8);

    Ok(())
}
