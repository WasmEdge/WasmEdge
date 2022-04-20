# 使用 WasmEdge 底层 Rust API 运行 WebAssembly 函数

本节演示如何使用 `wasmedge sys` crate 的 Rust API 来运行 host 函数。

你可能知道，一些主流编程语言，如 C/C++、Rust、Go 和 Python，都支持将程序编译成 WebAssembly 二进制文件。在本演示中，我们将介绍如何使用 WasmEdge Rust API 调用 WebAssembly 函数，该函数可以用上述任何编程语言编写。

我们使用 `fibonacci.wasm` 和 WebAssembly 文件的内容如下所示。语句 `(export“fib”（func$fib))` 声明了一个名为 `fib` 的导出函数。此函数使用给定的 `i32` 数作为输入计算斐波那契数。我们稍后将使用函数名来实现计算斐波那契数的目标。

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

## 步骤 1: 创建 WasmEdge AST 模块

在这一步中，我们将从 WebAssembly 文件创建一个 WasmEdge `AST Module` 实例。

- 首先，创建一个 `loader` 实例;

- 然后，通过 `loader` 程序实例的 `from_file` 方法加载指定的 WebAssebly 文件（“fibonacci.wasm”）。如果该过程成功，则返回一个 WasmEdge `AST module` 实例。

```rust
use wasmedge_sys::Loader
use std::path::PathBuf;

// 创建 Loader 程序实例
let loader = Loader::create(None).expect("fail to create a Loader context");

// 从指定的 wasm 文件加载 wasm 模块，并返回 WasmEdge AST 模块实例
let path = PathBuf::from("fibonacci.wasm");
let mut module = loader.from_file(path).expect("fail to load the WebAssembly file");
```

## 步骤 2: 创建一个 `Vm` 实例

在 WasmEdge 中， `Vm` 定义了一个运行环境，在其中存储和维护各种实例。在下面的演示代码中，我们可以创建一个 WasmEdge `store` 实例，然后将其用作创建 `Vm` 实例的输入之一。如果没有明确指定 `store` 实例，那么 `Vm` 将自己创建一个 `store` 。

```rust
use wasmedge_sys::{Config, Store, Vm};

// 创建一个 config 实例
let config = Config::create().expect("fail to create a Config context");

// 创建 store 实例
let store = Store::create().expect("fail to create a Store context");

// 使用指定的配置和存储创建 Vm 实例
let mut vm = Vm::create(Some(config), Some(store)).expect("fail to create a Vm context");
```

## 步骤 3: 调用 fib 函数

在步骤 1中，我们得到了一个模块，该模块承载 WebAssembly 中定义的目标 `fib` 函数。现在，我们可以通过传递导出的函数名 `fib` ，通过 `Vm` 实例的 `run_wasm_from_module` 方法调用该函数。

```rust
use wasmedge_sys::Value;

// 运行一个函数
let returns = vm.run_wasm_from_module(module, "fib", [Value::from_i32(5)]).expect("fail to run the target function in the module");

println!("The result of fib(5) is {}", returns[0].to_i32());
```

这是在屏幕上打印的最终结果：

 ```bash
 The result of fib(5) is 8
 ```
