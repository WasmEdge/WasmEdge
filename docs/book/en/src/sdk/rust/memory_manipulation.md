# Memory Manipulation

In this example, we'll present how to manipulate the linear memory with the APIs defined in [wasmedge_sdk::Memory](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Memory.html).

> The code in the following example is verified on
>
> * wasmedge-sdk v0.5.0
> * wasmedge-sys v0.10.0
> * wasmedge-types v0.3.0

## Wasm module

Before talking about the code, let's first see the wasm module we use in this example. In the wasm module, a linear memory of 1-page (64KiB) size is defined; in addition, three functions are exported from this module: `get_at`, `set_at`, and `mem_size`.

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

## Load and Register Module

Let's start off by getting all imports right away so you can follow along

```rust
// please add this feature if you're using rust of version < 1.63
// #![feature(explicit_generic_args_with_impl_trait)]

use wasmedge_sdk::{params, wat2wasm, Executor, Module, Store, WasmVal};
```

To load a `Module`, `wasmedge-sdk` defines two methods:

* [from_file](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Module.html#method.from_file) loads a wasm module from a file, and meanwhile, validates the loaded wasm module.

* [from_bytes](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Module.html#method.from_bytes) loads a wasm module from an array of in-memory bytes, and meanwhile, validates the loaded wasm module.

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

In the code above, we register the AST module into a `Store`, in which the module is instantiated, and as a result, a [module instance](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Instance.html) named `extern` is returned.

## Memory

In the previous section, we get an instance by registering a compiled module into the runtime environment. Now we retrieve the memory instance from the module instance, and make use of the APIs defined in [Memory](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Memory.html) to manipulate the linear memory.

```rust
// get the exported memory instance
let mut memory = extern_instance
    .memory("memory")
    .ok_or_else(|| anyhow::anyhow!("failed to get memory instance named 'memory'"))?;

// check memory size
assert_eq!(memory.size(), 1);
assert_eq!(memory.data_size(), 65536);

// grow memory size
memory.grow(2)?;
assert_eq!(memory.size(), 3);
assert_eq!(memory.data_size(), 3 * 65536);

// get the exported functions: "set_at" and "get_at"
let set_at = extern_instance
    .func("set_at")
    .ok_or_else(|| anyhow::Error::msg("Not found exported function named 'set_at'."))?;
let get_at = extern_instance
    .func("get_at")
    .ok_or_else(|| anyhow::Error::msg("Not found exported function named 'get_at`."))?;

// call the exported function named "set_at"
let mem_addr = 0x2220;
let val = 0xFEFEFFE;
set_at.call(&mut executor, params!(mem_addr, val))?;

// call the exported function named "get_at"
let returns = get_at.call(&mut executor, params!(mem_addr))?;
assert_eq!(returns[0].to_i32(), val);

// call the exported function named "set_at"
let page_size = 0x1_0000;
let mem_addr = (page_size * 2) - std::mem::size_of_val(&val) as i32;
let val = 0xFEA09;
set_at.call(&mut executor, params!(mem_addr, val))?;

// call the exported function named "get_at"
let returns = get_at.call(&mut executor, params!(mem_addr))?;
assert_eq!(returns[0].to_i32(), val);
```

The comments in the code explain the meaning of the code sample above, so we don't describe more.

The complete code of this example can be found in [memory.rs](https://github.com/WasmEdge/WasmEdge/blob/master/bindings/rust/wasmedge-sdk/examples/memory.rs).
