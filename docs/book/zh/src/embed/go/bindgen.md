# 嵌入 bindgen 函数

> 本章节中讨论的 `wasm_bindgen` 方法已经废弃，我们推荐你查看 [wasmedge_bindgen](function.md)，或者直接传入内存指针；

在这个例子中，我们将演示如何从 Go 应用中调用一些非常简单的 WebAssembly 函数。这些[函数](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_BindgenFuncs/rust_bindgen_funcs/src/lib.rs)都是基于  Rust 语言编写，并且调用参数和返回值都比较复杂。编译工具需要函数声明  `#[wasm_bindgen]` 宏才能自动生成正确的代码，以便支持从 Go 中传递调用参数给 WebAssembly。

WebAssembly 规范只支持几种开箱即用的简单数据类型，并不支持字符串和数组类型。为了实现在 Go 中向 WebAssembly 传递更丰富的类型，编译器需要将其转换成简单的整数。比如，将一个字符串转换成内存地址和字符串长度。`wasm_bindgen` 工具嵌入在 [rustwasmc](../../dev/rust/bindgen.md) 中自动执行转换。

> 在编写本文档的时候，我们要求 Rust 编译器必须是 1.50 或者以下的版本，这样才能够让 WebAssembly 函数与 WasmEdge 的 Go API 正常工作。当接口类型规范最终确定并完成支持后，我们会尽快迁移到[最新的 Rust 版本](https://github.com/WasmEdge/WasmEdge/issues/264)。

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

首先，我们使用 [rustwasmc](https://github.com/WasmEdge/WasmEdge/blob/master/docs/book/en/src/dev/rust/bindgen.md) 工具将 Rust 源代码编译成字节码。

```bash
$ rustup default 1.50.0
$ cd rust_bindgen_funcs
$ rustwasmc build
# The output WASM will be pkg/rust_bindgen_funcs_lib_bg.wasm
```

我们在 Go 程序里面嵌入 WasmEdge 运行 WebAssembly 函数，[Go 程序源码如下](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_BindgenFuncs/bindgen_funcs.go)，`ExecuteBindgen()` 函数调用 `#[wasm_bindgen]` 转换后的 WebAssembly 函数并传递调用参数。

```go
package main

import (
  "fmt"
  "os"
  "github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
  // Expected Args[0]: program name (./bindgen_funcs)
  // Expected Args[1]: wasm or wasm-so file (rust_bindgen_funcs_lib_bg.wasm))

  wasmedge.SetLogErrorLevel()

  var conf = wasmedge.NewConfigure(wasmedge.WASI)
  var vm = wasmedge.NewVMWithConfig(conf)
  var wasi = vm.GetImportModule(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],     // The args
    os.Environ(),    // The envs
    []string{".:."}, // The mapping directories
  )

  // Instantiate wasm
  vm.LoadWasmFile(os.Args[1])
  vm.Validate()
  vm.Instantiate()

  // Run bindgen functions
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

接下来，让我们用 WasmEdge Go SDK 来构建 Go 应用。

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0
go build
```

执行 Go 应用程序，该应用将在 WasmEdge runtime 中执行嵌入的 WebAssembly 函数。

```bash
$ ./bindgen_funcs rust_bindgen_funcs/pkg/rust_bindgen_funcs_lib_bg.wasm
Run bindgen -- say: hello bindgen funcs test
Run bindgen -- obfusticate: N dhvpx oebja sbk whzcf bire gur ynml qbt
Run bindgen -- lowest_common_multiple: 246
Run bindgen -- sha3_digest: [87 27 231 209 189 105 251 49 159 10 211 250 15 159 154 181 43 218 26 141 56 199 25 45 60 10 20 163 54 211 195 203]
Run bindgen -- keccak_digest: [126 194 241 200 151 116 227 33 216 99 159 22 107 3 177 169 216 191 114 156 174 193 32 159 246 228 245 133 52 75 55 27]
```
