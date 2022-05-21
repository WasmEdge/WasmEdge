# 使用操作系统服务

WASI（WebAssembly 系统接口）标准被设计来让 WebAssembly 应用程序可以访问操作系统服务。
Rust 编译器中的 `wasm32-wasi` 目标支持 WASI。
在这一部分中，我们将使用[一个示例工程](https://github.com/second-state/wasm-learning/tree/master/cli/wasi)来展示如何使用 Rust 的标准库来访问操作系统服务。

## 随机数

WebAssembly 虚拟机是一个纯软件实现，它不具有生成随机数必须的硬件资源。这就是 WASI 为 WebAssembly 定义了一个从宿主操作系统获取一个随机种子的函数的原因。作为一个 Rust 开发者，你需要做的只是使用受欢迎的 `rand` 或者 `getrandom` 包。得益于 `wasm32-wasi` 编译器后端，这些包将在 WebAssembly 字节码中生成正确的 WASI 调用。如下是 `Cargo.toml` 中的依赖部分。

```toml
[dependencies]
rand = "0.7.3"
getrandom = "0.1.14"
```

如下是在 WebAssembly 生成随机数的 Rust 代码。

```rust
use rand::prelude::*;

pub fn get_random_i32() -> i32 {
  let x: i32 = random();
  return x;
}

pub fn get_random_bytes() -> Vec<u8> {
  let mut rng = thread_rng();
  let mut arr = [0u8; 128];
  rng.fill(&mut arr[..]);
  return arr.to_vec();
}
```

## 从 Rust 中输出和调试

Rust 中的 `println!` 宏同样可以用在 WASI 中。下面的语句向运行 WasmEdge 的进程的 `STDOUT` 输出。

```rust
pub fn echo(content: &str) -> String {
  println!("Printed from wasi: {}", content);
  return content.to_string();
}
```

## 参数以及环境变量

在 WasmEdge 应用程序中，你可以传递命令行参数并且获取操作系统环境变量。
在 Rust 中你可以使用 `env::args()` 和 `env::vars()` 访问他们。

```rust
use std::env;

pub fn print_env() {
  println!("The env vars are as follows.");
  for (key, value) in env::vars() {
    println!("{}: {}", key, value);
  }

  println!("The args are as follows.");
  for argument in env::args() {
    println!("{}", argument);
  }
}
```

## 读写文件

WASI 让你可以通过标准的 Rust `std::fs` 接口来访问宿主的文件系统。
在 Rust 程序中，你通过相对路径来操作文件，相对路径的根目录可以在启动 WasmEdge Runtime 的时候被指定。

```rust
use std::fs;
use std::fs::File;
use std::io::{Write, Read};

pub fn create_file(path: &str, content: &str) {
  let mut output = File::create(path).unwrap();
  output.write_all(content.as_bytes()).unwrap();
}

pub fn read_file(path: &str) -> String {
  let mut f = File::open(path).unwrap();
  let mut s = String::new();
  match f.read_to_string(&mut s) {
    Ok(_) => s,
    Err(e) => e.to_string(),
  }
}

pub fn del_file(path: &str) {
  fs::remove_file(path).expect("Unable to delete");
}
```

## 包含 main() 的应用程序

包含 `main()` 函数的 Rust 程序可以被编译为一个独立的 WebAssembly 程序。

```rust
fn main() {
  println!("Random number: {}", get_random_i32());
  println!("Random bytes: {:?}", get_random_bytes());
  println!("{}", echo("This is from a main function"));
  print_env();
  create_file("tmp.txt", "This is in a file");
  println!("File content is {}", read_file("tmp.txt"));
  del_file("tmp.txt");
}
```

使用如下的命令来编译 [Rust 工程](https://github.com/second-state/wasm-learning/tree/master/cli/wasi)。

```bash
cargo build --target wasm32-wasi
```

使用如下命令在 `wasmedge` 中运行它。`--dir` 选项将命令行当前的目录映射为 ＷebAssembly 程序中文件系统的当前目录。

```bash
$ wasmedge --dir .:. target/wasm32-wasi/debug/wasi.wasm hello
Random number: -68634548
Random bytes: [87, 117, 194, 122, 74, 189, 29, 1, 113, 26, 90, 6, 151, 20, 11, 169, 131, 212, 161, 220, 216, 190, 77, 234, 30, 10, 159, 7, 14, 89, 81, 111, 247, 136, 39, 195, 83, 90, 153, 225, 66, 16, 150, 217, 137, 172, 216, 203, 251, 37, 4, 27, 32, 57, 76, 237, 99, 147, 24, 175, 208, 157, 3, 220, 46, 224, 199, 153, 144, 96, 120, 89, 160, 38, 171, 239, 87, 218, 41, 184, 220, 78, 157, 57, 229, 198, 222, 72, 219, 118, 237, 27, 229, 28, 51, 116, 88, 101, 40, 139, 160, 51, 156, 102, 66, 233, 101, 50, 131, 9, 253, 186, 73, 148, 85, 36, 155, 254, 168, 202, 23, 96, 181, 99, 120, 136, 28, 147]
This is from a main function
The env vars are as follows.
... ...
The args are as follows.
target/wasm32-wasi/debug/wasi.wasm
hello
File content is This is in a file
```

## 函数

正如[我们之前看到的](../rust.md#一个简单的函数)，你可以在 Rust `lib.rs` 中创建 WebAssembly 函数。在这些函数中，你同样可以使用 WASI 函数。然而，需要注意的是，没有 `main()` 函数的情况下，你将会需要显式地调用一个辅助函数来初始化环境，以此让 WASI 函数正常工作。
在 Cargo.toml 中添加一个辅助包，这样的话 WASI 初始化代码将会应用在你导出的的公开库函数上。

```toml
[dependencies]
... ...
wasmedge-wasi-helper = "=0.2.0"
```

在访问任何参数和环境变量或者操作任何文件之前，我们需要调用 _initialize() 函数。

```rust
pub fn print_env() -> i32 {
  _initialize();
  ... ...
}

pub fn create_file(path: &str, content: &str) -> String {
  _initialize();
  ... ...
}

pub fn read_file(path: &str) -> String {
  _initialize();
  ... ...
}

pub fn del_file(path: &str) -> String {
  _initialize();
  ... ...
}
```
