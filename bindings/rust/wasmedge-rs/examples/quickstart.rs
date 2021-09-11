//! Usage: [DY]LD_LIBRARY_PATH="$(git rev-parse --show-toplevel)/build/lib/api" cargo run --example quickstart

use std::path::PathBuf;

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let module_path = std::path::PathBuf::from(env!("WASMEDGE_SRC_DIR"))
        .join("tools/wasmedge/examples/fibonacci.wasm");

    let config = wedge::Config::default();
    let vm = wedge::Vm::new(module_path)?.with_config(config)?.build()?;

    let results = vm.run("fib", &[5.into()])?;

    assert_eq!(results.len(), 1);
    let result = results[0].as_i32().unwrap();

    assert_eq!(result, 8);
    println!("fib(5) = {}", result);

    Ok(())
}
