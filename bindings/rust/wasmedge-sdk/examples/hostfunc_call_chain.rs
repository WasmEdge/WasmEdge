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

use wasmedge_sdk::{
    error::HostFuncError, params, wat2wasm, Caller, CallingFrame, ImportObjectBuilder, Module,
    VmBuilder, WasmValue,
};

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let vm = VmBuilder::new().build()?;

    let host_layer1 =
        |_frame: CallingFrame, _args: Vec<WasmValue>| -> Result<Vec<WasmValue>, HostFuncError> {
            println!("There is layer1!");
            Ok(vec![])
        };

    let host_layer2 = move |frame: CallingFrame,
                            _args: Vec<WasmValue>|
          -> Result<Vec<WasmValue>, HostFuncError> {
        let caller = Caller::new(frame);
        let executor = caller.executor().unwrap();
        let active_instance = caller.instance().unwrap();
        let fn_host_layer1 = active_instance
            .func("layer1")
            .expect("fail to find host function 'host_layer1'");
        fn_host_layer1.run(executor, params!()).unwrap();

        println!("There is layer2!");
        Ok(vec![])
    };

    // create an import object
    let import = ImportObjectBuilder::new()
        .with_func::<(), ()>("layer1", host_layer1)?
        .with_func::<(), ()>("layer2", host_layer2)?
        .build("host")?;

    // register the import object into vm
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

    // call the host function
    vm.run_func(None, "layer2", params!())?;

    Ok(())
}
