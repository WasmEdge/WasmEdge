# Overview

The [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) crate defines a group of high-level Rust APIs, which are used to build up business applications.

* Notice that [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) requires **Rust v1.66 or above** in the **stable** channel.

## Build

To use or build the `wasmedge-sdk` crate, the `WasmEdge` library is required. Please refer to [WasmEdge Installation and Uninstallation](https://wasmedge.org/book/en/quick_start/install.html) to install the `WasmEdge` library.

* The following table provides the versioning information about each crate of WasmEdge Rust bindings.

  | wasmedge-sdk  | WasmEdge lib  | wasmedge-sys  | wasmedge-types| wasmedge-macro|
  | :-----------: | :-----------: | :-----------: | :-----------: | :-----------: |
  | 0.8.0         | 0.12.0        | 0.13.0        | 0.4.0         | 0.3.0         |
  | 0.7.1         | 0.11.2        | 0.12.2        | 0.3.1         | 0.3.0         |
  | 0.7.0         | 0.11.2        | 0.12          | 0.3.1         | 0.3.0         |
  | 0.6.0         | 0.11.2        | 0.11          | 0.3.0         | 0.2.0         |
  | 0.5.0         | 0.11.1        | 0.10          | 0.3.0         | 0.1.0         |
  | 0.4.0         | 0.11.0        | 0.9           | 0.2.1         | -             |
  | 0.3.0         | 0.10.1        | 0.8           | 0.2           | -             |
  | 0.1.0         | 0.10.0        | 0.7           | 0.1           | -             |
  
## Example

The example below is using `wasmedge-sdk` to run a WebAssembly module written with its WAT format (textual format). If you would like more examples, please refer to [Examples of WasmEdge RustSDK](https://github.com/second-state/wasmedge-rustsdk-examples).

```rust
use wasmedge_sdk::{
    error::HostFuncError, host_function, params, wat2wasm, Caller, ImportObjectBuilder, Module,
    VmBuilder, WasmValue,
};

// We define a function to act as our "env" "say_hello" function imported in the
// Wasm program above.
#[host_function]
pub fn say_hello(_caller: Caller, _args: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    println!("Hello, world!");

    Ok(vec![])
}

#[cfg_attr(test, test)]
fn main() -> anyhow::Result<()> {
    // create an import module
    let import = ImportObjectBuilder::new()
        .with_func::<(), ()>("say_hello", say_hello)?
        .build("env")?;

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

    // loads a wasm module from the given in-memory bytes
    let module = Module::from_bytes(None, wasm_bytes)?;

    // create an executor
    VmBuilder::new()
        .build()?
        .register_import_module(import)?
        .register_module(Some("extern"), module)?
        .run_func(Some("extern"), "run", params!())?;

    Ok(())
}

```
