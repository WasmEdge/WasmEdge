# Hello World

In this example, we'll use a wasm module, in which a function `run` is exported and it will call a function `say_hello` from an import module named `env`. The imported function `say_hello` has no inputs and outputs, and only prints a greeting message out.

> The code in the following example is verified on
>
> * wasmedge-sdk v0.5.0
> * wasmedge-sys v0.10.0
> * wasmedge-types v0.3.0

Let's start off by getting all imports right away so you can follow along

```rust
// please add this feature if you're using rust of version < 1.63
// #![feature(explicit_generic_args_with_impl_trait)]

#![feature(never_type)]

use wasmedge_sdk::{
    error::HostFuncError, host_function, params, wat2wasm, Caller, Executor, ImportObjectBuilder,
    Module, Store, WasmValue,
};

```

## Step 1: Define a native function and Create an ImportObject

First, let's define a native function named `say_hello_world` that prints out `Hello, World!`.

```rust
#[host_function]
fn say_hello(caller: &Caller, _args: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    println!("Hello, world!");

    Ok(vec![])
}
```

To use the native function as an import function in the `WasmEdge` runtime, we need an `ImportObject`. `wasmedge-sdk` defines a [ImportObjectBuilder](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.ImportObjectBuilder.html), which provides a group of chaining methods used to create an `ImportObject`. Let's see how to do it.

```rust
// create an import module
let import = ImportObjectBuilder::new()
    .with_func::<(), (), !>("say_hello", say_hello, None)?
    .build("env")?;
```

Now, we have an import module named `env` which holds a host function `say_hello`. As you may notice, the names we used for the import module and the host function are exactly the same as the ones appearing in the wasm module. You can find the wasm module in [Step 2](#step-2-load-a-wasm-module).

## Step 2: Load a wasm module

Now, let's load a wasm module. `wasmedge-sdk` defines two methods in `Module`:

* [from_file](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Module.html#method.from_file) loads a wasm module from a file, and meanwhile, validates the loaded wasm module.

* [from_bytes](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Module.html#method.from_bytes) loads a wasm module from an array of in-memory bytes, and meanwhile, validates the loaded wasm module.

Here we choose `Module::from_bytes` method to load our wasm module from an array of in-memory bytes.

```rust
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

// loads a wasm module from the given in-memory bytes and returns a compiled module
let module = Module::from_bytes(None, &wasm_bytes)?;
```

## Step 3: Register import module and compiled module

To register a compiled module, we need to check if it has dependency on some import modules. In the wasm module this statement `(import "env" "say_hello" (func $say_hello (type $no_args_no_rets_t)))` tells us that it depends on an import module named `env`. Therefore, we need to register the import module first before registering the compiled wasm module.

```rust
// loads a wasm module from the given in-memory bytes
let module = Module::from_bytes(None, &wasm_bytes)?;

// create an executor
let mut executor = Executor::new(None, None)?;

// create a store
let mut store = Store::new()?;

// register the module into the store
store.register_import_module(&mut executor, &import)?;

// register the compiled module into the store and get an module instance
let extern_instance = store.register_named_module(&mut executor, "extern", &module)?;
```

In the code above we use [Executor](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Executor.html) and [Store](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Store.html) to register the import module and the compiled module. `wasmedge-sdk` also provides alternative APIs to do the same thing:
[Vm::register_import_module](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Vm.html#method.register_import_module) and [Vm::register_module_from_bytes](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Vm.html#method.register_module_from_bytes).

## Step 4: Run the exported function

Now we are ready to run the exported function.

```rust
// get the exported function "run"
let run = extern_instance
    .func("run")
    .ok_or_else(|| anyhow::Error::msg("Not found exported function named 'run'."))?;

// run host function
run.call(&mut executor, params!())?;
```

In this example we created an instance of `Executor`, hence, we have two choices to call a [function instance](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Func.html):

* [Func::call](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Func.html#method.call)

* [Executor::run_func](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/trait.Engine.html#tymethod.run_func)

Any one of these two methods requires that you have to get a [function instance](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Func.html).

In addition, [Vm](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Vm.html) defines a group of methods which can invoke host function in different ways. For details, please reference [Vm](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Vm.html).

The complete example can be found in [hello_world.rs](https://github.com/WasmEdge/WasmEdge/blob/master/bindings/rust/wasmedge-sdk/examples/hello_world.rs).
