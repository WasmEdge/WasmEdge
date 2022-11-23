//! This example demonstrates how to implement hostfunc call chaining, namely call
//! another hostfunc in the current hostfunc.
//!
//! To run this example, use the following command:
//!
//! ```bash
//! cd /wasmedge-root-dir/bindings/rust/
//!
//! cargo run -p wasmedge-sdk --example hostfunc_call_chain -- --nocapture
//! ```
//!
//! The following info will be printed out in the terminal:
//!
//! ```bash
//! There is layer1!
//! There is layer2!
//! ```

use std::sync::{Arc, Mutex};
use wasmedge_sdk::{
    error::HostFuncError, params, wat2wasm, CallingFrame, ImportObjectBuilder, Module, Vm,
    WasmValue,
};

struct Wrapper(*const Vm);
unsafe impl Send for Wrapper {}

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let vm = Vm::new(None)?;

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
            (*s.lock().unwrap().0)
                .run_func(None, "layer1", params!())
                .unwrap();
        }
        println!("There is layer2!");
        Ok(vec![])
    };

    let import = ImportObjectBuilder::new()
        .with_func::<(), ()>("layer1", host_layer1)?
        .with_func::<(), ()>("layer2", host_layer2)?
        .build("host")?;

    let vm = vm.register_import_module(import)?;

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

    let module = Module::from_bytes(None, wasm_bytes)?;

    // register the wasm module into vm
    let vm = vm.register_module(None, module)?;

    vm.run_func(None, "layer2", params!())?;

    Ok(())
}
