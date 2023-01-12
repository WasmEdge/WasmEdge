# Compute Fibonacci numbers concurrently

## Overview

In this example, we will demonstrate how to use the objects and the APIs defined in `wasmedge-sys` to compute Fibonacci numbers concurrently. we creates two child threads, `thread_a` and `thread_b`, which are responsible for compute `Fib(4)` and `Fib(5)` by calling the host function `fib`, respectively. After that, the main thread computes `Fib(6)` by adding the numbers returned by `thread_a` and `thread_b`.

> The code in the example is verified on
>
> * wasmedge-sys v0.10.0
> * wasmedge-types v0.3.0

### Step 1: create a Vm context and register the WebAssembly module

  ```rust
  // create a Config context
  let mut config = Config::create()?;
  config.bulk_memory_operations(true);

  // create a Store context
  let mut store = Store::create()?;

  // create a Vm context with the given Config and Store
  let mut vm = Vm::create(Some(config), Some(&mut store))?;

  // register a wasm module from a wasm file
  let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
      .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
  vm.register_wasm_from_file("extern", file)?;

  ```

### Step 2: create two child threads to compute `Fib(4)` and `Fib(5)` respectively

  ```rust
  let vm = Arc::new(Mutex::new(vm));

  // compute fib(4) by a child thread
  let vm_cloned = Arc::clone(&vm);
  let handle_a = thread::spawn(move || {
      let vm_child_thread = vm_cloned.lock().expect("fail to lock vm");
      let returns = vm_child_thread
          .run_registered_function("extern", "fib", [WasmValue::from_i32(4)])
          .expect("fail to compute fib(4)");

      let fib4 = returns[0].to_i32();
      println!("fib(4) by child thread: {}", fib4);

      fib4
  });

  // compute fib(5) by a child thread
  let vm_cloned = Arc::clone(&vm);
  let handle_b = thread::spawn(move || {
      let vm_child_thread = vm_cloned.lock().expect("fail to lock vm");
      let returns = vm_child_thread
          .run_registered_function("extern", "fib", [WasmValue::from_i32(5)])
          .expect("fail to compute fib(5)");

      let fib5 = returns[0].to_i32();
      println!("fib(5) by child thread: {}", fib5);

      fib5
  });

  ```

### Step3: Get the returns from the two child threads, and compute `Fib(6)`

  ```Rust
  let fib4 = handle_a.join().unwrap();
  let fib5 = handle_b.join().unwrap();

  // compute fib(6)
  println!("fib(6) = fib(5) + fib(1) = {}", fib5 + fib4);
  ```

The final result of the code above should be printed on the screen like below:

```bash
fib(4) by child thread: 5
fib(5) by child thread: 8
fib(6) = fib(5) + fib(1) = 13
```

The complete code in this demo can be found in [threads.rs](https://github.com/WasmEdge/WasmEdge/blob/master/bindings/rust/wasmedge-sys/examples/threads.rs).
