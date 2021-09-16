//! Usage: [DY]LD_LIBRARY_PATH="$(git rev-parse --show-toplevel)/build/lib/api" cargo run --example hello

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let module_path = std::path::PathBuf::from(env!("WASMEDGE_SRC_DIR"))
        .join("tools/wasmedge/examples/hello.wasm");

    let config = wedge::Config::with_wasi();
    let module = wedge::Module::new(&config, &module_path)?;

    let vm = wedge::Vm::load(&module)?.with_config(&config)?.create()?;

    let results = vm.run("hello", &[1.into(),2.into(),3.into(),4.into(),5.into()])?;

    assert_eq!(results.len(), 5);

    for r in results.iter() {
        let res = r.as_i32().unwrap();
        println!("hello : {}", res);
    }

    Ok(())
}
