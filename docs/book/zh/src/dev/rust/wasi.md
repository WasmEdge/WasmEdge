# 使用操作系统服务

WASI (WebAssembly 系统接口)标准旨在允许 WebAssembly 应用去访问操作系统服务。Rust 编译器中的 `wasm32-wasi` 目标支持 `WASI`。在本章节中，我们将使用一个 [示例工程](https://github.com/second-state/wasm-learning/tree/master/cli/wasi) 来展示怎样使用 Rust 标准 API 去访问操作系统服务。

## 随机数

WebAssembly VM 是纯软件构建。它没有提供随机数的硬件熵源(entropy source)。这也是为什么 WASI 为 WebAssembly 程序定义了一个函数去调用主机操作系统获取随机数。作为一个 Rust 开发者，你只需要使用最流行的(也是事实标准) `rand` 和/或 `getrandom` crates。 使用 `wasm32-wasi` 编译器后端，这些 crates 会在 WebAssembly 字节码中生成 WASI 调用。`Cargo.toml` 如下所示。

```
[dependencies]
rand = "0.7.3"
getrandom = "0.1.14"
```

从 WebAssembly 中获取随机数的 Rust 代码如下所示。

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

## 打印并调试 Rust

Rust `println!` 宏只在 WASI 工作。 这条语句将输出到运行在 WasmEdge 的进程的 `STDOUT` 上。

```rust
pub fn echo(content: &str) -> String {
  println!("Printed from wasi: {}", content);
  return content.to_string();
}
```

## 参数和环境变量

可以将在 WasmEdge 中传递 CLI 参数和访问操作系统的环境变量。因为它们在 Rust 中只是 `env::args()` 和 `env::vars()` 的数组。

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

WASI 允许你的 Rust 函数通过标准 Rust `std::fs` API 访问主机文件系统。在 Rust 程序中，你通过相对路径操作文件。当你启动 WasmEdge 运行时，需要指定相对路径的根目录。

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

## A main() app 

使用 `main()` 函数， Rust 程序可以编译成独立的 WebAssembly 程序。

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

使用下面的命令编译 [Rust 项目](https://github.com/second-state/wasm-learning/tree/master/cli/wasi) 。

```bash
$ cargo build --target wasm32-wasi
```

为了在 `wasmedge` 运行，执行以下操作。`--dir` 选项将 shell 当前目录映射到 WebAssembly 应用程序内的文件系统目录。

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

正如大家所见， 你可以在 Rust `lib.rs` 项目中创建 WebAssembly 函数。你也可以在这些函数中使用 WASI 函数。然而一个重要的警告是，当没有 `main()` 函数时，你需要显示调用某个辅助函数去初始化环境，使得 WASI 函数能正常工作。在 Rust 项目中，在 `Cargo.toml` 添加 helper crate，以便 WASI 初始化代码能被应用到你导出的公共库函数。

```
[dependencies]
... ...
wasmedge-wasi-helper = "=0.2.0"
```

在 Rust 函数中，我们需要白访问任何参数，环境变量或者操作文件前，调用 `_initialize()`。就像下面这样。

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


