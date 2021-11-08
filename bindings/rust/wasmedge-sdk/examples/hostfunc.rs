//! Usage: [DY]LD_LIBRARY_PATH="$(git rev-parse --show-toplevel)/build/lib/api" cargo run --example hostfunc
use std::os::raw::c_void;

fn real_add(a: i32, b: i32) -> i32 {
    println!("Rust: Entering Rust function real_add");
    let c = a + b;
    println!("Rust: Leaving Rust function real_add");
    c
}

fn real_mul(a: i32, b: i32) -> i32 {
    println!("Rust: Entering Rust function real_mul");
    let c = a * b;
    println!("Rust: Leaving Rust function real_mul");
    c
}

fn real_square(a: i32) -> i32 {
    println!("Rust: Entering Rust function real_square");
    let c = a * a;
    println!("Rust: Leaving Rust function real_square");
    c
}

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let module_path =
        std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("tools/wasmedge/examples/funcs.wasm");

    let config = wasmedge_sdk::Config::default();
    let module = wasmedge_sdk::Module::new(&config, &module_path)?;

    let vm = wasmedge_sdk::Vm::load(&module)?
        .with_config(&config)?
        .create()?;

    Ok(())
}
