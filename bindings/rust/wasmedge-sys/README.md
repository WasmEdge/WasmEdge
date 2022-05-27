# Overview

The [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crate defines a group of low-level Rust APIs for WasmEdge, a light-weight, high-performance, and extensible WebAssembly runtime for cloud-native, edge, and decentralized applications.

For developers, it is recommended that the APIs in `wasmedge-sys` are used to construct high-level libraries, while `wasmedge-sdk` (coming soon) is for building up business applications.

## Usage

To use or build the `wasmedge-sys` crate, the `wasmedge-core` is required. The [*Build wasmedge-sys crate*](https://wasmedge.org/book/en/embed/rust.html#build-wasmedge-sys-crate) section of [WasmEdge Docs](https://wasmedge.org/book/en/) gives the tips.  

## Example

A quick-start example below is using `wasmedge-sys` to run a WebAssembly module written with its WAT format (textual format):

```rust
use wasmedge_sys::{Vm, WasmValue};
use wasmedge_types::wat2wasm;

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // read the wasm bytes
    let wasm_bytes = wat2wasm(
        br#"
        (module
            (export "fib" (func $fib))
            (func $fib (param $n i32) (result i32)
             (if
              (i32.lt_s
               (get_local $n)
               (i32.const 2)
              )
              (return
               (i32.const 1)
              )
             )
             (return
              (i32.add
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 2)
                )
               )
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 1)
                )
               )
              )
             )
            )
           )
"#,
    )?;

    // create a Vm instance
    let mut vm = Vm::create(None, None)?;

    // register the wasm bytes
    let module_name = "extern-module";
    vm.register_wasm_from_buffer(module_name, &wasm_bytes)?;

    // run the exported function named "fib"
    let func_name = "fib";
    let result = vm.run_registered_function(module_name, func_name, [WasmValue::from_i32(5)])?;

    assert_eq!(result.len(), 1);
    assert_eq!(result[0].to_i32(), 8);

    Ok(())
}
```

## See also

* [WasmEdge Runtime Official Website](https://wasmedge.org/)
* [WasmEdge Docs](https://wasmedge.org/book/en/)
* [WasmEdge C API Documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md)
