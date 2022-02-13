# Embed a bindgen function

> The `wasm_bindgen` approach discussed in this chapter is deprecated. We encourage you to check out the [wasmedge_bindgen](function.md) approach or to [pass memory pointers directly](memory.md) instead.

In [this example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_BindgenFuncs), we will demonstrate how to call a few simple WebAssembly functions from a Go app. The [functions](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_BindgenFuncs/rust_bindgen_funcs/src/lib.rs) are written in Rust, and require complex call parameters and return values. The `#[wasm_bindgen]` macro is needed for the compiler tools to auto-generate the correct code to pass call parameters from Go to WebAssembly.

The WebAssembly spec only supports a few simple data types out of the box. It [does not support](https://medium.com/wasm/strings-in-webassembly-wasm-57a05c1ea333) types such as string and array. In order to pass rich types in Go to WebAssembly, the compiler needs to convert them to simple integers. For example, it converts a string into an integer memory address and an integer length. The `wasm_bindgen` tool, embedded in [rustwasmc](../../dev/rust/bindgen.md), does this conversion automatically.

> At this time, we require Rust compiler version 1.50 or less in order for WebAssembly functions to work with WasmEdge's Go API. We will [catch up to the latest Rust](https://github.com/WasmEdge/WasmEdge/issues/264) compiler version once the Interface Types spec is finalized and supported.

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

First, we use the [rustwasmc](../../dev/rust/bindgen.md) tool to compile the Rust source code into WebAssembly bytecode functions using Rust 1.50 or less.

```bash
$ rustup default 1.50.0
$ cd rust_bindgen_funcs
$ rustwasmc build
# The output WASM will be pkg/rust_bindgen_funcs_lib_bg.wasm
```

The [Go source code](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_BindgenFuncs/bindgen_funcs.go) to run the WebAssembly function in WasmEdge is as follows. The `ExecuteBindgen()` function calls the WebAssembly function and passes the call parameters using the `#[wasm_bindgen]` convention.

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
    os.Args[1:],   /// The args
    os.Environ(),  /// The envs
    []string{".:."}, /// The mapping directories
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

Next, let's build the Go application with the WasmEdge Go SDK.

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.1
go build
```

Run the Go application and it will run the WebAssembly functions embedded in the WasmEdge runtime.

```bash
$ ./bindgen_funcs rust_bindgen_funcs/pkg/rust_bindgen_funcs_lib_bg.wasm
Run bindgen -- say: hello bindgen funcs test
Run bindgen -- obfusticate: N dhvpx oebja sbk whzcf bire gur ynml qbt
Run bindgen -- lowest_common_multiple: 246
Run bindgen -- sha3_digest: [87 27 231 209 189 105 251 49 159 10 211 250 15 159 154 181 43 218 26 141 56 199 25 45 60 10 20 163 54 211 195 203]
Run bindgen -- keccak_digest: [126 194 241 200 151 116 227 33 216 99 159 22 107 3 177 169 216 191 114 156 174 193 32 159 246 228 245 133 52 75 55 27]
```
