# Access OS services

The WASI (WebAssembly Systems Interface) standard is designed to allow WebAssembly applications to access operating system services.
The `wasm32-wasi` target in the Rust compiler supports WASI.
In this section, we will use [an example project](https://github.com/second-state/wasm-learning/tree/master/cli/wasi) to show how to use Rust standard APIs to access operating system services.

## Random numbers

The WebAssembly VM is a pure software construct. It does not have a hardware entropy source for random numbers. That's why WASI defines a function for WebAssembly programs to call its host operating system to get a random seed. As a Rust developer, all you need is to use the popular (de facto standard) `rand` and/or `getrandom` crates. With the `wasm32-wasi` compiler backend, these crates generate the correct WASI calls in the WebAssembly bytecode. The `Cargo.toml` dependencies are as follows.

```toml
[dependencies]
rand = "0.7.3"
getrandom = "0.1.14"
```

The Rust code to get random number from WebAssembly is this.

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

## Printing and debugging from Rust

The Rust `println!` marco just works in WASI. The statements print to the `STDOUT` of the process that runs the WasmEdge.

```rust
pub fn echo(content: &str) -> String {
  println!("Printed from wasi: {}", content);
  return content.to_string();
}
```

## Arguments and environment variables

It is possible to pass CLI arguments to and access OS environment variables in a WasmEdge application.
They are just `env::args()` and `env::vars()` arrays in Rust.

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

## Reading and writing files

WASI allows your Rust functions to access the host computer's file system through the standard Rust `std::fs` API.
In the Rust program, you operate on files through a relative path. The relative path's root is specified when you start the WasmEdge runtime.

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

With a `main()` function, the Rust program can be compiled into a standalone WebAssembly program.

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

Use the command below to compile [the Rust project](https://github.com/second-state/wasm-learning/tree/master/cli/wasi).

```bash
cargo build --target wasm32-wasi
```

To run it in `wasmedge`, do the following. The `--dir` option maps the current directory of the command shell to the file system current directory inside the WebAssembly app.

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

## Functions

As [we have seen](../rust.md#a-simple-function), you can create WebAssembly functions in a Rust `lib.rs` project. You can also use WASI functions in those functions.
However, an important caveat is that, without a `main()` function, you will need to explicitly call a helper function to initialize environment for WASI functions to work properly.
In the Rust program, add a helper crate in Cargo.toml so that the WASI initialization code can be applied to your exported public library functions.

```toml
[dependencies]
... ...
wasmedge-wasi-helper = "=0.2.0"
```

In the Rust function, we need to call `_initialize()` before we access any arguments and environment variables or operate any files.

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
