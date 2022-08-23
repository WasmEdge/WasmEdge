// If the version of rust used is less than v1.63, please uncomment the follow attribute.
// #![feature(explicit_generic_args_with_impl_trait)]

use wasmedge_sdk::{params, Executor, ImportObjectBuilder, Module, Store};
use wasmedge_sys::WasmValue;
use wasmedge_types::wat2wasm;

#[cfg_attr(test, test)]
fn main() -> anyhow::Result<()> {
    // We define a function to act as our "env" "say_hello" function imported in the
    // Wasm program above.
    fn say_hello_world(_inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, u32> {
        println!("Hello, world!");

        Ok(vec![])
    }

    // create an import module
    let import = ImportObjectBuilder::new()
        .with_func::<(), ()>("say_hello", say_hello_world)?
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
    let module = Module::from_bytes(None, &wasm_bytes)?;

    // create an executor
    let mut executor = Executor::new(None, None)?;

    // create a store
    let mut store = Store::new()?;

    // register the module into the store
    store.register_import_module(&mut executor, &import)?;

    // register the compiled module into the store and get an module instance
    let extern_instance = store.register_named_module(&mut executor, "extern", &module)?;

    // get the exported function "run"
    let run = extern_instance
        .func("run")
        .ok_or_else(|| anyhow::Error::msg("Not found exported function named 'run'."))?;

    // run host function
    run.call(&mut executor, params!())?;

    Ok(())
}
