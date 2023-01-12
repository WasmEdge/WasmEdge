# Run a WebAssembly function with WasmEdge low-level Rust APIs

## Overview

This section demonstrates how to use the Rust APIs of the `wasmedge-sys` crate to run a host function.

As you may know, several mainstream programming languages, such as C/C++, Rust, Go, and Python, support compiling their programs into WebAssembly binary. In this demo, we'll introduce how to use the APIs defined in `Vm` of `wasmedge-sys` crate to call a WebAssembly function which could be coded in any programming language mentioned above.

> The code in the example is verified on
>
> * wasmedge-sys v0.10.0
> * wasmedge-types v0.3.0

## Example

We use `fibonacci.wasm` in this demo, and the contents of the WebAssembly file are presented below. The statement, `(export "fib" (func $fib))`, declares an exported function named `fib`. This function computes a Fibonacci number with a given `i32` number as input. We'll use the function name later to achieve the goal of computing a Fibonacci number.

```wasm
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
```

### Step 1: Create a WasmEdge AST Module

In this step,  we'll create a WasmEdge `AST Module` instance from a WebAssembly file.

* First, create a `Loader` context;

* Then, load a specified WebAssebly file ("fibonacci.wasm") via the `from_file` method of the `Loader` context. If the process is successful, then a WasmEdge `AST Module` instance is returned.

```rust
use wasmedge_sys::Loader;
use std::path::PathBuf;

// create a Loader context
let loader = Loader::create(None).expect("fail to create a Loader context");

// load a wasm module from a specified wasm file, and return a WasmEdge AST Module instance
let path = PathBuf::from("fibonacci.wasm");
let module = loader.from_file(path).expect("fail to load the WebAssembly file");
```

### Step 2: Create a WasmEdge `Vm` context

In WasmEdge, a `Vm` defines a running environment, in which all varieties of instances and contexts are stored and maintained. In the demo code below, we explicitly create a WasmEdge `Store` context, and then use it as one of the inputs in the creation of a `Vm` context. If not specify a `Store` context explicitly, then `Vm` will create a store by itself.

```rust
use wasmedge_sys::{Config, Store, Vm};

// create a Config context
let config = Config::create().expect("fail to create a Config context");

// create a Store context
let mut store = Store::create().expect("fail to create a Store context");

// create a Vm context with the given Config and Store
let mut vm = Vm::create(Some(config), Some(&mut store)).expect("fail to create a Vm context");
```

### Step 3: Invoke the `fib` function

In Step 1, we got a module that hosts the target `fib` function defined in the WebAssembly. Now, we can call the function via the `run_wasm_from_module` method of the `Vm` context by passing the exported function name, `fib`.

```rust
use wasmedge_sys::WasmValue;

// run a function
let returns = vm.run_wasm_from_module(module, "fib", [WasmValue::from_i32(5)]).expect("fail to run the target function in the module");

println!("The result of fib(5) is {}", returns[0].to_i32());
```

This is the final result printing on the screen:

```bash
The result of fib(5) is 8
```
