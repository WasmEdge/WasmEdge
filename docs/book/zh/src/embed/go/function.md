# 嵌入 Wasm 函数

WasmEdge Go SDK 允许将 WebAssembly 函数嵌入到一个 Go 主程序。可以使用 Go SDK API 来传递调用参数到嵌入的 WebAssembly 函数，然后捕获返回值。

然而，WebAssembly 规范本身只支持一些简单数据类型。 它[不支持](https://medium.com/wasm/strings-in-webassembly-wasm-57a05c1ea333) 字符串和数组等类型。 为了将 Go 中的丰富类型传递给 WebAssembly，我们可以手动编码内存指针（[点击这里](memory.md)），或者使用管理数据交换的自动化工具。

[wasmedge-bindgen](https://github.com/second-state/wasmedge-bindgen) 项目为函数提供了 Rust 宏来接受和返回复杂的数据类型，然后让 Go 函数调用在 WasmEdge 中运行的此类 Rust 函数。
本章中演示的完整源代码 [可在此处获得](https://github.com/second-state/WasmEdge-go-examples/tree/master/wasmedge-bindgen/go_BindgenFuncs)。

## 将 Rust 函数编译成 WebAssembly

在 [Rust 项目](https://github.com/second-state/WasmEdge-go-examples/tree/master/wasmedge-bindgen/go_BindgenFuncs/rust_bindgen_funcs) 中，你只需要用 “[wasmedge_bindgen] ”宏来注释 [你的函数](https ://github.com/second-state/WasmEdge-go-examples/blob/master/wasmedge-bindgen/go_BindgenFuncs/rust_bindgen_funcs/src/lib.rs) 。
这些带注释的函数将由 Rust 编译器自动检测并转换为可以从 `wasmedge-bindgen` GO SDK 调用的 WebAssembly 函数。
在下面的示例中，我们有几个 Rust 函数可以进行复杂的调用参数并返回复杂值。

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
pub fn create_line(p1: String, p2: String, desc: String) -> Result<Vec<u8>, String> {
  let point1: Point = serde_json::from_str(p1.as_str()).unwrap();
  let point2: Point = serde_json::from_str(p2.as_str()).unwrap();
  let length = ((point1.x - point2.x) * (point1.x - point2.x) + (point1.y - point2.y) * (point1.y - point2.y)).sqrt();

  let valid = if length == 0.0 { false } else { true };

  let line = Line { points: vec![point1, point2], valid: valid, length: length, desc: desc };

  return Ok(serde_json::to_vec(&line).unwrap());
}

#[wasmedge_bindgen]
pub fn say(s: String) -> Result<Vec<u8>, String> {
  let r = String::from("hello ");
  return Ok((r + s.as_str()).as_bytes().to_vec());
}

#[wasmedge_bindgen]
pub fn obfusticate(s: String) -> Result<Vec<u8>, String> {
  let r: String = (&s).chars().map(|c| {
    match c {
      'A' ..= 'M' | 'a' ..= 'm' => ((c as u8) + 13) as char,
      'N' ..= 'Z' | 'n' ..= 'z' => ((c as u8) - 13) as char,
      _ => c
    }
  }).collect();
  Ok(r.as_bytes().to_vec())
}

#[wasmedge_bindgen]
pub fn lowest_common_multiple(a: i32, b: i32) -> Result<Vec<u8>, String> {
  let r = lcm(a, b);
  return Ok(r.to_string().as_bytes().to_vec());
}

#[wasmedge_bindgen]
pub fn sha3_digest(v: Vec<u8>) -> Result<Vec<u8>, String> {
  return Ok(Sha3_256::digest(&v).as_slice().to_vec());
}

#[wasmedge_bindgen]
pub fn keccak_digest(s: Vec<u8>) -> Result<Vec<u8>, String> {
  return Ok(Keccak256::digest(&s).as_slice().to_vec());
}
```

你可以使用标准的 Cargo 命令构建 WebAssembly 字节码文件。

```bash
$ cd rust_bindgen_funcs
$ cargo build --target wasm32-wasi --release

# 输出 WASM 将是 target/wasm32-wasi/release/rust_bindgen_funcs_lib.wasm.

$ cp target/wasm32-wasi/release/rust_bindgen_funcs_lib.wasm ../
$ cd ../
```

## Go host 程序

在 [Go host 程序](https://github.com/second-state/WasmEdge-go-examples/blob/master/wasmedge-bindgen/go_BindgenFuncs/bindgen_funcs.go)中，你可以使用 WasmEdge Go SDK 创建和设置 WasmEdge VM。
但是，你现不应该调用 `vm.Instantiate()` 而应该调用`bindgen.Instantiate(vm)` 来实例化 VM 并返回 `bindgen` 对象。

```go
func main() {
  // 预期的 Args[0]: 程序名 (./bindgen_funcs)
  // 预期的 Args[1]: wasm 文件 (rust_bindgen_funcs_lib.wasm))
  
  wasmedge.SetLogErrorLevel()
  var conf = wasmedge.NewConfigure(wasmedge.WASI)
  var vm = wasmedge.NewVMWithConfig(conf)
  var wasi = vm.GetImportModule(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],     // The args
    os.Environ(),    // The envs
    []string{".:."}, // The mapping preopens
  )
  vm.LoadWasmFile(os.Args[1])
  vm.Validate()

  // 实例化 bindgen 和 vm
  bg := bindgen.Instantiate(vm)
```

接下来，你可以通过 `bindgen` 对象调用 VM 中的任何 `[wasmedge_bindgen]` 注释函数。

```go
  // create_line: string, string, string -> string (输入已 JSON 字符串化)  
  res, err := bg.Execute("create_line", "{\"x\":2.5,\"y\":7.8}", "{\"x\":2.5,\"y\":5.8}", "A thin red line")
  if err == nil {
    fmt.Println("Run bindgen -- create_line:", string(res))
  } else {
    fmt.Println("Run bindgen -- create_line FAILED", err)
  }

  // say: string -> string
  res, err = bg.Execute("say", "bindgen funcs test")
  if err == nil {
    fmt.Println("Run bindgen -- say:", string(res))
  } else {
    fmt.Println("Run bindgen -- say FAILED")
  }

  // obfusticate: string -> string
  res, err = bg.Execute("obfusticate", "A quick brown fox jumps over the lazy dog")
  if err == nil {
    fmt.Println("Run bindgen -- obfusticate:", string(res))
  } else {
    fmt.Println("Run bindgen -- obfusticate FAILED")
  }

  // lowest_common_multiple: i32, i32 -> i32
  res, err = bg.Execute("lowest_common_multiple", int32(123), int32(2))
  if err == nil {
    num, _ := strconv.ParseInt(string(res), 10, 32)
    fmt.Println("Run bindgen -- lowest_common_multiple:", num)
  } else {
    fmt.Println("Run bindgen -- lowest_common_multiple FAILED")
  }

  // sha3_digest: array -> array
  res, err = bg.Execute("sha3_digest", []byte("This is an important message"))
  if err == nil {
    fmt.Println("Run bindgen -- sha3_digest:", res)
  } else {
    fmt.Println("Run bindgen -- sha3_digest FAILED")
  }

  // keccak_digest: array -> array
  res, err = bg.Execute("keccak_digest", []byte("This is an important message"))
  if err == nil {
    fmt.Println("Run bindgen -- keccak_digest:", res)
  } else {
    fmt.Println("Run bindgen -- keccak_digest FAILED")
  }

  bg.Release()
  vm.Release()
  conf.Release()
}
```

最后，你可以构建并运行 Go host 程序。

```bash
go build
./bindgen_funcs rust_bindgen_funcs_lib.wasm
```

本示例的标准输出如下。

```bash
Run bindgen -- create_line: {"points":[{"x":1.5,"y":3.8},{"x":2.5,"y":5.8}],"valid":true,"length":2.2360682,"desc":"A thin red line"}
Run bindgen -- say: hello bindgen funcs test
Run bindgen -- obfusticate: N dhvpx oebja sbk whzcf bire gur ynml qbt
Run bindgen -- lowest_common_multiple: 246
Run bindgen -- sha3_digest: [87 27 231 209 189 105 251 49 159 10 211 250 15 159 154 181 43 218 26 141 56 199 25 45 60 10 20 163 54 211 195 203]
Run bindgen -- keccak_digest: [126 194 241 200 151 116 227 33 216 99 159 22 107 3 177 169 216 191 114 156 174 193 32 159 246 228 245 133 52 75 55 27]
```
