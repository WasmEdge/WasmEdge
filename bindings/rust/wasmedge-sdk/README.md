# Overview

The [wasmedge-sdk](https://crates.io/crates/wasmedge-sdk) crate defines a group of high-level Rust APIs, which are used to build up business applications.

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

To use or build the `wasmedge-sdk` crate, the `WasmEdge` library is required.

- If you choose to use [install.sh](https://github.com/WasmEdge/WasmEdge/blob/master/utils/install.sh) to install WasmEdge Runtime on your local system. Please use `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR` to specify the paths to the `include` and `lib` directories, respectively. For example, use the following commands to specify the paths after using `bash install.sh --path=$HOME/wasmedge-install` to install WasmEdge Runtime on Ubuntu 20.04:

    ```bash
    export WASMEDGE_INCLUDE_DIR=$HOME/wasmedge-install/include 
    export WASMEDGE_LIB_DIR=$HOME/wasmedge-install/lib
    ```

- If you choose to manually download WasmEdge Runtime binary from [WasmEdge Releases Page](https://github.com/WasmEdge/WasmEdge/releases), it is strongly recommended to place it in `$HOME/.wasmedge` directory. It looks like below on Ubuntu 20.04. `wasmedge-sdk` will search the directory automatically, you do not have to set any environment variables for it.

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

A quick-start example below is using `wasmedge-sdk` to run a WebAssembly module written with its WAT format (textual format):

  ```rust
  // If the version of rust used is less than v1.63, please uncomment the follow attribute.
  // #![feature(explicit_generic_args_with_impl_trait)]

  use wasmedge_sdk::{Executor, FuncTypeBuilder, ImportObjectBuilder, Module, Store};
  use wasmedge_sys::WasmValue;
  use wasmedge_types::wat2wasm;
  
  #[cfg_attr(test, test)]
  fn main() -> anyhow::Result<()> {
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
  
      // We define a function to act as our "env" "say_hello" function imported in the
      // Wasm program above.
      fn say_hello_world(_: CallingFrame, _: Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> {
          println!("Hello, world!");
  
          Ok(vec![])
      }
  
      // create an import module
      let import = ImportObjectBuilder::new()
          .with_func::<(), ()>("say_hello", Box::new(say_hello_world))?
          .build("env")?;
  
      // loads a wasm module from the given in-memory bytes
      let module = Module::from_bytes(None, &wasm_bytes)?;
  
      // create an executor
      let mut executor = Executor::new(None, None)?;
  
      // create a store
      let mut store = Store::new()?;
  
      // register the module into the store
      store.register_import_module(&mut executor, &import)?;
      let extern_instance = store.register_named_module(&mut executor, "extern", &module)?;
  
      // get the exported function "run"
      let run = extern_instance.func("run").ok_or(anyhow::Error::msg(
          "Not found exported function named 'run'.",
      ))?;
  
      // run host function
      run.call(&mut executor, [])?;
  
      Ok(())
  }

   ```

   [[Click for more examples]](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust/wasmedge-sdk/examples)

## See also

- [WasmEdge Runtime](https://wasmedge.org/)
- [WasmEdge C API Documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md)
