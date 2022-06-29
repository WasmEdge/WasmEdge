use std::path::Path;
use wasmedge_sys::{Config, Vm};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // enable the `wasi` option
    let mut config = Config::create()?;
    config.wasi(true);

    // create a vm
    let mut vm = Vm::create(Some(config), None)?;

    // set the envs and args for the wasi module
    let args = vec!["arg1", "arg2"];
    let envs = vec!["ENV1=VAL1", "ENV1=VAL2", "ENV3=VAL3"];
    let mut wasi_module = vm.wasi_module_mut()?;
    wasi_module.init_wasi(Some(args), Some(envs), None);

    assert_eq!(wasi_module.exit_code(), 0);

    // load wasm module
    vm.load_wasm_from_file(Path::new("/Volumes/Dev/secondstate/me/WasmEdge/bindings/rust/wasmedge-sys/examples/wasi_print_env.rs"))?;

    vm.validate()?;

    vm.instantiate()?;

    vm.run_function("print_env", vec![])?;

    Ok(())
}
