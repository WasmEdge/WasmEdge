//! Usage: [DY]LD_LIBRARY_PATH="$(git rev-parse --show-toplevel)/build/lib/api" cargo run --example quickstart

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // NOTICE: The following code is commented out temporarily because the wasmedge-sys crate is in the process of refactoring, which would bring some changes to the current wasmedge-sdk crate.

    // let module_path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
    //     .join("tools/wasmedge/examples/fibonacci.wasm");

    // let config = wasmedge_sdk::Config::create().expect("fail to create Config instance");
    // let mut module = wasmedge_sdk::Module::new(&config, &module_path)?;

    // let vm = wasmedge_sdk::Vm::load(&mut module)?
    //     .with_config(&config)?
    //     .create()?;

    // let results = vm.run("fib", &[5.into()])?;

    // assert_eq!(results.len(), 1);
    // let result = results[0].as_i32().unwrap();

    // assert_eq!(result, 8);
    // println!("fib(5) = {}", result);

    Ok(())
}
