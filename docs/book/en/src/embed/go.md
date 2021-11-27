# WasmEdge Go SDK


The followings are the guides to working with the WasmEdge Go API. You can embed the WasmEdge into your go application through the WasmEdge Go API.

## Table of Contents

* [Getting Started](#getting-started)
* [WasmEdge-go Extensions](#wasmedge-go-extensions)
* [Go-API Examples](#go-api-examples)
	* [Embed a function](#embed-a-function)
	* [Embed a full program](#embed-a-full-program)
	* [WasmEdge AOT Compiler in GO](#wasmedge-aot-compiler-in-go)

## Getting Started

The WasmEdge-go requires golang version >= 1.15. Please check your golang version before installation. Developers can [download golang here](https://golang.org/dl/).

```bash
$ go version
go version go1.16.5 linux/amd64
```

Developers must [install the WasmEdge shared library](https://github.com/WasmEdge/WasmEdge/blob/master/docs/install.md) with the same `WasmEdge-go` release version.

```bash
wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v 0.9.0-rc.4
```

For more details, please refer to the [Installation Guide](../start/install.md) for the WasmEdge installation.

For the developers need the `TensorFlow` or `Image` extension for `WasmEdge-go`, please install the `WasmEdge` with extensions:

```bash
wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v 0.9.0-rc.4
```

Noticed that the `TensorFlow` and `Image` extensions are only for the `Linux` platforms.

Install the `WasmEdge-go` package and build in your Go project directory:

```bash
$ go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.0-rc3
$ go build
```

## WasmEdge-go Extensions

By default, the `WasmEdge-go` only turns on the basic runtime.

`WasmEdge-go` has the following extensions:

 - Tensorflow
    * This extension supports the host functions in [WasmEdge-tensorflow](https://github.com/second-state/WasmEdge-tensorflow).
    * The `TensorFlow` extension when installing `WasmEdge` is required. Please install `WasmEdge` with the `-e tensorflow` command.
    * For using this extension, the tag `tensorflow` when building is required:
        ```bash
        $ go build -tags tensorflow
        ```
 - Image
    * This extension supports the host functions in [WasmEdge-image](https://github.com/second-state/WasmEdge-image).
    * The `Image` extension when installing `WasmEdge` is required. Please install `WasmEdge` with the `-e image` command.
    * For using this extension, the tag `image` when building is required:
        ```bash
        $ go build -tags image
        ```

Users can also turn on the multiple extensions when building:

```bash
$ go build -tags image,tensorflow
```

For examples, please refer to the [example repository](https://github.com/second-state/WasmEdge-go-examples/).

## Go-API Examples

### Embed a function

Note: At this time, we require Rust compiler version 1.50 or less in order for WebAssembly functions to work with WasmEdge’s Golang API. We will [catch up to the latest Rust](https://github.com/WasmEdge/WasmEdge/issues/264) compiler version once the Interface Types spec is finalized and supported.

In [this example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_BindgenFuncs), we will demonstrate how to call a few simple WebAssembly functions from a Golang app. The [functions](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_BindgenFuncs/rust_bindgen_funcs/src/lib.rs) are written in Rust, and require complex call parameters and return values. The #[wasm_bindgen] macro is needed for the compiler tools to auto-generate the correct code to pass call parameters from Golang to WebAssembly.

Note: The WebAssembly spec only supports a few simple data types out of the box. It [does not support](https://medium.com/wasm/strings-in-webassembly-wasm-57a05c1ea333) types such as string and array. In order to pass rich types in Golang to WebAssembly, the compiler needs to convert them to simple integers. For example, it converts a string into an integer memory address and an integer length. The wasm_bindgen tool, embedded in rustwasmc, does this conversion automatically.

```rust
use wasm_bindgen::prelude::*;
use num_integer::lcm;
use sha3::{Digest, Sha3_256, Keccak256};

#[wasm_bindgen]
pub fn say(s: &str) -> String {
  let r = String::from("hello ");
  return r + s;
}

#[wasm_bindgen]
pub fn obfusticate(s: String) -> String {
  (&s).chars().map(|c| {
    match c {
      'A' ..= 'M' | 'a' ..= 'm' => ((c as u8) + 13) as char,
      'N' ..= 'Z' | 'n' ..= 'z' => ((c as u8) - 13) as char,
      _ => c
    }
  }).collect()
}

#[wasm_bindgen]
pub fn lowest_common_multiple(a: i32, b: i32) -> i32 {
  let r = lcm(a, b);
  return r;
}

#[wasm_bindgen]
pub fn sha3_digest(v: Vec<u8>) -> Vec<u8> {
  return Sha3_256::digest(&v).as_slice().to_vec();
}

#[wasm_bindgen]
pub fn keccak_digest(s: &[u8]) -> Vec<u8> {
  return Keccak256::digest(s).as_slice().to_vec();
}
```

First, we use the rustwasmc tool to compile the Rust source code into WebAssembly bytecode functions using Rust 1.50 or less.

```bash
$ rustup default 1.50.0
$ cd rust_bindgen_funcs
$ rustwasmc build
# The output WASM will be pkg/rust_bindgen_funcs_lib_bg.wasm
```

The [Golang source code](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_BindgenFuncs/bindgen_funcs.go) to run the WebAssembly function in WasmEdge is as follows. The ExecuteBindgen() function calls the WebAssembly function and passes the call parameters using the #[wasm_bindgen] convention.


```go
package main

import (
    "fmt"
    "os"
    "github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
    /// Expected Args[0]: program name (./bindgen_funcs)
    /// Expected Args[1]: wasm or wasm-so file (rust_bindgen_funcs_lib_bg.wasm))

    wasmedge.SetLogErrorLevel()

    var conf = wasmedge.NewConfigure(wasmedge.WASI)
    var vm = wasmedge.NewVMWithConfig(conf)
    var wasi = vm.GetImportObject(wasmedge.WASI)
    wasi.InitWasi(
        os.Args[1:],     /// The args
        os.Environ(),    /// The envs
        []string{".:."}, /// The mapping directories
        []string{},      /// The preopens will be empty
    )

    /// Instantiate wasm
    vm.LoadWasmFile(os.Args[1])
    vm.Validate()
    vm.Instantiate()

    /// Run bindgen functions
    var res interface{}
    var err error
    
    res, err = vm.ExecuteBindgen("say", wasmedge.Bindgen_return_array, []byte("bindgen funcs test"))
    if err == nil {
        fmt.Println("Run bindgen -- say:", string(res.([]byte)))
    } 
    res, err = vm.ExecuteBindgen("obfusticate", wasmedge.Bindgen_return_array, []byte("A quick brown fox jumps over the lazy dog"))
    if err == nil {
        fmt.Println("Run bindgen -- obfusticate:", string(res.([]byte)))
    } 
    res, err = vm.ExecuteBindgen("lowest_common_multiple", wasmedge.Bindgen_return_i32, int32(123), int32(2))
    if err == nil {
        fmt.Println("Run bindgen -- lowest_common_multiple:", res.(int32))
    } 
    res, err = vm.ExecuteBindgen("sha3_digest", wasmedge.Bindgen_return_array, []byte("This is an important message"))
    if err == nil {
        fmt.Println("Run bindgen -- sha3_digest:", res.([]byte))
    } 
    res, err = vm.ExecuteBindgen("keccak_digest", wasmedge.Bindgen_return_array, []byte("This is an important message"))
    if err == nil {
        fmt.Println("Run bindgen -- keccak_digest:", res.([]byte))
    } 

    vm.Release()
    conf.Release()
}
```

Next, let's build the Golang application with the WasmEdge Golang SDK.

```bash
$ go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.0-rc3
$ go build
```

Run the Golang application and it will run the WebAssembly functions embedded in the WasmEdge runtime.

```bash
$ ./bindgen_funcs rust_bindgen_funcs/pkg/rust_bindgen_funcs_lib_bg.wasm
Run bindgen -- say: hello bindgen funcs test
Run bindgen -- obfusticate: N dhvpx oebja sbk whzcf bire gur ynml qbt
Run bindgen -- lowest_common_multiple: 246
Run bindgen -- sha3_digest: [87 27 231 209 189 105 251 49 159 10 211 250 15 159 154 181 43 218 26 141 56 199 25 45 60 10 20 163 54 211 195 203]
Run bindgen -- keccak_digest: [126 194 241 200 151 116 227 33 216 99 159 22 107 3 177 169 216 191 114 156 174 193 32 159 246 228 245 133 52 75 55 27]
```

### Embed a full program

Note: You can use the latest Rust compiler to create a standalone WasmEdge application with a main.rs and then embed it into a Golang application.

Besides functions, the WasmEdge Golang SDK can also [embed standalone WebAssembly applications](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_ReadFile) — ie a Rust application with a main() function compiled into WebAssembly.

Our [demo Rust application](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_ReadFile/rust_readfile) reads from a file. Note that there is no need for #{wasm_bindgen] here, as the WebAssembly program’s input and output data are now passed by the STDIN and STDOUT.


```rust
use std::env;
use std::fs::File;
use std::io::{self, BufRead};

fn main() {
    // Get the argv.
    let args: Vec<String> = env::args().collect();
    if args.len() <= 1 {
        println!("Rust: ERROR - No input file name.");
        return;
    }

    // Open the file.
    println!("Rust: Opening input file \"{}\"...", args[1]);
    let file = match File::open(&args[1]) {
        Err(why) => {
            println!("Rust: ERROR - Open file \"{}\" failed: {}", args[1], why);
            return;
        },
        Ok(file) => file,
    };

    // Read lines.
    let reader = io::BufReader::new(file);
    let mut texts:Vec<String> = Vec::new();
    for line in reader.lines() {
        if let Ok(text) = line {
            texts.push(text);
        }
    }
    println!("Rust: Read input file \"{}\" succeeded.", args[1]);

    // Get stdin to print lines.
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

Use the rustwasmc tool to compile the application into WebAssembly.

```bash
$ cd rust_readfile
$ rustwasmc build
# The output file will be pkg/rust_readfile.wasm
```

The Golang source code to run the WebAssembly function in WasmEdge is as follows.

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
    var wasi = vm.GetImportObject(wasmedge.WASI)
    wasi.InitWasi(
        os.Args[1:],     /// The args
        os.Environ(),    /// The envs
        []string{".:."}, /// The mapping directories
        []string{},      /// The preopens will be empty
    )

    /// Instantiate wasm. _start refers to the main() function
    vm.RunWasmFile(os.Args[1], "_start")

    vm.Release()
    conf.Release()
}
```

Next, let's build the Golang application with the WasmEdge Golang SDK.

```bash
$ go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.0-rc3
$ go build
```

Run the Golang application.

```bash
$ ./read_file rust_readfile/pkg/rust_readfile.wasm file.txt
Rust: Opening input file "file.txt"...
Rust: Read input file "file.txt" succeeded.
Rust: Please input the line number to print the line of file.
# Input "5" and press Enter.
5
# The output will be the 5th line of `file.txt`:
abcDEF___!@#$%^
# To terminate the program, send the EOF (Ctrl + D).
^D
# The output will print the terminate message:
Rust: Process end.
```

More examples can be found at [the WasmEdge-go-examples GitHub repo.](https://github.com/second-state/WasmEdge-go-examples)

### WasmEdge AOT Compiler in GO

The [go_WasmAOT example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_WasmAOT) provide a tool for compiling a WASM file into compiled-WASM for AOT mode.

