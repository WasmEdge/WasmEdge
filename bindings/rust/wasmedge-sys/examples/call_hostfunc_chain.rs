//! This example demonstrates how to implement hostfunc call chaining, namely call
//! another hostfunc in the current hostfunc.
//!
//! To run this example, use the following command:
//!
//! ```bash
//! cd /wasmedge-root-dir/bindings/rust/
//!
//! cargo run -p wasmedge-sys --example call_hostfunc_chain -- --nocapture
//! ```
//!
//! The following info will be printed out in the terminal:
//!
//! ```bash
//! There is layer1!
//! There is layer2!
//! ```

use std::sync::{Arc, Mutex};
use wasmedge_sys::*;
use wasmedge_types::{error::HostFuncError, wat2wasm};

struct Wrapper(*const Vm);
unsafe impl Send for Wrapper {}

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let wasm_bytes = wat2wasm(
        br#"
    (module
        (import "host" "layer1" (func $host_layer1))
        (import "host" "layer2" (func $host_layer2))

        (func (export "layer1")
            call $host_layer1)
        (func (export "layer2")
            call $host_layer2)
    )
    "#,
    )?;

    let mut vm = Vm::create(None, None)?;
    vm.load_wasm_from_bytes(&wasm_bytes)?;
    vm.validate()?;

    let host_layer1 =
        |_frame: CallingFrame, _args: Vec<WasmValue>| -> Result<Vec<WasmValue>, HostFuncError> {
            println!("There is layer1!");
            Ok(vec![])
        };

    let s = Arc::new(Mutex::new(Wrapper(&vm as *const Vm)));
    let host_layer2 = move |_frame: CallingFrame,
                            _args: Vec<WasmValue>|
          -> Result<Vec<WasmValue>, HostFuncError> {
        unsafe {
            (*s.lock().unwrap().0).run_function("layer1", []).unwrap();
        }
        println!("There is layer2!");
        Ok(vec![])
    };

    let func_ty = FuncType::create(vec![], vec![]).unwrap();
    let host_layer1 = Function::create(&func_ty, Box::new(host_layer1), 0)?;
    let host_layer2 = Function::create(&func_ty, Box::new(host_layer2), 0)?;

    let mut import = ImportModule::create("host")?;
    import.add_func("layer1", host_layer1);
    import.add_func("layer2", host_layer2);

    vm.register_wasm_from_import(ImportObject::Import(import))?;
    vm.instantiate()?;

    vm.run_function("layer2", [])?;

    Ok(())
}
