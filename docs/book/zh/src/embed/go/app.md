# 嵌入 WASM 应用程序

WasmEdge Go SDK 能[嵌入单独的 WebAssembly 应用程序](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_ReadFile) — 比如，一个带着 `main()` 函数，编译成 WebAssembly 的 Rust 应用程序。

我们的 [demo Rust 应用程序](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_ReadFile/rust_readfile)从一个文件中读取内容。注意，WebAssembly 程序的输入和输出数据现在是通过 STDIN 和 STDOUT 传递的。

```rust
use std::env;
use std::fs::File;
use std::io::{self, BufRead};

fn main() {
  // 获取参数。
  let args: Vec<String> = env::args().collect();
  if args.len() <= 1 {
    println!("Rust: ERROR - No input file name.");
    return;
  }

  // 打开文件。
  println!("Rust: Opening input file \"{}\"...", args[1]);
  let file = match File::open(&args[1]) {
    Err(why) => {
      println!("Rust: ERROR - Open file \"{}\" failed: {}", args[1], why);
      return;
    },
    Ok(file) => file,
  };

  // 按行读取文件内容。
  let reader = io::BufReader::new(file);
  let mut texts:Vec<String> = Vec::new();
  for line in reader.lines() {
    if let Ok(text) = line {
      texts.push(text);
    }
  }
  println!("Rust: Read input file \"{}\" succeeded.", args[1]);

  // 获取 stdin 来打印内容。
  println!("Rust: Please input the line number to print the line of file.");
  let stdin = io::stdin();
  for line in stdin.lock().lines() {
    let input = line.unwrap();
    match input.parse::<usize>() {
      Ok(n) => if n > 0 && n <= texts.len() {
        println!("{}", texts[n - 1]);
      } else {
        println!("Rust: ERROR - Line \"{}\" is out of range.", n);
      },
      Err(e) => println!("Rust: ERROR - Input \"{}\" is not an integer: {}", input, e),
    }
  }
  println!("Rust: Process end.");
}
```

将应用程序编译成 WebAssembly。

```bash
$ cd rust_readfile
$ cargo build --target wasm32-wasi
# 输出文件是 target/wasm32-wasi/debug/rust_readfile.wasm
```

我们在 Go 程序里面嵌入 WasmEdge 运行 WebAssembly 函数，这个 Go 程序源代码如下。

```go
package main

import (
  "os"
  "github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
  wasmedge.SetLogErrorLevel()

  var conf = wasmedge.NewConfigure(wasmedge.REFERENCE_TYPES)
  conf.AddConfig(wasmedge.WASI)
  var vm = wasmedge.NewVMWithConfig(conf)
  var wasi = vm.GetImportModule(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],     // 参数
    os.Environ(),    // 环境变量
    []string{".:."}, // 目录映射
  )

  // 实例化 wasm。_start 指的是 wasm 程序的 main() 函数
  vm.RunWasmFile(os.Args[1], "_start")

  vm.Release()
  conf.Release()
}
```

接下来，让我们用 WasmEdge Go SDK 构建 Go 应用程序。

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0
go build
```

运行 Golang 应用程序。

```bash
$ ./read_file rust_readfile/target/wasm32-wasi/debug/rust_readfile.wasm file.txt
Rust: Opening input file "file.txt"...
Rust: Read input file "file.txt" succeeded.
Rust: Please input the line number to print the line of file.
# 输入 "5" 然后按下 Enter。
5
# `file.txt` 文件的第 5 行内容将被输出：
abcDEF___!@#$%^
# 要停止程序，发送 EOF (Ctrl + D)。
^D
# 输出将会打印停止信息：
Rust: Process end.
```

更多的例子可以在 [WasmEdge-go-examples GitHub 仓库](https://github.com/second-state/WasmEdge-go-examples) 中找到。
