//! This example presents the support for multi-value proposal.
//!
//! The wasm function "swap" takes two inputs, say (a, b), and returns (b, a).
//!

use wasmedge_sdk::{params, Executor, Module, Store, WasmVal};
use wasmedge_types::wat2wasm;

#[cfg_attr(test, test)]
fn main() -> anyhow::Result<()> {
    let wasm_bytes = wat2wasm(
        br#"
(module
  (func $swap (param i32 i32) (result i32 i32)
	(local.get 1) (local.get 0))
  
  (export "swap" (func $swap)))
"#,
    )?;
    let module = Module::from_bytes(None, wasm_bytes)?;

    let mut executor = Executor::new(None, None)?;
    let mut store = Store::new()?;
    let active_instance = store.register_active_module(&mut executor, &module)?;

    let swap = active_instance
        .func("swap")
        .expect("Not found a host function named 'swap'.");

    let returns = swap.call(&mut executor, params!(2, 3))?;
    assert_eq!(returns.len(), 2);

    println!(
        "the result of swap: {}, {}",
        returns[0].to_i32(),
        returns[1].to_i32()
    );

    Ok(())
}
