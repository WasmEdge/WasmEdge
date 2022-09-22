# Embed a bindgen function

In [this example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_BindgenFuncs), we will demonstrate how to call a few simple WebAssembly functions from a Go app. The [functions](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_BindgenFuncs/rust_bindgen_funcs/src/lib.rs) are written in Rust, and require complex call parameters and return values. The `#[wasmedge_bindgen]` macro is needed for the compiler tools to auto-generate the correct code to pass call parameters from Go to WebAssembly.

The WebAssembly spec only supports a few simple data types out of the box. It [does not support](https://medium.com/wasm/strings-in-webassembly-wasm-57a05c1ea333) types such as string and array. In order to pass rich types in Go to WebAssembly, the compiler needs to convert them to simple integers. For example, it converts a string into an integer memory address and an integer length. The `wasmedge_bindgen` tool does this conversion automatically.

```rust
use num_integer::lcm;
use serde::{Deserialize, Serialize};
use sha3::{Digest, Keccak256, Sha3_256};
#[allow(unused_imports)]
use wasmedge_bindgen::*;
use wasmedge_bindgen_macro::*;

#[derive(Serialize, Deserialize, Debug)]
struct Point {
    x: f32,
    y: f32,
}

#[derive(Serialize, Deserialize, Debug)]
struct Line {
    points: Vec<Point>,
    valid: bool,
    length: f32,
    desc: String,
}

#[wasmedge_bindgen]
pub fn create_line(p1: String, p2: String, desc: String) -> String {
    let point1: Point = serde_json::from_str(&p1).unwrap();
    let point2: Point = serde_json::from_str(&p2).unwrap();
    let length = ((point1.x - point2.x) * (point1.x - point2.x)
        + (point1.y - point2.y) * (point1.y - point2.y))
        .sqrt();

    let valid = if length == 0.0 { false } else { true };

    let line = Line {
        points: vec![point1, point2],
        valid: valid,
        length: length,
        desc: desc,
    };

    return serde_json::to_string(&line).unwrap();
}

#[wasmedge_bindgen]
pub fn say(s: String) -> String {
    let r = String::from("hello ");
    return r + &s;
}

#[wasmedge_bindgen]
pub fn obfusticate(s: String) -> String {
    (&s).chars()
        .map(|c| match c {
            'A'..='M' | 'a'..='m' => ((c as u8) + 13) as char,
            'N'..='Z' | 'n'..='z' => ((c as u8) - 13) as char,
            _ => c,
        })
        .collect()
}

#[wasmedge_bindgen]
pub fn lowest_common_multiple(a: i32, b: i32) -> i32 {
    let r = lcm(a, b);
    return r;
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

First, we will compile the Rust source code into WebAssembly bytecode functions.

```bash
$ cd rust_bindgen_funcs
$ cargo build --release --target wasm32-wasi
# The output WASM will be target/wasm32-wasi/release/rust_bindgen_funcs_lib.wasm
```

The [Go source code](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_BindgenFuncs/bindgen_funcs.go) to run the WebAssembly function in WasmEdge is as follows. The `Execute()` function calls the WebAssembly function and passes the call parameters using the `#[wasmedge_bindgen]` convention.

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
  // Expected Args[1]: wasm or wasm-so file (rust_bindgen_funcs_lib.wasm))

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

  vm.LoadWasmFile(os.Args[1])
  vm.Validate()
  // Instantiate the bindgen and vm
  bg := bindgen.New(vm)
  bg.Instantiate()

  // Run bindgen functions
  var res []interface{}
  var err error
  // create_line: array, array, array -> array (inputs are JSON stringified)
  res, _, err = bg.Execute("create_line", "{\"x\":1.5,\"y\":3.8}", "{\"x\":2.5,\"y\":5.8}", "A thin red line")
  if err == nil {
    fmt.Println("Run bindgen -- create_line:", res[0].(string))
  } else {
    fmt.Println("Run bindgen -- create_line FAILED")
  }
  // say: array -> array
  res, _, err = bg.Execute("say", "bindgen funcs test")
  if err == nil {
    fmt.Println("Run bindgen -- say:", res[0].(string))
  } else {
    fmt.Println("Run bindgen -- say FAILED")
  }
  // obfusticate: array -> array
  res, _, err = bg.Execute("obfusticate", "A quick brown fox jumps over the lazy dog")
  if err == nil {
    fmt.Println("Run bindgen -- obfusticate:", res[0].(string))
  } else {
    fmt.Println("Run bindgen -- obfusticate FAILED")
  }
  // lowest_common_multiple: i32, i32 -> i32
  res, _, err = bg.Execute("lowest_common_multiple", int32(123), int32(2))
  if err == nil {
    fmt.Println("Run bindgen -- lowest_common_multiple:", res[0].(int32))
  } else {
    fmt.Println("Run bindgen -- lowest_common_multiple FAILED")
  }
  // sha3_digest: array -> array
  res, _, err = bg.Execute("sha3_digest", []byte("This is an important message"))
  if err == nil {
    fmt.Println("Run bindgen -- sha3_digest:", res[0].([]byte))
  } else {
    fmt.Println("Run bindgen -- sha3_digest FAILED")
  }
  // keccak_digest: array -> array
  res, _, err = bg.Execute("keccak_digest", []byte("This is an important message"))
  if err == nil {
    fmt.Println("Run bindgen -- keccak_digest:", res[0].([]byte))
  } else {
    fmt.Println("Run bindgen -- keccak_digest FAILED")
  }

  bg.Release()
  conf.Release()
}
```

Next, let's build the Go application with the WasmEdge Go SDK.

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v{{ wasmedge_version }}
go build
```

Run the Go application and it will run the WebAssembly functions embedded in the WasmEdge runtime.

```bash
$ ./bindgen_funcs rust_bindgen_funcs/target/wasm32-wasi/release/rust_bindgen_funcs_lib.wasm
Run bindgen -- create_line: {"points":[{"x":1.5,"y":3.8},{"x":2.5,"y":5.8}],"valid":true,"length":2.2360682,"desc":"A thin red line"}
Run bindgen -- say: hello bindgen funcs test
Run bindgen -- obfusticate: N dhvpx oebja sbk whzcf bire gur ynml qbt
Run bindgen -- lowest_common_multiple: 246
Run bindgen -- sha3_digest: [87 27 231 209 189 105 251 49 159 10 211 250 15 159 154 181 43 218 26 141 56 199 25 45 60 10 20 163 54 211 195 203]
Run bindgen -- keccak_digest: [126 194 241 200 151 116 227 33 216 99 159 22 107 3 177 169 216 191 114 156 174 193 32 159 246 228 245 133 52 75 55 27]
```
