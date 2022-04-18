# WasmEdge Go API references

The followings are the guides to working with the WasmEdge-Go SDK.

## Table of Contents

* [Getting Started](#getting-started)
  * [WasmEdge Installation](#wasmedge-installation)
  * [Get WasmEdge-go](#get-wasmedge-go)
  * [WasmEdge-go Extensions](#wasmedge-go-extensions)
  * [Example of Embedding A Function with wasmedge-bindgen](#example-of-embedding-a-function-with-wasmedge-bindgen)
  * [Example of Embedding A Full WASI Program](#example-of-embedding-a-full-wasi-program)
* [WasmEdge-go Basics](#wasmedge-go-basics)
  * [Version](#version)
  * [Logging Settings](#logging-settings)
  * [Value Types](#value-types)
  * [Results](#results)
  * [Contexts And Their Life Cycles](#contexts-and-their-life-cycles)
  * [WASM data structures](#wasm-data-structures)
  * [Configurations](#configurations)
* [WasmEdge VM](#wasmedge-vm)
  * [WASM Execution Example With VM Object](#wasm-execution-example-with-vm-object)
  * [VM Creations](#vm-creations)
  * [Preregistrations](#preregistrations)
  * [Host Module Registrations](#host-module-registrations)
  * [WASM Registrations And Executions](#wasm-registrations-and-executions)
  * [Instance Tracing](#instance-tracing)
* [WasmEdge Runtime](#wasmedge-runtime)
  * [WASM Execution Example Step-By-Step](#wasm-execution-example-step-by-step)
  * [Loader](#loader)
  * [Validator](#validator)
  * [Executor](#executor)
  * [AST Module](#ast-module)
  * [Store](#store)
  * [Instances](#instances)
  * [Host Functions](#host-functions)
* [WasmEdge AOT Compiler](#wasmedge-aot-compiler)
  * [Compilation Example](#compilation-example)
  * [Compiler Options](#compiler-options)

## Getting Started

The WasmEdge-go requires golang version >= 1.16. Please check your golang version before installation. Developers can [download golang here](https://golang.org/dl/).

```bash
$ go version
go version go1.16.5 linux/amd64
```

### WasmEdge Installation

Developers must [install the WasmEdge shared library](start/install.md) with the same `WasmEdge-go` release or pre-release version.

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v 0.10.0-alpha.1
```

For the developers need the `TensorFlow` or `Image` extension for `WasmEdge-go`, please install the `WasmEdge` with extensions:

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -e tf,image -v 0.10.0-alpha.1
```

Noticed that the `TensorFlow` and `Image` extensions are only for the `Linux` platforms.
After installation, developers can use the `source` command to update the include and linking searching path.

### Get WasmEdge-go

After the WasmEdge installation, developers can get the `WasmEdge-go` package and build it in your Go project directory.

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0-alpha.1
go build
```

### WasmEdge-go Extensions

By default, the `WasmEdge-go` only turns on the basic runtime.

`WasmEdge-go` has the following extensions (on the Linux platforms only):

* Tensorflow
  * This extension supports the host functions in [WasmEdge-tensorflow](https://github.com/second-state/WasmEdge-tensorflow).
  * The `TensorFlow` extension when installing `WasmEdge` is required. Please install `WasmEdge` with the `-e tensorflow` command.
  * For using this extension, the tag `tensorflow` when building is required:

    ```bash
    go build -tags tensorflow
    ```

* Image
  * This extension supports the host functions in [WasmEdge-image](https://github.com/second-state/WasmEdge-image).
  * The `Image` extension when installing `WasmEdge` is required. Please install `WasmEdge` with the `-e image` command.
  * For using this extension, the tag `image` when building is required:

    ```bash
    go build -tags image
    ```

Users can also turn on the multiple extensions when building:

```bash
go build -tags image,tensorflow
```

### Example of Embedding A Function with wasmedge-bindgen

In [this example](https://github.com/second-state/WasmEdge-go-examples/tree/master/wasmedge-bindgen/go_BindgenFuncs), we will demonstrate how to call a few simple WebAssembly functions with wasmedge-bindgen from a Golang app. The [functions](https://github.com/second-state/WasmEdge-go-examples/blob/master/wasmedge-bindgen/go_BindgenFuncs/rust_bindgen_funcs/src/lib.rs) are written in Rust, and require complex call parameters and return values.

While the WebAssembly only supports a few simple data types out of the box. It [does not support](https://medium.com/wasm/strings-in-webassembly-wasm-57a05c1ea333) types such as string and array. In order to pass rich types in Golang to WebAssembly, the compiler needs to convert them to simple integers. For example, it converts a string into an integer memory address and an integer length. The `#[wasmedge_bindgen]` macro does this conversion automatically, combining it with Golang's `wasmedge-bindgen` package to auto-generate the correct code to pass call parameters from Golang to WebAssembly.

```rust
use wasmedge_bindgen::*;
use wasmedge_bindgen_macro::*;
use num_integer::lcm;
use sha3::{Digest, Sha3_256, Keccak256};
use serde::{Serialize, Deserialize};

#[derive(Serialize, Deserialize, Debug)]
struct Point {
  x: f32,
  y: f32
}

#[derive(Serialize, Deserialize, Debug)]
struct Line {
  points: Vec<Point>,
  valid: bool,
  length: f32,
  desc: String
}

#[wasmedge_bindgen]
pub fn create_line(p1: String, p2: String, desc: String) -> String {
  let point1: Point = serde_json::from_str(p1.as_str()).unwrap();
  let point2: Point = serde_json::from_str(p2.as_str()).unwrap();
  let length = ((point1.x - point2.x) * (point1.x - point2.x) + (point1.y - point2.y) * (point1.y - point2.y)).sqrt();

  let valid = if length == 0.0 { false } else { true };

  let line = Line { points: vec![point1, point2], valid: valid, length: length, desc: desc };

  return serde_json::to_string(&line).unwrap();
}

#[wasmedge_bindgen]
pub fn say(s: String) -> String {
  let r = String::from("hello ");
  return r + s.as_str();
}

#[wasmedge_bindgen]
pub fn obfusticate(s: String) -> String {
  (&s).chars().map(|c| {
    match c {
      'A' ..= 'M' | 'a' ..= 'm' => ((c as u8) + 13) as char,
      'N' ..= 'Z' | 'n' ..= 'z' => ((c as u8) - 13) as char,
      _ => c
    }
  }).collect()
}

#[wasmedge_bindgen]
pub fn lowest_common_multiple(a: i32, b: i32) -> i32 {
  return lcm(a, b);
}

#[wasmedge_bindgen]
pub fn sha3_digest(v: Vec<u8>) -> Vec<u8> {
  return Sha3_256::digest(&v).as_slice().to_vec();
}

#[wasmedge_bindgen]
pub fn keccak_digest(s: Vec<u8>) -> Vec<u8> {
  return Keccak256::digest(&s).as_slice().to_vec();
}
```

First, compile the Rust source code into WebAssembly bytecode functions.

```bash
rustup target add wasm32-wasi
cd rust_bindgen_funcs
cargo build --target wasm32-wasi --release
# The output WASM will be target/wasm32-wasi/release/rust_bindgen_funcs_lib.wasm
```

The [Golang source code](https://github.com/second-state/WasmEdge-go-examples/blob/master/wasmedge-bindgen/go_BindgenFuncs/bindgen_funcs.go) to run the WebAssembly function in WasmEdge is as follows. The `bg.Execute()` function calls the WebAssembly function and passes the parameters with the `wasmedge-bindgen` supporting.

```go
package main

import (
  "fmt"
  "os"

  "github.com/second-state/WasmEdge-go/wasmedge"
  bindgen "github.com/second-state/wasmedge-bindgen/host/go"
)

func main() {
  // Expected Args[0]: program name (./bindgen_funcs)
  // Expected Args[1]: wasm file (rust_bindgen_funcs_lib.wasm))
  
  // Set not to print debug info
  wasmedge.SetLogErrorLevel()

  // Create configure
  var conf = wasmedge.NewConfigure(wasmedge.WASI)

  // Create VM with configure
  var vm = wasmedge.NewVMWithConfig(conf)

  // Init WASI
  var wasi = vm.GetImportModule(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],     // The args
    os.Environ(),    // The envs
    []string{".:."}, // The mapping preopens
  )

  // Load and validate the wasm
  vm.LoadWasmFile(os.Args[1])
  vm.Validate()

  // Instantiate the bindgen and vm
  bg := bindgen.Instantiate(vm)

  // create_line: string, string, string -> string (inputs are JSON stringified)  
  res, err := bg.Execute("create_line", "{\"x\":2.5,\"y\":7.8}", "{\"x\":2.5,\"y\":5.8}", "A thin red line")
  if err == nil {
    fmt.Println("Run bindgen -- create_line:", res[0].(string))
  } else {
    fmt.Println("Run bindgen -- create_line FAILED", err)
  }

  // say: string -> string
  res, err = bg.Execute("say", "bindgen funcs test")
  if err == nil {
    fmt.Println("Run bindgen -- say:", res[0].(string))
  } else {
    fmt.Println("Run bindgen -- say FAILED")
  }

  // obfusticate: string -> string
  res, err = bg.Execute("obfusticate", "A quick brown fox jumps over the lazy dog")
  if err == nil {
    fmt.Println("Run bindgen -- obfusticate:", res[0].(string))
  } else {
    fmt.Println("Run bindgen -- obfusticate FAILED")
  }

  // lowest_common_multiple: i32, i32 -> i32
  res, err = bg.Execute("lowest_common_multiple", int32(123), int32(2))
  if err == nil {
    fmt.Println("Run bindgen -- lowest_common_multiple:", res[0].(int32))
  } else {
    fmt.Println("Run bindgen -- lowest_common_multiple FAILED")
  }

  // sha3_digest: array -> array
  res, err = bg.Execute("sha3_digest", []byte("This is an important message"))
  if err == nil {
    fmt.Println("Run bindgen -- sha3_digest:", res[0].([]byte))
  } else {
    fmt.Println("Run bindgen -- sha3_digest FAILED")
  }

  // keccak_digest: array -> array
  res, err = bg.Execute("keccak_digest", []byte("This is an important message"))
  if err == nil {
    fmt.Println("Run bindgen -- keccak_digest:", res[0].([]byte))
  } else {
    fmt.Println("Run bindgen -- keccak_digest FAILED")
  }

  bg.Release()
  vm.Release()
  conf.Release()
}
```

Next, build the Golang application with the WasmEdge Golang SDK.

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0-alpha.1
go get github.com/second-state/wasmedge-bindgen@v0.1.12
go build
```

Run the Golang application and it will run the WebAssembly functions embedded in the WasmEdge runtime.

```bash
$ ./bindgen_funcs rust_bindgen_funcs/target/wasm32-wasi/release/rust_bindgen_funcs_lib.wasm
Run bindgen -- create_line: {"points":[{"x":2.5,"y":7.8},{"x":2.5,"y":5.8}],"valid":true,"length":2.0,"desc":"A thin red line"}
Run bindgen -- say: hello bindgen funcs test
Run bindgen -- obfusticate: N dhvpx oebja sbk whzcf bire gur ynml qbt
Run bindgen -- lowest_common_multiple: 246
Run bindgen -- sha3_digest: [87 27 231 209 189 105 251 49 159 10 211 250 15 159 154 181 43 218 26 141 56 199 25 45 60 10 20 163 54 211 195 203]
Run bindgen -- keccak_digest: [126 194 241 200 151 116 227 33 216 99 159 22 107 3 177 169 216 191 114 156 174 193 32 159 246 228 245 133 52 75 55 27]
```

### Example of Embedding A Full WASI Program

Note: You can use the latest Rust compiler to create a standalone WasmEdge application with a `main.rs` functions and then embed it into a Golang application.

Besides functions, the WasmEdge Golang SDK can also [embed standalone WebAssembly applications](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_ReadFile) — i.e. a Rust application with a `main()` function compiled into WebAssembly.

Our [demo Rust application](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_ReadFile/rust_readfile) reads from a file.

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

Use the `rustwasmc` tool to compile the application into WebAssembly.

```bash
cd rust_readfile
rustwasmc build
# The output file will be at `pkg/rust_readfile.wasm`.
```

Or you can compile the application into WebAssembly directly by `cargo`:

```bash
cd rust_readfile
# Need to add the `wasm32-wasi` target.
rustup target add wasm32-wasi
cargo build --release --target=wasm32-wasi
# The output wasm will be at `target/wasm32-wasi/release/rust_readfile.wasm`.
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
  var wasi = vm.GetImportModule(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],     // The args
    os.Environ(),    // The envs
    []string{".:."}, // The mapping directories
  )

  // Instantiate and run WASM "_start" function, which refers to the main() function
  vm.RunWasmFile(os.Args[1], "_start")

  vm.Release()
  conf.Release()
}
```

Next, build the Golang application with the WasmEdge Golang SDK.

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0-alpha.1
go build
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

For more examples, please refer to the [example repository](https://github.com/second-state/WasmEdge-go-examples/).

## WasmEdge-go Basics

In this partition, we will introduce the utilities and concepts of WasmEdge-go APIs and data structures.

### Version

The `Version` related APIs provide developers to check for the installed WasmEdge shared library version.

```go
import "github.com/second-state/WasmEdge-go/wasmedge"

verstr := wasmedge.GetVersion() // Will be `string` of WasmEdge version.
vermajor := wasmedge.GetVersionMajor() // Will be `uint` of WasmEdge major version number.
verminor := wasmedge.GetVersionMinor() // Will be `uint` of WasmEdge minor version number.
verpatch := wasmedge.GetVersionPatch() // Will be `uint` of WasmEdge patch version number.
```

### Logging Settings

The `wasmedge.SetLogErrorLevel()` and `wasmedge.SetLogDebugLevel()` APIs can set the logging system to debug level or error level. By default, the error level is set, and the debug info is hidden.

### Value Types

In WasmEdge-go, the APIs will automatically do the conversion for the built-in types, and implement the data structure for the reference types.

1. Number types: `i32`, `i64`, `f32`, and `f64`

    * Convert the `uint32` and `int32` to `i32` automatically when passing a value into WASM.
    * Convert the `uint64` and `int64` to `i64` automatically when passing a value into WASM.
    * Convert the `uint` and `int` to `i32` automatically when passing a value into WASM in 32-bit system.
    * Convert the `uint` and `int` to `i64` automatically when passing a value into WASM in 64-bit system.
    * Convert the `float32` to `f32` automatically when passing a value into WASM.
    * Convert the `float64` to `f64` automatically when passing a value into WASM.
    * Convert the `i32` from WASM to `int32` when getting a result.
    * Convert the `i64` from WASM to `int64` when getting a result.
    * Convert the `f32` from WASM to `float32` when getting a result.
    * Convert the `f64` from WASM to `float64` when getting a result.

2. Number type: `v128` for the `SIMD` proposal

    Developers should use the `wasmedge.NewV128()` to generate a `v128` value, and use the `wasmedge.GetV128()` to get the value.

    ```go
    val := wasmedge.NewV128(uint64(1234), uint64(5678))
    high, low := val.GetVal()
    // `high` will be uint64(1234), `low` will be uint64(5678)
    ```

3. Reference types: `FuncRef` and `ExternRef` for the `Reference-Types` proposal

    ```go
    var funccxt *wasmedge.Function = ... // Create or get function object.
    funcref := wasmedge.NewFuncRef(funccxt)
    // Create a `FuncRef` with the function object.

    num := 1234
    // `num` is a `int`.
    externref := wasmedge.NewExternRef(&num)
    // Create an `ExternRef` which reference to the `num`.
    num = 5678
    // Modify the `num` to 5678.
    numref := externref.GetRef().(*int)
    // Get the original reference from the `ExternRef`.
    fmt.Println(*numref)
    // Will print `5678`.
    numref.Release()
    // Should call the `Release` method.
    ```

### Results

The `Result` object specifies the execution status. Developers can use the `Error()` function to get the error message.

```go
// Assume that `vm` is a `wasmedge.VM` object.
res, err = vm.Execute(...) // Ignore the detail of parameters.
// Assume that `res, err` are the return values for executing a function with `vm`.
if err != nil {
  fmt.Println("Error message:", err.Error())
}
```

### Contexts And Their Life Cycles

The objects, such as `VM`, `Store`, and `Function`, etc., are composed of `Context`s in the WasmEdge shared library.
All of the contexts can be created by calling the corresponding `New` APIs, developers should also call the corresponding `Release` functions of the contexts to release the resources.
Noticed that it's not necessary to call the `Release` functions for the contexts which are retrieved from other contexts but not created from the `New` APIs.

```go
// Create a Configure.
conf := wasmedge.NewConfigure()
// Release the `conf` immediately.
conf.Release()
```

The details of other contexts will be introduced later.

### WASM Data Structures

The WASM data structures are used for creating instances or can be queried from instance contexts.
The details of instances creation will be introduced in the [Instances](#Instances).

1. Limit

    The `Limit` struct presents the minimum and maximum value data structure.

    ```go
    lim1 := wasmedge.NewLimit(12)
    fmt.Println(lim1.HasMax())
    // Will print `false`.
    fmt.Println(lim1.GetMin())
    // Will print `12`.

    lim2 := wasmedge.NewLimitWithMax(15, 50)
    fmt.Println(lim2.HasMax())
    // Will print `true`.
    fmt.Println(lim2.GetMin())
    // Will print `15`.
    fmt.Println(lim2.GetMax())
    // Will print `50`.
    ```

2. Function type context

    The `FunctionType` is an object holds the function type context and used for the `Function` creation, checking the value types of a `Function` instance, or getting the function type with function name from VM.
    Developers can use the `FunctionType` APIs to get the parameter or return value types information.

    ```go
    functype := wasmedge.NewFunctionType(
      []wasmedge.ValType{
        wasmedge.ValType_ExternRef,
        wasmedge.ValType_I32,
        wasmedge.ValType_I64,
      }, []wasmedge.ValType{
        wasmedge.ValType_F32,
        wasmedge.ValType_F64,
      })

    plen := functype.GetParametersLength()
    // `plen` will be 3.
    rlen := functype.GetReturnsLength()
    // `rlen` will be 2.
    plist := functype.GetParameters()
    // `plist` will be `[]wasmedge.ValType{wasmedge.ValType_ExternRef, wasmedge.ValType_I32, wasmedge.ValType_I64}`.
    rlist := functype.GetReturns()
    // `rlist` will be `[]wasmedge.ValType{wasmedge.ValType_F32, wasmedge.ValType_F64}`.

    functype.Release()
    ```

3. Table type context

    The `TableType` is an object holds the table type context and used for `Table` instance creation or getting information from `Table` instances.

    ```go
    lim := wasmedge.NewLimit(12)
    tabtype := wasmedge.NewTableType(wasmedge.RefType_ExternRef, lim)

    rtype := tabtype.GetRefType()
    // `rtype` will be `wasmedge.RefType_ExternRef`.
    getlim := tabtype.GetLimit()
    // `getlim` will be the same value as `lim`.

    tabtype.Release()
    ```

4. Memory type context

    The `MemoryType` is an object holds the memory type context and used for `Memory` instance creation or getting information from `Memory` instances.

    ```go
    lim := wasmedge.NewLimit(1)
    memtype := wasmedge.NewMemoryType(lim)

    getlim := memtype.GetLimit()
    // `getlim` will be the same value as `lim`.

    memtype.Release()
    ```

5. Global type context

    The `GlobalType` is an object holds the global type context and used for `Global` instance creation or getting information from `Global` instances.

    ```go
    globtype := wasmedge.NewGlobalType(wasmedge.ValType_F64, wasmedge.ValMut_Var)

    vtype := globtype.GetValType()
    // `vtype` will be `wasmedge.ValType_F64`.
    vmut := globtype.GetMutability()
    // `vmut` will be `wasmedge.ValMut_Var`.

    globtype.Release()
    ```

6. Import type context

    The `ImportType` is an object holds the import type context and used for getting the imports information from a [AST Module](#AST-Module).
    Developers can get the external type (`function`, `table`, `memory`, or `global`), import module name, and external name from an `ImportType` object.
    The details about querying `ImportType` objects will be introduced in the [AST Module](#AST-Module).

    ```go
    var ast *wasmedge.AST = ...
    // Assume that `ast` is returned by the `Loader` for the result of loading a WASM file.
    imptypelist := ast.ListImports()
    // Assume that `imptypelist` is an array listed from the `ast` for the imports.

    for i, imptype := range imptypelist {
      exttype := imptype.GetExternalType()
      // The `exttype` must be one of `wasmedge.ExternType_Function`, `wasmedge.ExternType_Table`,
      // wasmedge.ExternType_Memory`, or `wasmedge.ExternType_Global`.

      modname := imptype.GetModuleName()
      extname := imptype.GetExternalName()
      // Get the module name and external name of the imports.

      extval := imptype.GetExternalValue()
      // The `extval` is the type of `interface{}` which indicates one of `*wasmedge.FunctionType`,
      // `*wasmedge.TableType`, `*wasmedge.MemoryType`, or `*wasmedge.GlobalType`.
    }
    ```

7. Export type context

    The `ExportType` is an object holds the export type context is used for getting the exports information from a [AST Module](#AST-Module).
    Developers can get the external type (`function`, `table`, `memory`, or `global`) and external name from an `Export Type` context.
    The details about querying `ExportType` objects will be introduced in the [AST Module](#AST-Module).

    ```go
    var ast *wasmedge.AST = ...
    // Assume that `ast` is returned by the `Loader` for the result of loading a WASM file.
    exptypelist := ast.ListExports()
    // Assume that `exptypelist` is an array listed from the `ast` for the exports.

    for i, exptype := range exptypelist {
      exttype := exptype.GetExternalType()
      // The `exttype` must be one of `wasmedge.ExternType_Function`, `wasmedge.ExternType_Table`,
      // wasmedge.ExternType_Memory`, or `wasmedge.ExternType_Global`.

      extname := exptype.GetExternalName()
      // Get the external name of the exports.

      extval := exptype.GetExternalValue()
      // The `extval` is the type of `interface{}` which indicates one of `*wasmedge.FunctionType`,
      // `*wasmedge.TableType`, `*wasmedge.MemoryType`, or `*wasmedge.GlobalType`.
    }
    ```

### Configurations

The configuration object, `wasmedge.Configure`, manages the configurations for `Loader`, `Validator`, `Executor`, `VM`, and `Compiler`.
Developers can adjust the settings about the proposals, VM host pre-registrations (such as `WASI`), and AOT compiler options, and then apply the `Configure` object to create other runtime objects.

1. Proposals

    WasmEdge supports turning on or off the WebAssembly proposals.
    This configuration is effective in any contexts created with the `Configure` object.

    ```go
    const (
      IMPORT_EXPORT_MUT_GLOBALS         = Proposal(C.WasmEdge_Proposal_ImportExportMutGlobals)
      NON_TRAP_FLOAT_TO_INT_CONVERSIONS = Proposal(C.WasmEdge_Proposal_NonTrapFloatToIntConversions)
      SIGN_EXTENSION_OPERATORS          = Proposal(C.WasmEdge_Proposal_SignExtensionOperators)
      MULTI_VALUE                       = Proposal(C.WasmEdge_Proposal_MultiValue)
      BULK_MEMORY_OPERATIONS            = Proposal(C.WasmEdge_Proposal_BulkMemoryOperations)
      REFERENCE_TYPES                   = Proposal(C.WasmEdge_Proposal_ReferenceTypes)
      SIMD                              = Proposal(C.WasmEdge_Proposal_SIMD)
      TAIL_CALL                         = Proposal(C.WasmEdge_Proposal_TailCall)
      ANNOTATIONS                       = Proposal(C.WasmEdge_Proposal_Annotations)
      MEMORY64                          = Proposal(C.WasmEdge_Proposal_Memory64)
      THREADS                           = Proposal(C.WasmEdge_Proposal_Threads)
      EXCEPTION_HANDLING                = Proposal(C.WasmEdge_Proposal_ExceptionHandling)
      FUNCTION_REFERENCES               = Proposal(C.WasmEdge_Proposal_FunctionReferences)
    )
    ```

    Developers can add or remove the proposals into the `Configure` object.

    ```go
    // By default, the following proposals have turned on initially:
    // * IMPORT_EXPORT_MUT_GLOBALS
    // * NON_TRAP_FLOAT_TO_INT_CONVERSIONS
    // * SIGN_EXTENSION_OPERATORS
    // * MULTI_VALUE
    // * BULK_MEMORY_OPERATIONS
    // * REFERENCE_TYPES
    // * SIMD
    // For the current WasmEdge version, the following proposals are supported:
    // * TAIL_CALL
    // * MULTI_MEMORIES
    // * THREADS
    conf := wasmedge.NewConfigure()
    // Developers can also pass the proposals as parameters:
    // conf := wasmedge.NewConfigure(wasmedge.SIMD, wasmedge.BULK_MEMORY_OPERATIONS)
    conf.AddConfig(wasmedge.SIMD)
    conf.RemoveConfig(wasmedge.REFERENCE_TYPES)
    is_bulkmem := conf.HasConfig(wasmedge.BULK_MEMORY_OPERATIONS)
    // The `is_bulkmem` will be `true`.
    conf.Release()
    ```

2. Host registrations

    This configuration is used for the `VM` context to turn on the `WASI` or `wasmedge_process` supports and only effective in `VM` objects.

    ```go
    const (
      WASI             = HostRegistration(C.WasmEdge_HostRegistration_Wasi)
      WasmEdge_PROCESS = HostRegistration(C.WasmEdge_HostRegistration_WasmEdge_Process)
    )
    ```

    The details will be introduced in the [preregistrations of VM context](###Preregistrations).

    ```go
    conf := wasmedge.NewConfigure()
    // Developers can also pass the proposals as parameters:
    // conf := wasmedge.NewConfigure(wasmedge.WASI)
    conf.AddConfig(wasmedge.WASI)
    conf.Release()
    ```

3. Maximum memory pages

    Developers can limit the page size of memory instances by this configuration.
    When growing the page size of memory instances in WASM execution and exceeding the limited size, the page growing will fail.
    This configuration is only effective in the `Executor` and `VM` objects.

    ```go
    conf := wasmedge.NewConfigure()

    pagesize := conf.GetMaxMemoryPage()
    // By default, the maximum memory page size in each memory instances is 65536.
    conf.SetMaxMemoryPage(1234)
    pagesize := conf.GetMaxMemoryPage()
    // `pagesize` will be 1234.

    conf.Release()
    ```

4. AOT compiler options

    The AOT compiler options configure the behavior about optimization level, output format, dump IR, and generic binary.

    ```go
    const (
      // Disable as many optimizations as possible.
      CompilerOptLevel_O0 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O0)
      // Optimize quickly without destroying debuggability.
      CompilerOptLevel_O1 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O1)
      // Optimize for fast execution as much as possible without triggering significant incremental compile time or code size growth.
      CompilerOptLevel_O2 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O2)
      // Optimize for fast execution as much as possible.
      CompilerOptLevel_O3 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O3)
      // Optimize for small code size as much as possible without triggering significant incremental compile time or execution time slowdowns.
      CompilerOptLevel_Os = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_Os)
      // Optimize for small code size as much as possible.
      CompilerOptLevel_Oz = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_Oz)
    )

    const (
      // Native dynamic library format.
      CompilerOutputFormat_Native = CompilerOutputFormat(C.WasmEdge_CompilerOutputFormat_Native)
      // WebAssembly with AOT compiled codes in custom section.
      CompilerOutputFormat_Wasm = CompilerOutputFormat(C.WasmEdge_CompilerOutputFormat_Wasm)
    )
    ```

    These configurations are only effective in `Compiler` contexts.

    ```go
    conf := wasmedge.NewConfigure()

    // By default, the optimization level is O3.
    conf.SetCompilerOptimizationLevel(wasmedge.CompilerOptLevel_O2)
    // By default, the output format is universal WASM.
    conf.SetCompilerOutputFormat(wasmedge.CompilerOutputFormat_Native)
    // By default, the dump IR is `false`.
    conf.SetCompilerDumpIR(true)
    // By default, the generic binary is `false`.
    conf.SetCompilerGenericBinary(true)

    conf.Release()
    ```

5. Statistics options

    The statistics options configure the behavior about instruction counting, cost measuring, and time measuring in both runtime and AOT compiler.
    These configurations are effective in `Compiler`, `VM`, and `Executor` objects.

    ```go
    conf := wasmedge.NewConfigure()

    // By default, the intruction counting is `false` when running a compiled-WASM or a pure-WASM.
    conf.SetStatisticsInstructionCounting(true)
    // By default, the cost measurement is `false` when running a compiled-WASM or a pure-WASM.
    conf.SetStatisticsTimeMeasuring(true)
    // By default, the time measurement is `false` when running a compiled-WASM or a pure-WASM.
    conf.SetStatisticsCostMeasuring(true)

    conf.Release()
    ```

## WasmEdge VM

In this partition, we will introduce the functions of `wasmedge.VM` object and show examples of executing WASM functions.

### WASM Execution Example With VM Object

The following shows the example of running the WASM for getting the Fibonacci.
This example uses the [fibonacci.wasm](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/tools/wasmedge/examples/fibonacci.wasm), and the corresponding WAT file is at [fibonacci.wat](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/tools/wasmedge/examples/fibonacci.wat).

```wasm
(module
  (export "fib" (func $fib))
  (func $fib (param $n i32) (result i32)
    (if
      (i32.lt_s (get_local $n)(i32.const 2))
      (return (i32.const 1))
    )
    (return
      (i32.add
        (call $fib (i32.sub (get_local $n)(i32.const 2)))
        (call $fib (i32.sub (get_local $n)(i32.const 1)))
      )
    )
  )
)
```

1. Run WASM functions rapidly

    Create a new Go project first:

    ```bash
    mkdir wasmedge_test && cd wasmedge_test
    go mod init wasmedge_test
    ```

    Assume that the WASM file [`fibonacci.wasm`](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/tools/wasmedge/examples/fibonacci.wasm) is copied into the current `wasmedge_test` directory, and create and edit the Go file `main.go` as following:

    ```go
    package main

    import (
      "fmt"

      "github.com/second-state/WasmEdge-go/wasmedge"
    )

    func main() {
      // Set the logging level.
      wasmedge.SetLogErrorLevel()

      // Create the configure context and add the WASI support.
      // This step is not necessary unless you need WASI support.
      conf := wasmedge.NewConfigure(wasmedge.WASI)
      // Create VM with the configure.
      vm := wasmedge.NewVMWithConfig(conf)

      res, err := vm.RunWasmFile("fibonacci.wasm", "fib", uint32(21))
      if err == nil {
        fmt.Println("Get fibonacci[21]:", res[0].(int32))
      } else {
        fmt.Println("Run failed:", err.Error())
      }

      vm.Release()
      conf.Release()
    }
    ```

    Then you can build and run the Golang application with the WasmEdge Golang SDK: (the 21 Fibonacci number is 17711 in 0-based index)

    ```bash
    $ go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0-alpha.1
    $ go build
    $ ./wasmedge_test
    Get fibonacci[21]: 17711
    ```

2. Instantiate and run WASM functions manually

    Besides the above example, developers can run the WASM functions step-by-step with `VM` object APIs:

    ```go
    package main

    import (
      "fmt"

      "github.com/second-state/WasmEdge-go/wasmedge"
    )

    func main() {
      // Set the logging level.
      wasmedge.SetLogErrorLevel()

      // Create VM.
      vm := wasmedge.NewVM()
      var err error
      var res []interface{}

      // Step 1: Load WASM file.
      err = vm.LoadWasmFile("fibonacci.wasm")
      if err != nil {
        fmt.Println("Load WASM from file FAILED:", err.Error())
        return
      }

      // Step 2: Validate the WASM module.
      err = vm.Validate()
      if err != nil {
        fmt.Println("Validation FAILED:", err.Error())
        return
      }

      // Step 3: Instantiate the WASM module.
      err = vm.Instantiate()
      // Developers can load, validate, and instantiate another WASM module
      // to replace the instantiated one. In this case, the old module will
      // be cleared, but the registered modules are still kept.
      if err != nil {
        fmt.Println("Instantiation FAILED:", err.Error())
        return
      }

      // Step 4: Execute WASM functions. Parameters: (funcname, args...)
      res, err = vm.Execute("fib", uint32(25))
      // Developers can execute functions repeatedly after instantiation.
      if err == nil {
        fmt.Println("Get fibonacci[25]:", res[0].(int32))
      } else {
        fmt.Println("Run failed:", err.Error())
      }

      vm.Release()
    }
    ```

    Then you can build and run: (the 25th Fibonacci number is 121393 in 0-based index)

    ```bash
    $ go build
    $ ./wasmedge_test
    Get fibonacci[25]: 121393
    ```

    The following graph explains the status of the `VM` object.

    ```text
                           |========================|
                  |------->|      VM: Initiated     |
                  |        |========================|
                  |                    |
                  |                 LoadWasm
                  |                    |
                  |                    v
                  |        |========================|
                  |--------|       VM: Loaded       |<-------|
                  |        |========================|        |
                  |              |            ^              |
                  |         Validate          |              |
              Cleanup            |          LoadWasm         |
                  |              v            |            LoadWasm
                  |        |========================|        |
                  |--------|      VM: Validated     |        |
                  |        |========================|        |
                  |              |            ^              |
                  |      Instantiate          |              |
                  |              |          RegisterModule   |
                  |              v            |              |
                  |        |========================|        |
                  |--------|    VM: Instantiated    |--------|
                           |========================|
                                 |            ^
                                 |            |
                                 --------------
                    Instantiate, Execute, ExecuteRegistered,
                    ExecuteBindgen, ExecuteBindgenRegistered
    ```

    The status of the `VM` context would be `Inited` when created.
    After loading WASM successfully, the status will be `Loaded`.
    After validating WASM successfully, the status will be `Validated`.
    After instantiating WASM successfully, the status will be `Instantiated`, and developers can invoke functions.
    Developers can register WASM or import objects in any status, but they should instantiate WASM again.
    Developers can also load WASM in any status, and they should validate and instantiate the WASM module before function invocation.
    When in the `Instantiated` status, developers can instantiate the WASM module again to reset the old WASM runtime structures.

### VM Creations

The `VM` creation APIs accepts the `Configure` object and the `Store` object.
Noticed that if the `VM` created with the outside `Store` object, the `VM` will execute WASM on that `Store` object. If the `Store` object is set into multiple `VM` objects, it may causes data conflict when in execution.
The details of the `Store` object will be introduced in [Store](#Store).

```go
conf := wasmedge.NewConfigure()
store := wasmedge.NewStore()

// Create a VM with default configure and store.
vm := wasmedge.NewVM()
vm.Release()

// Create a VM with the specified configure and default store.
vm = wasmedge.NewVMWithConfig(conf)
vm.Release()

// Create a VM with the default configure and specified store.
vm = wasmedge.NewVMWithStore(store)
vm.Release()

// Create a VM with the specified configure and store.
vm = wasmedge.NewVMWithConfigAndStore(conf, store)
vm.Release()

conf.Release()
store.Release()
```

### Preregistrations

WasmEdge provides the following built-in pre-registrations.

1. [WASI (WebAssembly System Interface)](https://github.com/WebAssembly/WASI)

    Developers can turn on the WASI support for VM in the `Configure` object.

    ```go
    conf := wasmedge.NewConfigure(wasmedge.WASI)
    // Or you can set the `wasmedge.WASI` into the configure object through `(*Configure).AddConfig`.
    vm := wasmedge.NewVMWithConfig(conf)
    vm.Release()

    // The following API can retrieve the pre-registration import objects from the VM object.
    // This API will return `nil` if the corresponding pre-registration is not set into the configuration.
    wasiconf := conf.GetImportModule(wasmedge.WASI)
    // Initialize the WASI.
    wasiconf.InitWasi(/* ... ignored */)

    conf.Release()
    ```

    And also can create the WASI import object from API. The details will be introduced in the [Host Functions](#Host-Functions) and the [Host Module Registrations](#Host-Module-Registrations).

2. [WasmEdge_Process](https://crates.io/crates/wasmedge_process_interface)

    This pre-registration is for the process interface for WasmEdge on `Rust` sources.
    After turning on this pre-registration, the VM will support the `wasmedge_process` host functions.

    ```go
    conf := wasmedge.NewConfigure(wasmedge.WasmEdge_PROCESS)
    vm := wasmedge.NewVMWithConfig(conf)
    vm.Release()
    
    // The following API can retrieve the pre-registration import objects from the VM object.
    // This API will return `nil` if the corresponding pre-registration is not set into the configuration.
    procconf := conf.GetImportModule(wasmedge.WasmEdge_PROCESS)
    // Initialize the WasmEdge_Process.
    procconf.InitWasmEdgeProcess(/* ... ignored */)

    conf.Release()
    ```

    And also can create the WasmEdge_Process import object from API. The details will be introduced in the [Host Functions](#Host-Functions) and the [Host Module Registrations](#Host-Module-Registrations).

### Host Module Registrations

[Host functions](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) are functions outside WebAssembly and passed to WASM modules as imports.
In WasmEdge-go, the host functions are composed into host modules as `Module` objects with module names.
Please refer to the [Host Functions in WasmEdge Runtime](#Host-Functions) for the details.
In this chapter, we show the example for registering the host modules into a `VM` object.

```go
vm := wasmedge.NewVM()
// You can also create and register the WASI host modules by this API.
wasiobj := wasmedge.NewWasiModule(/* ... ignored ... */)

res := vm.RegisterModule(wasiobj)
// The result status should be checked.

vm.Release()
// The created import objects should be released.
wasiobj.Release()
```

### WASM Registrations And Executions

In WebAssembly, the instances in WASM modules can be exported and can be imported by other WASM modules.
WasmEdge VM provides APIs for developers to register and export any WASM modules, and execute the functions or host functions in the registered WASM modules.

1. Register the WASM modules with exported module names

    Unless the import objects have already contained the module names, every WASM module should be named uniquely when registering. The following shows the example.

    Create a new Go project first:

    ```bash
    mkdir wasmedge_test && cd wasmedge_test
    go mod init wasmedge_test
    ```

    Assume that the WASM file [`fibonacci.wasm`](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/tools/wasmedge/examples/fibonacci.wasm) is copied into the current directory.
    Then create and edit the Go file `main.go` as following:

    ```go
    package main

    import "github.com/second-state/WasmEdge-go/wasmedge"

    func main() {
      // Create VM.
      vm := wasmedge.NewVM()

      var err error
      err = vm.RegisterWasmFile("module_name", "fibonacci.wasm")
      // Developers can register the WASM module from `[]byte` with the
      // `(*VM).RegisterWasmBuffer` function, or from `AST` object with
      // the `(*VM).RegisterAST` function.
      // The result status should be checked. The error will occur if the
      // WASM module instantiation failed or the module name conflicts.

      vm.Release()
    }
    ```

2. Execute the functions in registered WASM modules

    Edit the Go file `main.go` as following:

    ```go
    package main

    import (
      "fmt"

      "github.com/second-state/WasmEdge-go/wasmedge"
    )

    func main() {
      // Create VM.
      vm := wasmedge.NewVM()

      var res []interface{}
      var err error
      // Register the WASM module from file into VM with the module name "mod".
      err = vm.RegisterWasmFile("mod", "fibonacci.wasm")
      // Developers can register the WASM module from `[]byte` with the
      // `(*VM).RegisterWasmBuffer` function, or from `AST` object with
      // the `(*VM).RegisterAST` function.
      if err != nil {
        fmt.Println("WASM registration failed:", err.Error())
        return
      }
      // The function "fib" in the "fibonacci.wasm" was exported with the module
      // name "mod". As the same as host functions, other modules can import the
      // function `"mod" "fib"`.

      // Execute WASM functions in registered modules.
      // Unlike the execution of functions, the registered functions can be
      // invoked without `(*VM).Instantiate` because the WASM module was
      // instantiated when registering.
      // Developers can also invoke the host functions directly with this API.
      res, err = vm.ExecuteRegistered("mod", "fib", int32(25))
      if err == nil {
        fmt.Println("Get fibonacci[25]:", res[0].(int32))
      } else {
        fmt.Println("Run failed:", err.Error())
      }

      vm.Release()
    }
    ```

    Then you can build and run: (the 25th Fibonacci number is 121393 in 0-based index)

    ```bash
    $ go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0-alpha.1
    $ go build
    $ ./wasmedge_test
    Get fibonacci[25]: 121393
    ```

### Instance Tracing

Sometimes the developers may have requirements to get the instances of the WASM runtime.
The `VM` object supplies the APIs to retrieve the instances.

1. Store

    If the `VM` object is created without assigning a `Store` object, the `VM` context will allocate and own a `Store`.

    ```go
    vm := wasmedge.NewVM()
    store := vm.GetStore()
    // The object should __NOT__ be deleted by calling `(*Store).Release`.
    vm.Release()
    ```

    Developers can also create the `VM` object with a `Store` object.
    In this case, developers should guarantee that the `Store` object cannot be released before the `VM` object.
    Please refer to the [Store Objects](#Store) for the details about the `Store` APIs.

    ```go
    store := wasmedge.NewStore()
    vm := wasmedge.NewVMWithStore(store)

    storemock := vm.GetStore()
    // The internal store context of the `store` and the `storemock` are the same.

    vm.Release()
    store.Release()
    ```

2. List exported functions

    After the WASM module instantiation, developers can use the `(*VM).Execute` function to invoke the exported WASM functions. For this purpose, developers may need information about the exported WASM function list.
    Please refer to the [Instances in runtime](#Instances) for the details about the function types.

    Assume that a new Go project is created as following:

    ```bash
    mkdir wasmedge_test && cd wasmedge_test
    go mod init wasmedge_test
    ```

    Then assume that the WASM file [`fibonacci.wasm`](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/tools/wasmedge/examples/fibonacci.wasm) is copied into the current directory, and create and edit a Go file `main.go`:

    ```go
    package main

    import (
      "fmt"

      "github.com/second-state/WasmEdge-go/wasmedge"
    )

    func main() {
      // Create VM.
      vm := wasmedge.NewVM()

      // Step 1: Load WASM file.
      err := vm.LoadWasmFile("fibonacci.wasm")
      if err != nil {
        fmt.Println("Load WASM from file FAILED:", err.Error())
        return
      }

      // Step 2: Validate the WASM module.
      err = vm.Validate()
      if err != nil {
        fmt.Println("Validation FAILED:", err.Error())
        return
      }

      // Step 3: Instantiate the WASM module.
      err = vm.Instantiate()
      if err != nil {
        fmt.Println("Instantiation FAILED:", err.Error())
        return
      }

      // List the exported functions for the names and function types.
      funcnames, functypes := vm.GetFunctionList()
      for _, fname := range funcnames {
        fmt.Println("Exported function name:", fname)
      }
      for _, ftype := range functypes {
        // `ftype` is the `FunctionType` object of the same index in the `funcnames` array.
        // Developers should __NOT__ call the `ftype.Release()`.
      }

      vm.Release()
    }
    ```

    Then you can build and run: (the only exported function in `fibonacci.wasm` is `fib`)

    ```bash
    $ go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0-alpha.1
    $ go build
    $ ./wasmedge_test
    Exported function name: fib
    ```

    If developers want to get the exported function names in the registered WASM modules, please retrieve the `Store` object from the `VM` object and refer to the APIs of [Store Contexts](#Store) to list the registered functions by the module name.

3. Get function types

    The `VM` object provides APIs to find the function type by function name.
    Please refer to the [Instances in runtime](#Instances) for the details about the function types.

    ```go
    // Assume that a WASM module is instantiated in `vm` which is a `wasmedge.VM` object.
    functype := vm.GetFunctionType("fib")
    // Developers can get the function types of functions in the registered modules via the 
    // `(*VM).GetFunctionTypeRegistered` API with the functon name and the module name.
    // If the function is not found, these APIs will return `nil`.
    // Developers should __NOT__ call the `(*FunctionType).Release` function of the returned object.
    ```

4. Get the active module

    After the WASM module instantiation, an anonymous module is instantiated and owned by the `VM` object.
    Developers may need to retrieve it to get the instances beyond the module.
    Then developers can use the `(*VM).GetActiveModule()` API to get that anonymous module instance.
    Please refer to the [Module instance](#instances) for the details about the module instance APIs.

    ```go
    // Assume that a WASM module is instantiated in `vm` which is a `wasmedge.VM` object.
    mod := vm.GetActiveModule()
    // If there's no WASM module instantiated, this API will return `nil`.
    // Developers should __NOT__ call the `(*Module).Release` function of the returned module instance.
    ```

## WasmEdge Runtime

In this partition, we will introduce the objects of WasmEdge runtime manually.

### WASM Execution Example Step-By-Step

Besides the WASM execution through the [`VM` object](#WasmEdge-VM) rapidly, developers can execute the WASM functions or instantiate WASM modules step-by-step with the `Loader`, `Validator`, `Executor`, and `Store` objects.

Assume that a new Go project is created as following:

```bash
mkdir wasmedge_test && cd wasmedge_test
go mod init wasmedge_test
```

Then assume that the WASM file [`fibonacci.wasm`](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/tools/wasmedge/examples/fibonacci.wasm) is copied into the current directory, and create and edit a Go file `main.go`:

```go
package main

import (
  "fmt"

  "github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
  // Set the logging level to debug to print the statistics info.
  wasmedge.SetLogDebugLevel()
  // Create the configure object. This is not necessary if developers use the default configuration.
  conf := wasmedge.NewConfigure()
  // Turn on the runtime instruction counting and time measuring.
  conf.SetStatisticsInstructionCounting(true)
  conf.SetStatisticsTimeMeasuring(true)
  // Create the statistics object. This is not necessary if the statistics in runtime is not needed.
  stat := wasmedge.NewStatistics()
  // Create the store object. The store object is the WASM runtime structure core.
  store := wasmedge.NewStore()

  var err error
  var res []interface{}
  var ast *wasmedge.AST
  var mod *wasmedge.Module

  // Create the loader object.
  // For loader creation with default configuration, you can use `wasmedge.NewLoader()` instead.
  loader := wasmedge.NewLoaderWithConfig(conf)
  // Create the validator object.
  // For validator creation with default configuration, you can use `wasmedge.NewValidator()` instead.
  validator := wasmedge.NewValidatorWithConfig(conf)
  // Create the executor object.
  // For executor creation with default configuration and without statistics, you can use `wasmedge.NewExecutor()` instead.
  executor := wasmedge.NewExecutorWithConfigAndStatistics(conf, stat)

  // Load the WASM file or the compiled-WASM file and convert into the AST module object.
  ast, err = loader.LoadFile("fibonacci.wasm")
  if err != nil {
    fmt.Println("Load WASM from file FAILED:", err.Error())
    return
  }
  // Validate the WASM module.
  err = validator.Validate(ast)
  if err != nil {
    fmt.Println("Validation FAILED:", err.Error())
    return
  }
  // Instantiate the WASM module and get the output module instance.
  mod, err = executor.Instantiate(store, ast)
  if err != nil {
    fmt.Println("Instantiation FAILED:", err.Error())
    return
  }

  // Try to list the exported functions of the instantiated WASM module.
  funcnames := mod.ListFunction()
  for _, fname := range funcnames {
    fmt.Println("Exported function name:", fname)
  }

  // Invoke the WASM function.
  funcinst := mod.FindFunction("fib")
  if funcinst == nil {
    fmt.Println("Run FAILED: Function name `fib` not found")
    return
  }
  res, err = executor.Invoke(store, funcinst, int32(30))
  if err == nil {
    fmt.Println("Get fibonacci[30]:", res[0].(int32))
  } else {
    fmt.Println("Run FAILED:", err.Error())
  }

  // Resources deallocations.
  conf.Release()
  stat.Release()
  ast.Release()
  loader.Release()
  validator.Release()
  executor.Release()
  store.Release()
  mod.Release()
}
```

Then you can build and run: (the 18th Fibonacci number is 1346269 in 30-based index)

```bash
$ go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0-alpha.1
$ go build
$ ./wasmedge_test
Exported function name: fib
[2021-11-24 18:53:01.451] [debug]  Execution succeeded.
[2021-11-24 18:53:01.452] [debug]
 ====================  Statistics  ====================
 Total execution time: 556372295 ns
 Wasm instructions execution time: 556372295 ns
 Host functions execution time: 0 ns
 Executed wasm instructions count: 28271634
 Gas costs: 0
 Instructions per second: 50814237
Get fibonacci[30]: 1346269
```

### Loader

The `Loader` object loads the WASM binary from files or buffers.
Both the WASM and the compiled-WASM from the [WasmEdge AOT Compiler](#WasmEdge-AOT-Compiler) are supported.

```go
var buf []byte
// ... Read the WASM code to the `buf`.

// Developers can adjust settings in the configure object.
conf := wasmedge.NewConfigure()
// Create the loader object.
// For loader creation with default configuration, you can use `wasmedge.NewLoader()` instead.
loader := wasmedge.NewLoaderWithConfig(conf)
conf.Release()

// Load WASM or compiled-WASM from the file.
ast, err := loader.LoadFile("fibonacci.wasm")
if err != nil {
  fmt.Println("Load WASM from file FAILED:", err.Error())
} else {
  // The output AST object should be released.
  ast.Release()
}

// Load WASM or compiled-WASM from the buffer
ast, err = loader.LoadBuffer(buf)
if err != nil {
  fmt.Println("Load WASM from buffer FAILED:", err.Error())
} else {
  // The output AST object should be released.
  ast.Release()   
}

loader.Release()
```

### Validator

The `Validator` object can validate the WASM module.
Every WASM module should be validated before instantiation.

```go
// ...
// Assume that the `ast` is the output `*wasmedge.AST` object from the loader context.
// Assume that the `conf` is the `*wasmedge.Configure` object.

// Create the validator context.
// For validator creation with default configuration, you can use `wasmedge.NewValidator()` instead.
validator := wasmedge.NewValidatorWithConfig(conf)

err := validator.Validate(ast)
if err != nil {
  fmt.Println("Validation FAILED:", err.Error())
}

validator.Release()
```

### Executor

The `Executor` object is the executor for both WASM and compiled-WASM.
This object should work base on the `Store` object. For the details of the `Store` object, please refer to the [next chapter](#Store).

1. Instantiate and register an `AST` object as a named `Module` instance

    As the same of [registering host modules](#host-module-registrations) or [importing WASM modules](#wasm-registrations-and-executions) in `VM` objects, developers can instantiate and register an `AST` objects into the `Store` context as a named `Module` instance by the `Executor` APIs.
    After the registration, the result `Module` instance is exported with the given module name and can be linked when instantiating another module.
    For the details about the `Module` instances APIs, please refer to the [Instances](#instances).

    ```go
    // ...
    // Assume that the `ast` is the output `*wasmedge.AST` object from the loader
    // and has passed the validation.
    // Assume that the `conf` is the `*wasmedge.Configure` object.

    // Create the statistics object. This step is not necessary if the statistics
    // is not needed.
    stat := wasmedge.NewStatistics()
    // Create the executor object.
    // For executor creation with default configuration and without statistics,
    // you can use `wasmedge.NewExecutor()` instead.
    executor := wasmedge.NewExecutorWithConfigAndStatistics(conf, stat)
    // Create the store object. The store is the WASM runtime structure core.
    store := wasmedge.NewStore()

    // Register the loaded WASM `ast` into store with the export module name "mod".
    mod, res := executor.Register(store, ast, "mod")
    if err != nil {
      fmt.Println("WASM registration FAILED:", err.Error())
      return
    }

    // ...

    // Resources deallocations.
    executor.Release()
    stat.Release()
    store.Release()
    mod.Release()
    ```

2. Register an existing `Module` instance and export the module name

    Besides instantiating and registering an `AST` object, developers can register an existing `Module` instance into the store with exporting the module name (which is in the `Module` instance already).
    This case occurs when developers create a `Module` instance for the host functions and want to register it for linking.
    For the details about the construction of host functions in `Module` instances, please refer to the [Host Functions](#host-functions).

    ```go
    // ...
    // Assume that the `ast` is the output `*wasmedge.AST` object from the loader
    // and has passed the validation.
    // Assume that the `conf` is the `*wasmedge.Configure` object.

    // Create the statistics object. This step is not necessary if the statistics
    // is not needed.
    stat := wasmedge.NewStatistics()
    // Create the executor object.
    // For executor creation with default configuration and without statistics,
    // you can use `wasmedge.NewExecutor()` instead.
    executor := wasmedge.NewExecutorWithConfigAndStatistics(conf, stat)
    // Create the store object. The store is the WASM runtime structure core.
    store := wasmedge.NewStore()

    // Create a module instance for host functions.
    mod := wasmedge.NewModule("mod")
    // ...
    // Create and add the host functions, tables, memories, and globals into the module instance.
    // ...

    // Register the module instance into store with the exported module name.
    // The export module name is in the module instance already.
    res := executor.RegisterImport(store, mod)
    if err != nil {
      fmt.Println("WASM registration FAILED:", err.Error())
      return
    }

    // ...

    // Resources deallocations.
    executor.Release()
    stat.Release()
    store.Release()
    mod.Release()
    ```

3. Instantiate an `AST` object to an anonymous `Module` instance

    WASM or compiled-WASM modules should be instantiated before the function invocation.
    Before instantiating a WASM module, please check the [import section](https://webassembly.github.io/spec/core/syntax/modules.html#syntax-import) for ensuring the imports are registered into the `Store` object for linking.

    ```go
    // ...
    // Assume that the `ast` is the output `*wasmedge.AST` object from the loader
    // and has passed the validation.
    // Assume that the `conf` is the `*wasmedge.Configure` object.

    // Create the statistics object. This step is not necessary if the statistics
    // is not needed.
    stat := wasmedge.NewStatistics()
    // Create the executor object.
    // For executor creation with default configuration and without statistics,
    // you can use `wasmedge.NewExecutor()` instead.
    executor := wasmedge.NewExecutorWithConfigAndStatistics(conf, stat)
    // Create the store object. The store is the WASM runtime structure core.
    store := wasmedge.NewStore()

    // Instantiate the WASM module.
    mod, err := executor.Instantiate(stpre, ast)
    if err != nil {
      fmt.Println("WASM instantiation FAILED:", err.Error())
      return
    }

    executor.Release()
    stat.Release()
    store.Release()
    mod.Release()
    ```

4. Invoke functions

    After registering or instantiating and get the result `Module` instance, developers can retrieve the exported `Function` instances from the `Module` instance for invocation.
    For the details about the `Module` instances APIs, please refer to the [Instances](#instances).
    Please refer to the [example above](#wasm-execution-example-step-by-step) for the `Function` instance invocation with the `(*Executor).Invoke` API.

### AST Module

The `AST` object presents the loaded structure from a WASM file or buffer. Developer will get this object after loading a WASM file or buffer from [Loader](#Loader).
Before instantiation, developers can also query the imports and exports of an `AST` object.

```go
ast := ...
// Assume that a WASM is loaded into an `*wasmedge.AST` object from loader.

// List the imports.
imports := ast.ListImports()
for _, import := range imports {
  fmt.Println("Import:", import.GetModuleName(), import.GetExternalName())
}

// List the exports.
exports := ast.ListExports()
for _, export := range exports {
  fmt.Println("Export:", export.GetExternalName())
}

ast.Release()
```

### Store

[Store](https://webassembly.github.io/spec/core/exec/runtime.html#store) is the runtime structure for the representation of all global state that can be manipulated by WebAssembly programs.
The `Store` object in WasmEdge is an object to provide the instance exporting and importing when instantiating WASM modules.
Developers can retrieve the named modules from the `Store` context.

```go
store := wasmedge.NewStore()

// ...
// Register a WASM module via the executor object.
// ...

// Try to list the registered WASM modules.
modnames := store.ListModule()
// ...

// Find named module by name.
mod := store.FindModule("module")
// If the module with name not found, the `mod` will be `nil`.

store.Release()
```

### Instances

The instances are the runtime structures of WASM. Developers can retrieve the `Module` instances from the `Store` contexts, and retrieve the other instances from the `Module` instances.
A single instance can be allocated by its creation function. Developers can construct instances into an `Module` instance for registration. Please refer to the [Host Functions](#host-functions) for details.
The instances created by their creation functions should be destroyed by developers, EXCEPT they are added into an `Module` instance.

1. Module instance

    After instantiating or registering an `AST` object, developers will get a `Module` instance as the result, and have the responsibility to release it when not in use.
    A `Module` instance can also be created for the host module. Please refer to the [host function](#host-functions) for the details.
    `Module` instance provides APIs to list and find the exported instances in the module.

    ```go
    // ...
    // Instantiate a WASM module via the executor object and get the `mod` as the output module instance.
    // ...

    // List the exported instance of the instantiated WASM module.
    // Take the function instances for example here.
    funcnames := mod.ListFunction()

    // Try to find the exported instance of the instantiated WASM module.
    // Take the function instances for example here.
    funcinst := mod.FindFunction("fib")
    // `funcinst` will be `nil` if the function not found.
    // The returned instance is owned by the module instance and should __NOT__ be released.
    ```

2. Function instance

    [Host functions](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) are functions outside WebAssembly and passed to WASM modules as imports.
    In WasmEdge, developers can create the `Function` objects for host functions and add them into an `Module` instance for registering into a `VM` or a `Store`.
    Developers can retrieve the `Function Type` from the `Function` objects through the API.
    For the details of the `Host Function` guide, please refer to the [next chapter](#host-functions).

    ```go
    funcobj := ...
    // `funcobj` is the `*wasmedge.Function` retrieved from the module instance.
    functype := funcobj.GetFunctionType()
    // The `funcobj` retrieved from the module instance should __NOT__ be released.
    // The `functype` retrieved from the `funcobj` should __NOT__ be released.

    // For the function object creation, please refer to the `Host Function` guide.
    ```

3. Table instance

    In WasmEdge, developers can create the `Table` objects and add them into an `ImportObject` object for registering into a `VM` or a `Store`.
    The `Table` objects supply APIs to control the data in table instances.

    ```go
    lim := wasmedge.NewLimitWithMax(10, 20)
    // Create the table type with limit and the `FuncRef` element type.
    tabtype := wasmedge.NewTableType(wasmedge.RefType_FuncRef, lim)
    // Create the table instance with table type.
    tabinst := wasmedge.NewTable(tabtype)
    // Delete the table type.
    tabtype.Release()

    gottabtype := tabinst.GetTableType()
    // The `gottabtype` got from table instance is owned by the `tabinst`
    // and should __NOT__ be released.
    reftype := gottabtype.GetRefType()
    // The `reftype` will be `wasmedge.RefType_FuncRef`.

    var gotdata interface{}
    data := wasmedge.NewFuncRef(5)
    err := tabinst.SetData(data, 3)
    // Set the function index 5 to the table[3].

    // The following line will get an "out of bounds table access" error
    // because the position (13) is out of the table size (10):
    //   err = tabinst.SetData(data, 13)

    gotdata, err = tabinst.GetData(3)
    // Get the FuncRef value of the table[3].

    // The following line will get an "out of bounds table access" error
    // because the position (13) is out of the table size (10):
    //   gotdata, err = tabinst.GetData(13)

    tabsize := tabinst.GetSize()
    // `tabsize` will be 10.
    err = tabinst.Grow(6)
    // Grow the table size of 6, the table size will be 16.

    // The following line will get an "out of bounds table access" error
    // because the size (16 + 6) will reach the table limit (20):
    //   err = tabinst.Grow(6)

    tabinst.Release()
    ```

4. Memory instance

    In WasmEdge, developers can create the `Memory` objects and add them into an `ImportObject` object for registering into a `VM` or a `Store`.
    The `Memory` objects supply APIs to control the data in memory instances.

    ```go
    lim := wasmedge.NewLimitWithMax(1, 5)
    // Create the memory type with limit. The memory page size is 64KiB.
    memtype := wasmedge.NewMemoryType(lim)
    // Create the memory instance with memory type.
    meminst := wasmedge.NewMemory(memtype)
    // Delete the memory type.
    memtype.Release()

    data := []byte("A quick brown fox jumps over the lazy dog")
    err := meminst.SetData(data, 0x1000, 10)
    // Set the data[0:9] to the memory[4096:4105].

    // The following line will get an "out of bounds memory access" error
    // because [65535:65544] is out of 1 page size (65536):
    //   err = meminst.SetData(data, 0xFFFF, 10)

    var gotdata []byte
    gotdata, err = meminst.GetData(0x1000, 10)
    // Get the memory[4096:4105]. The `gotdata` will be `[]byte("A quick br").
    // The following line will get an "out of bounds memory access" error
    // because [65535:65544] is out of 1 page size (65536):
    //   gotdata, err = meminst.Getdata(0xFFFF, 10)

    pagesize := meminst.GetPageSize()
    // `pagesize` will be 1.
    err = meminst.GrowPage(2)
    // Grow the page size of 2, the page size of the memory instance will be 3.

    // The following line will get an "out of bounds memory access" error
    // because the size (3 + 3) will reach the memory limit (5):
    //   err = meminst.GetPageSize(3)

    meminst.Release()
    ```

5. Global instance

    In WasmEdge, developers can create the `Global` objects and add them into an `ImportObject` object for registering into a `VM` or a `Store`.
    The `Global` objects supply APIs to control the value in global instances.

    ```go
    // Create the global type with value type and mutation.
    globtype := wasmedge.NewGlobalType(wasmedge.ValType_I64, wasmedge.ValMut_Var)
    // Create the global instance with value and global type.
    globinst := wasmedge.NewGlobal(globtype, uint64(1000))
    // Delete the global type.
    globtype.Release()

    gotglobtype := globinst.GetGlobalType()
    // The `gotglobtype` got from global instance is owned by the `globinst`
    // and should __NOT__ be released.
    valtype := gotglobtype.GetValType()
    // The `valtype` will be `wasmedge.ValType_I64`.
    valmut := gotglobtype.GetMutability()
    // The `valmut` will be `wasmedge.ValMut_Var`.

    globinst.SetValue(uint64(888))
    // Set the value u64(888) to the global.
    // This function will do nothing if the value type mismatched or the
    // global mutability is `wasmedge.ValMut_Const`.
    gotval := globinst.GetValue()
    // The `gotbal` will be `interface{}` which the type is `uint64` and
    // the value is 888.

    globinst.Release()
    ```

### Host Functions

[Host functions](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) are functions outside WebAssembly and passed to WASM modules as imports.
In WasmEdge-go, developers can create the `Function`, `Memory`, `Table`, and `Global` objects and add them into an `ImportObject` object for registering into a `VM` or a `Store`.

1. Host function allocation

    Developers can define Go functions with the following function signature as the host function body:

    ```go
    type hostFunctionSignature func(
        data interface{}, mem *Memory, params []interface{}) ([]interface{}, Result)
    ```

    The example of an `add` host function to add 2 `i32` values:

    ```go
    func host_add(data interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
      // add: i32, i32 -> i32
      res := params[0].(int32) + params[1].(int32)

      // Set the returns
      returns := make([]interface{}, 1)
      returns[0] = res

      // Return
      return returns, wasmedge.Result_Success
    }
    ```

    Then developers can create `Function` object with the host function body and function type:

    ```go
    // Create a function type: {i32, i32} -> {i32}.
    functype := wasmedge.NewFunctionType(
      []wasmedge.ValType{wasmedge.ValType_I32, wasmedge.ValType_I32},
      []wasmedge.ValType{wasmedge.ValType_I32},
    )

    // Create a function context with the function type and host function body.
    // The third parameter is the pointer to the additional data.
    // Developers should guarantee the life cycle of the data, and it can be
    // `nil` if the external data is not needed.
    // The last parameter can be 0 if developers do not need the cost measuring.
    func_add := wasmedge.NewFunction(functype, host_add, nil, 0)

    // If the function object is not added into an module instance object, it should be released.
    func_add.Release()
    functype.Release()
    ```

2. Construct a module instance with host instances

    Besides creating a `Module` instance by registering or instantiating a WASM module, developers can create a `Module` instance with a module name and add the `Function`, `Memory`, `Table`, and `Global` instances into it with their exporting names.

    ```go
    // Host function body definition.
    func host_add(data interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
      // add: i32, i32 -> i32
      res := params[0].(int32) + params[1].(int32)

      // Set the returns
      returns := make([]interface{}, 1)
      returns[0] = res

      // Return
      return returns, wasmedge.Result_Success
    }

    // Create a module instance with the module name "module".
    mod := wasmedge.NewModule("module")

    // Create and add a function instance into the module instance with export name "add".
    functype := wasmedge.NewFunctionType(
      []wasmedge.ValType{wasmedge.ValType_I32, wasmedge.ValType_I32},
      []wasmedge.ValType{wasmedge.ValType_I32},
    )
    hostfunc := wasmedge.NewFunction(functype, host_add, nil, 0)
    functype.Release()
    mod.AddFunction("add", hostfunc)

    // Create and add a table instance into the module instance with export name "table".
    tabtype := wasmedge.NewTableType(wasmedge.RefType_FuncRef ,wasmedge.NewLimitWithMax(10, 20))
    hosttab := wasmedge.NewTable(tabtype)
    tabtype.Release()
    mod.AddTable("table", hosttab)

    // Create and add a memory instance into the module instance with export name "memory".
    memtype := wasmedge.NewMemoryType(wasmedge.NewLimitWithMax(1, 2))
    hostmem := wasmedge.NewMemory(memtype)
    memtype.Release()
    mod.AddMemory("memory", hostmem)

    // Create and add a global instance into the module instance with export name "global".
    globtype := wasmedge.NewGlobalType(wasmedge.ValType_I32, wasmedge.ValMut_Var)
    hostglob := wasmedge.NewGlobal(globtype, uint32(666))
    globtype.Release()
    mod.AddGlobal("global", hostglob)

    // The module instances should be released.
    // Developers should __NOT__ release the instances added into the module instance objects.
    mod.Release()
    ```

3. Specified module instance

    `wasmedge.NewWasiModule()` API can create and initialize the `WASI` module instance.
    `wasmedge.NewWasmEdgeProcessModule()` API can create and initialize the `wasmedge_process` module instance.
    Developers can create these module instance objects and register them into the `Store` or `VM` objects rather than adjust the settings in the `Configure` objects.

    ```go
    wasiobj := wasmedge.NewWasiModule(
      os.Args[1:],     // The args
      os.Environ(),    // The envs
      []string{".:."}, // The mapping preopens
    )
    procobj := wasmedge.NewWasmEdgeProcessModule(
      []string{"ls", "echo"}, // The allowed commands
      false,                  // Not to allow all commands
    )

    // Register the WASI and WasmEdge_Process into the VM object.
    vm := wasmedge.NewVM()
    vm.RegisterImport(wasiobj)
    vm.RegisterImport(procobj)

    // ... Execute some WASM functions.

    // Get the WASI exit code.
    exitcode := wasiobj.WasiGetExitCode()
    // The `exitcode` will be 0 if the WASI function "_start" execution has no error.
    // Otherwise, it will return with the related exit code.

    vm.Release()
    // The import objects should be deleted.
    wasiobj.Release()
    procobj.Release()
    ```

4. Example

    Create a new Go project first:

    ```bash
    mkdir wasmedge_test && cd wasmedge_test
    go mod init wasmedge_test
    ```

    Assume that there is a simple WASM from the WAT as following:

    ```wasm
    (module
      (type $t0 (func (param i32 i32) (result i32)))
      (import "extern" "func-add" (func $f-add (type $t0)))
      (func (export "addTwo") (param i32 i32) (result i32)
        local.get 0
        local.get 1
        call $f-add)
    )
    ```

    Create and edit the Go file `main.go` as following:

    ```go
    package main

    import (
      "fmt"

      "github.com/second-state/WasmEdge-go/wasmedge"
    )

    // Host function body definition.
    func host_add(data interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
      // add: i32, i32 -> i32
      res := params[0].(int32) + params[1].(int32)

      // Set the returns
      returns := make([]interface{}, 1)
      returns[0] = res

      // Return
      return returns, wasmedge.Result_Success
    }

    func main() {
      // Create the VM object.
      vm := wasmedge.NewVM()

      // The WASM module buffer.
      wasmbuf := []byte{
        /* WASM header */
        0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,
        /* Type section */
        0x01, 0x07, 0x01,
        /* function type {i32, i32} -> {i32} */
        0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F,
        /* Import section */
        0x02, 0x13, 0x01,
        /* module name: "extern" */
        0x06, 0x65, 0x78, 0x74, 0x65, 0x72, 0x6E,
        /* extern name: "func-add" */
        0x08, 0x66, 0x75, 0x6E, 0x63, 0x2D, 0x61, 0x64, 0x64,
        /* import desc: func 0 */
        0x00, 0x00,
        /* Function section */
        0x03, 0x02, 0x01, 0x00,
        /* Export section */
        0x07, 0x0A, 0x01,
        /* export name: "addTwo" */
        0x06, 0x61, 0x64, 0x64, 0x54, 0x77, 0x6F,
        /* export desc: func 0 */
        0x00, 0x01,
        /* Code section */
        0x0A, 0x0A, 0x01,
        /* code body */
        0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x0B,
      }

      // Create the module instance with the module name "extern".
      impmod := wasmedge.NewModule("extern")

      // Create and add a function instance into the module instance with export name "func-add".
      functype := wasmedge.NewFunctionType(
        []wasmedge.ValType{wasmedge.ValType_I32, wasmedge.ValType_I32},
        []wasmedge.ValType{wasmedge.ValType_I32},
      )
      hostfunc := wasmedge.NewFunction(functype, host_add, nil, 0)
      functype.Release()
      impmod.AddFunction("func-add", hostfunc)

      // Register the module instance into VM.
      vm.RegisterImport(impmod)

      res, err := vm.RunWasmBuffer(wasmbuf, "addTwo", uint32(1234), uint32(5678))
      if err == nil {
        fmt.Println("Get the result:", res[0].(int32))
      } else {
        fmt.Println("Error message:", err.Error())
      }

      impmod.Release()
      vm.Release()
    }
    ```

    Then you can build and run the Golang application with the WasmEdge Golang SDK:

    ```bash
    $ go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0-alpha.1
    $ go build
    $ ./wasmedge_test
    Get the result: 6912
    ```

5. Host Data Example

    Developers can set a external data object to the `Function` object, and access to the object in the function body.
    Assume that edit the Go file `main.go` above:

    ```go
    package main

    import (
      "fmt"

      "github.com/second-state/WasmEdge-go/wasmedge"
    )

    // Host function body definition.
    func host_add(data interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
      // add: i32, i32 -> i32
      res := params[0].(int32) + params[1].(int32)

      // Set the returns
      returns := make([]interface{}, 1)
      returns[0] = res

      // Also set the result to the data.
      *data.(*int32) = res

      // Return
      return returns, wasmedge.Result_Success
    }

    func main() {
      // Create the VM object.
      vm := wasmedge.NewVM()

      // The WASM module buffer.
      wasmbuf := []byte{
        /* WASM header */
        0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,
        /* Type section */
        0x01, 0x07, 0x01,
        /* function type {i32, i32} -> {i32} */
        0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F,
        /* Import section */
        0x02, 0x13, 0x01,
        /* module name: "extern" */
        0x06, 0x65, 0x78, 0x74, 0x65, 0x72, 0x6E,
        /* extern name: "func-add" */
        0x08, 0x66, 0x75, 0x6E, 0x63, 0x2D, 0x61, 0x64, 0x64,
        /* import desc: func 0 */
        0x00, 0x00,
        /* Function section */
        0x03, 0x02, 0x01, 0x00,
        /* Export section */
        0x07, 0x0A, 0x01,
        /* export name: "addTwo" */
        0x06, 0x61, 0x64, 0x64, 0x54, 0x77, 0x6F,
        /* export desc: func 0 */
        0x00, 0x01,
        /* Code section */
        0x0A, 0x0A, 0x01,
        /* code body */
        0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x0B,
      }

      // The additional data to set into the host function.
      var data int32 = 0

      // Create the module instance with the module name "extern".
      impmod := wasmedge.NewImportObject("extern")

      // Create and add a function instance into the module instance with export name "func-add".
      functype := wasmedge.NewFunctionType(
        []wasmedge.ValType{wasmedge.ValType_I32, wasmedge.ValType_I32},
        []wasmedge.ValType{wasmedge.ValType_I32},
      )
      hostfunc := wasmedge.NewFunction(functype, host_add, &data, 0)
      functype.Release()
      impmod.AddFunction("func-add", hostfunc)

      // Register the module instance into VM.
      vm.RegisterImport(impmod)

      res, err := vm.RunWasmBuffer(wasmbuf, "addTwo", uint32(1234), uint32(5678))
      if err == nil {
        fmt.Println("Get the result:", res[0].(int32))
      } else {
        fmt.Println("Error message:", err.Error())
      }
      fmt.Println("Data value:", data)

      impmod.Release()
      vm.Release()
    }
    ```

    Then you can build and run the Golang application with the WasmEdge Golang SDK:

    ```bash
    $ go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0-alpha.1
    $ go build
    $ ./wasmedge_test
    Get the result: 6912
    Data value: 6912
    ```

## WasmEdge AOT Compiler

In this partition, we will introduce the WasmEdge AOT compiler and the options in Go.
WasmEdge runs the WASM files in interpreter mode, and WasmEdge also supports the AOT (ahead-of-time) mode running without modifying any code.
The WasmEdge AOT (ahead-of-time) compiler compiles the WASM files for running in AOT mode which is much faster than interpreter mode. Developers can compile the WASM files into the compiled-WASM files in shared library format for universal WASM format for the AOT mode execution.

### Compilation Example

The [go_WasmAOT example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_WasmAOT) provide a tool for compiling a WASM file.

### Compiler Options

Developers can set options for AOT compilers such as optimization level and output format:

```go
const (
  // Disable as many optimizations as possible.
  CompilerOptLevel_O0 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O0)
  // Optimize quickly without destroying debuggability.
  CompilerOptLevel_O1 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O1)
  // Optimize for fast execution as much as possible without triggering significant incremental compile time or code size growth.
  CompilerOptLevel_O2 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O2)
  // Optimize for fast execution as much as possible.
  CompilerOptLevel_O3 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O3)
  // Optimize for small code size as much as possible without triggering significant incremental compile time or execution time slowdowns.
  CompilerOptLevel_Os = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_Os)
  // Optimize for small code size as much as possible.
  CompilerOptLevel_Oz = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_Oz)
)

const (
  // Native dynamic library format.
  CompilerOutputFormat_Native = CompilerOutputFormat(C.WasmEdge_CompilerOutputFormat_Native)
  // WebAssembly with AOT compiled codes in custom section.
  CompilerOutputFormat_Wasm = CompilerOutputFormat(C.WasmEdge_CompilerOutputFormat_Wasm)
)
```

Please refer to the [AOT compiler options configuration](#Configurations) for details.
