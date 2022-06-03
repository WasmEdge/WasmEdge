
# 斐波那契数列的同时计算

在这个例子中，我们将演示如何使用 `wasmedge-sys` 中定义的对象和 API 来同时计算斐波那契数。

在下面的代码中，我们创建了两个子线程 `thread_a` 和 `thread_b` ，它们分别通过调用主机函数 `fib` 来响应计算 `Fib(4)` 和 `Fib(5)`。之后，主线程通过将 `thread_a` 和 `thread_b` 返回的数字相加来计算 `Fib(6)`。

* 第一步：创建 Vm 上下文并注册 WebAssembly 模块。

    ```rust
    // create a Config context
    let mut config = Config::create()?;
    config.bulk_memory_operations(true);

    // create a Store context
    let store = Store::create()?;

    // create a Vm context with the given Config and Store
    let mut vm = Vm::create(Some(config), Some(store))?;

    // register a wasm module from a wasm file
    let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
    vm.register_wasm_from_file("extern", file)?;
    ```

* 第二步：创建两个子线程分别计算 `Fib(4)` 和 `Fib(5)`。

    ```rust
    let vm = Arc::new(Mutex::new(vm));

    // compute fib(4) by a child thread
    let vm_cloned = Arc::clone(&vm);
    let handle_a = thread::spawn(move || {
      let vm_child_thread = vm_cloned.lock().expect("fail to lock vm");
      let returns = vm_child_thread
        .run_registered_function("extern", "fib", [Value::from_i32(4)])
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
        .run_registered_function("extern", "fib", [Value::from_i32(5)])
        .expect("fail to compute fib(5)");

      let fib5 = returns[0].to_i32();
      println!("fib(5) by child thread: {}", fib5);

      fib5
    });

    ```

* 第三步：获取两个子线程的返回值，计算 `Fib(6)`。

    ```Rust
    let fib4 = handle_a.join().unwrap();
    let fib5 = handle_b.join().unwrap();

    // compute fib(6)
    println!("fib(6) = fib(5) + fib(1) = {}", fib5 + fib4);
    ```

上面代码的最终结果应该像下面一样打印在屏幕上:

```bash
fib(4) by child thread: 5
fib(5) by child thread: 8
fib(6) = fib(5) + fib(1) = 13
```

此演示中的完整代码可以在 [WasmEdge Github](https://github.com/WasmEdge/WasmEdge/blob/master/bindings/rust/wasmedge-sys/examples/threads.rs).
