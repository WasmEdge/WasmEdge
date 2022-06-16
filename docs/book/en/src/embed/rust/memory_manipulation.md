# Memory Manipulation

In this example, we'll present how to manipulate the linear memory with the APIs defined in [wasmedge_sdk::Memory](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Memory.html).

N.B. In this example, `wasmedge-sdk v0.1.1`, `wasmedge-sys v0.7.1` and `wasmedge-types v0.1.3` are used.

## Wasm module

Before talk about the code, let's first see the wasm module we use in this example. In the wasm module, a linear memory of 1-page (64KiB) size is defined; in addition, three functions are exported from this module: `get_at`, `set_at`, and `mem_size`.

```wasm
(module
  (type $mem_size_t (func (result i32)))
  (type $get_at_t (func (param i32) (result i32)))
  (type $set_at_t (func (param i32) (param i32)))

  # A memory with initial size of 1 page
  (memory $mem 1)

  (func $get_at (type $get_at_t) (param $idx i32) (result i32)
    (i32.load (local.get $idx)))

  (func $set_at (type $set_at_t) (param $idx i32) (param $val i32)
    (i32.store (local.get $idx) (local.get $val)))

  (func $mem_size (type $mem_size_t) (result i32)
    (memory.size))
  
  # Exported functions
  (export "get_at" (func $get_at))
  (export "set_at" (func $set_at))
  (export "mem_size" (func $mem_size))
  (export "memory" (memory $mem)))
```

Next, we'll demonstrate how to manipulate the linear memory by calling the exported functions.

## Load Module

Let's start off by getting all imports right away so you can follow along

```rust
// please add this feature if you're using rust of version < 1.63
// #![feature(explicit_generic_args_with_impl_trait)]

use wasmedge_sdk::{params, Executor, ImportObjectBuilder, Module, Store};
use wasmedge_sys::WasmValue;
use wasmedge_types::wat2wasm;
```

To load a `Module`, `wasmedge-sdk` defines two methods:

- [from_file](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Module.html#method.from_file) loads a wasm module from a file, and meanwhile, validates the loaded wasm module.

- [from_bytes](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Module.html#method.from_bytes) loads a wasm module from an array of in-memory bytes, and meanwhile, validates the loaded wasm module.

Here we use `Module::from_bytes` method to load our wasm module from an array of in-memory bytes.

```rust
let wasm_bytes = wat2wasm(
        r#"
(module
  (type $mem_size_t (func (result i32)))
  (type $get_at_t (func (param i32) (result i32)))
  (type $set_at_t (func (param i32) (param i32)))

  (memory $mem 1)

  (func $get_at (type $get_at_t) (param $idx i32) (result i32)
    (i32.load (local.get $idx)))

  (func $set_at (type $set_at_t) (param $idx i32) (param $val i32)
    (i32.store (local.get $idx) (local.get $val)))

  (func $mem_size (type $mem_size_t) (result i32)
    (memory.size))

  (export "get_at" (func $get_at))
  (export "set_at" (func $set_at))
  (export "mem_size" (func $mem_size))
  (export "memory" (memory $mem)))
"#
    .as_bytes(),
)?;

// loads a wasm module from the given in-memory bytes
let module = Module::from_bytes(None, &wasm_bytes)?;
```

The module returned by `Module::from_bytes` is a compiled module, also called AST Module in WasmEdge terminology. To use it in WasmEdge runtime environment, we need to instantiate the AST module. We use [Store::register_named_module](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Store.html#method.register_named_module) API to achieve the goal.

```rust
// create an executor
let mut executor = Executor::new(None, None)?;

// create a store
let mut store = Store::new()?;

// register the module into the store
let extern_instance = store.register_named_module(&mut executor, "extern", &module)?;
```

In the code above, we register the AST module into a `Store`, in which the module is instantiated, and as a result, a [module instance](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Instance.html) is returned.
