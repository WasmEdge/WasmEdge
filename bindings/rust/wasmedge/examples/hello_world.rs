use wasmedge::{wat2wasm, Executor, FuncTypeBuilder, ImportModuleBuilder, Module, Store};
use wasmedge_sys::WasmValue;

fn main() -> anyhow::Result<()> {
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

    // We define a function to act as our "env" "say_hello" function imported in the
    // Wasm program above.
    fn say_hello_world(_inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> {
        println!("Hello, world!");

        Ok(vec![])
    }

    // create an import module
    let import = ImportModuleBuilder::new()
        .with_func(
            "say_hello",
            FuncTypeBuilder::default().build(),
            Box::new(say_hello_world),
        )?
        .build("env")?;

    // loads a wasm module from the given in-memory bytes
    let module = Module::from_bytes(None, &wasm_bytes)?;

    // create an executor
    let mut executor = Executor::new(None, None)?;

    // create a store
    let mut store = Store::new()?;

    // register the module into the store
    store.register_import_module(&mut executor, &import)?;
    let extern_instance = store.register_named_module(&mut executor, "extern", &module)?;

    // get the exported function "run"
    let run = extern_instance.func("run").ok_or(anyhow::Error::msg(
        "Not found exported function named 'run'.",
    ))?;

    // run host function
    executor.run_func(&run, [])?;

    Ok(())
}
