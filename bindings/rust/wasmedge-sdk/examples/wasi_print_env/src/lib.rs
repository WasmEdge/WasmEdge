//! The project is used to generate wasi_print_env.wasm, which will be used by the example wasi_print_env.rs
//!
//! # Usage
//!
//! run `cargo build --target=wasm32-wasi --release`
use std::env;

#[no_mangle]
pub fn print_env() {
    println!("The env vars are as follows.");
    for (key, value) in env::vars() {
        println!("{}: {}", key, value);
    }

    println!("The args are as follows.");
    for argument in env::args() {
        println!("{}", argument);
    }
}
