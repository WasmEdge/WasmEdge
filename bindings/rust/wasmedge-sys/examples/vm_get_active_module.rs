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
    let vm = Vm::create(None, None)?;

    // load a wasm module from a in-memory bytes, and the loaded wasm module works as an anoymous
    // module (aka. active module in WasmEdge terminology)
    vm.load_wasm_from_bytes(&wasm_bytes)?;

    // validate the loaded active module
    vm.validate()?;

    // instatiate the loaded active module
    vm.instantiate()?;

    // get the active module instance
    let active_instance = vm.active_module()?;

    assert!(active_instance.get_func("fib").is_ok());

    Ok(())
}
