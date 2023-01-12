# Overview

The [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crate defines a group of low-level Rust APIs for WasmEdge, a light-weight, high-performance, and extensible WebAssembly runtime for cloud-native, edge, and decentralized applications.

For developers, it is recommended that the APIs in `wasmedge-sys` are used to construct high-level libraries, while `wasmedge-sdk` is for building up business applications.

Notice that [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) requires **Rust v1.63 or above** in the **stable** channel.

## Versioning Table

The following table provides the versioning information about each crate of WasmEdge Rust bindings.

| wasmedge-sdk  | WasmEdge lib  | wasmedge-sys  | wasmedge-types| wasmedge-macro|
| :-----------: | :-----------: | :-----------: | :-----------: | :-----------: |
| 0.7.1         | 0.11.2        | 0.12.2        | 0.3.1         | 0.3.0         |
| 0.7.0         | 0.11.2        | 0.12          | 0.3.1         | 0.3.0         |
| 0.6.0         | 0.11.2        | 0.11          | 0.3.0         | 0.2.0         |
| 0.5.0         | 0.11.1        | 0.10          | 0.3.0         | 0.1.0         |
| 0.4.0         | 0.11.0        | 0.9           | 0.2.1         | -             |
| 0.3.0         | 0.10.1        | 0.8           | 0.2           | -             |
| 0.1.0         | 0.10.0        | 0.7           | 0.1           | -             |

## Build

To use or build the `wasmedge-sys` crate, the `WasmEdge` library is required.

- If you choose to use [install.sh](https://github.com/WasmEdge/WasmEdge/blob/master/utils/install.sh) to install WasmEdge Runtime on your local system. Please use `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR` to specify the paths to the `include` and `lib` directories, respectively. For example, use the following commands to specify the paths after using `bash install.sh --path=$HOME/wasmedge-install` to install WasmEdge Runtime on Ubuntu 20.04:

    ```bash
    export WASMEDGE_INCLUDE_DIR=$HOME/wasmedge-install/include 
    export WASMEDGE_LIB_DIR=$HOME/wasmedge-install/lib
    ```

- If you choose to manually download WasmEdge Runtime binary from [WasmEdge Releases Page](https://github.com/WasmEdge/WasmEdge/releases), it is strongly recommended to place it in `$HOME/.wasmedge` directory. It looks like below on Ubuntu 20.04. `wasmedge-sys` will search the directory automatically, you do not have to set any environment variables for it.

   ```bash
   // $HOME/.wasmedge/
   .
   |-- bin
   |   |-- wasmedge
   |   `-- wasmedgec
   |-- include
   |   `-- wasmedge
   |       |-- enum.inc
   |       |-- enum_configure.h
   |       |-- enum_errcode.h
   |       |-- enum_types.h
   |       |-- int128.h
   |       |-- version.h
   |       `-- wasmedge.h
   `-- lib64
       |-- libwasmedge_c.so
       `-- wasmedge
           `-- libwasmedgePluginWasmEdgeProcess.so

   5 directories, 11 files
   ```

### Enable WasmEdge Plugins

If you'd like to enable WasmEdge Plugins (currently, only available on Linux platform), please use `WASMEDGE_PLUGIN_PATH` environment variable to specify the path to the directory containing the plugins. For example, use the following commands to specify the path on Ubuntu 20.04:

```bash
export WASMEDGE_PLUGIN_PATH=$HOME/.wasmedge/lib/wasmedge
```

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

- [WasmEdge Runtime Official Website](https://wasmedge.org/)
- [WasmEdge Docs](https://wasmedge.org/book/en/)
- [WasmEdge C API Documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md)
