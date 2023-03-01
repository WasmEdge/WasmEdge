use wasmedge_sdk::{
    error::HostFuncError, host_function, params, wat2wasm, Caller, ImportObjectBuilder, Module,
    VmBuilder, WasmValue,
};

// We define a function to act as our "env" "say_hello" function imported in the
// Wasm program above.
#[host_function]
pub fn say_hello(_caller: Caller, _args: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    println!("Hello, world!");

    Ok(vec![])
}

#[cfg_attr(test, test)]
fn main() -> anyhow::Result<()> {
    // create an import module
    let import = ImportObjectBuilder::new()
        .with_func::<(), ()>("say_hello", say_hello)?
        .build("env")?;

    let wasm_bytes = wat2wasm(
        br#"
    (module
      ;; First we define a type with no parameters and no results.
      (type $no_args_no_rets_t (func (param) (result)))
    
      ;; Then we declare that we want to import a function named "env" "say_hello" with
      ;; that type signature.
      (import "env" "say_hello" (func $say_hello (type $no_args_no_rets_t)))
    
      ;; Finally we create an entrypoint that calls our imported function.
      (func $run (type $no_args_no_rets_t)
        (call $say_hello))
      ;; And mark it as an exported function named "run".
      (export "run" (func $run)))
    "#,
    )?;

    // loads a wasm module from the given in-memory bytes
    let module = Module::from_bytes(None, wasm_bytes)?;

    // create an executor
    VmBuilder::new()
        .build()?
        .register_import_module(import)?
        .register_module(Some("extern"), module)?
        .run_func(Some("extern"), "run", params!())?;

    Ok(())
}
