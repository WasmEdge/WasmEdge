//! Usage: [DY]LD_LIBRARY_PATH="$(git rev-parse --show-toplevel)/build/lib/api" cargo run --example hello 1 2 3

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut args: Vec<String> = std::env::args().collect();
    let args: Vec<&str> = args.iter_mut().map(|arg| arg.as_str()).collect();
    if args.len() <= 1 {
        println!("Rust: No input args.");
    }
    let module_path =
        std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("tools/wasmedge/examples/hello.wasm");

    let config = wasmedge_sdk::Config::create().expect("fail to create Config instance");
    let mut module = wasmedge_sdk::Module::new(&config, &module_path)?;

    let vm = wasmedge_sdk::Vm::load(&mut module)?
        .with_config(&config)?
        .create()?;

    vm.init_wasi_obj().with_args(args).build();
    // or
    // vm.init_wasi_obj().with_args(args).with_envs(envs).build();

    let results = vm.run("_start", &[])?;

    assert_eq!(results.len(), 0);

    for r in results.iter() {
        let res = r.as_i32().unwrap();
        println!("hello : {}", res);
    }

    Ok(())
}
