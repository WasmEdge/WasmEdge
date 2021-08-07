//! Usage: [DY]LD_LIBRARY_PATH="$(git rev-parse --show-toplevel)/build/lib/api" cargo run --example quickstart

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let module_path = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .unwrap()
        .join("tools/wasmedge/examples/fibonacci.wasm");

    let config = wedge::Config::default();
    let module = wedge::Module::load_from_file(&module_path, &config)?;
    let mut vm = wedge::Vm::create(&module, &config)?;

    let results = vm.run("fib", &[5.into()])?;
    assert_eq!(results.len(), 1);
    let result = results[0].as_i32().unwrap();

    assert_eq!(result, 8);
    println!("fib(5) = {}", result);

    Ok(())
}
