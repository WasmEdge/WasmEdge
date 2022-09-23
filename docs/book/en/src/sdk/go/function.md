# Embed a Wasm function

The WasmEdge Go SDK allows WebAssembly functions to be embedded into a Go host app. You can use the Go SDK API to pass call parameters to the embedded WebAssembly functions, and then capture the return values.
However, the WebAssembly spec only supports a few simple data types out of the box. It [does not support](https://medium.com/wasm/strings-in-webassembly-wasm-57a05c1ea333) types such as string and array. In order to pass rich types in Go to WebAssembly, we could hand-code memory pointers ([see here](memory.md)), or use an automated tool to manage the data exchange.

The [wasmedge-bindgen](https://github.com/second-state/wasmedge-bindgen) project provides Rust macros for functions to accept and return complex data types, and then for Go functions to call such Rust functions running in WasmEdge.
The full source code for the demo in this chapter is [available here](https://github.com/second-state/WasmEdge-go-examples/tree/master/wasmedge-bindgen/go_BindgenFuncs).

## Rust function compiled into WebAssembly

In the [Rust project](https://github.com/second-state/WasmEdge-go-examples/tree/master/wasmedge-bindgen/go_BindgenFuncs/rust_bindgen_funcs), all you need is to annotate [your functions](https://github.com/second-state/WasmEdge-go-examples/blob/master/wasmedge-bindgen/go_BindgenFuncs/rust_bindgen_funcs/src/lib.rs) with a `[wasmedge_bindgen]` macro.
Those annotated functions will be automatically instrumented by the Rust compiler and turned into WebAssembly functions that can be called from the bindgen related functions of WasmEdge GO SDK.
In the example below, we have several Rust functions that take complex call parameters and return complex values.

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

You can build the WebAssembly bytecode file using standard `Cargo` commands.

```bash
cd rust_bindgen_funcs
cargo build --target wasm32-wasi --release
# The output WASM will be target/wasm32-wasi/release/rust_bindgen_funcs_lib.wasm.
cp target/wasm32-wasi/release/rust_bindgen_funcs_lib.wasm ../
cd ../
```

## Go host application

In the [Go host application](https://github.com/second-state/WasmEdge-go-examples/blob/master/wasmedge-bindgen/go_BindgenFuncs/bindgen_funcs.go), you can create and set up the WasmEdge VM using the WasmEdge Go SDK.
However, instead of calling `vm.Instantiate()`, you should now call `bindgen.Instantiate(vm)` to instantiate the VM and return a `bindgen` object.

```go
func main() {
  // Expected Args[0]: program name (./bindgen_funcs)
  // Expected Args[1]: wasm file (rust_bindgen_funcs_lib.wasm))

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

  // Instantiate the bindgen and vm
  bg := bindgen.Instantiate(vm)
```

Next, you can call any `[wasmedge_bindgen]` annotated functions in the VM via the `bindgen` object.

```go
  // create_line: string, string, string -> string (inputs are JSON stringified) 
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

Finally, you can build and run the Go host application.

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v{{ wasmedge_version }}
go build
./bindgen_funcs rust_bindgen_funcs_lib.wasm
```

The standard output of this example will be the following.

```bash
Run bindgen -- create_line: {"points":[{"x":1.5,"y":3.8},{"x":2.5,"y":5.8}],"valid":true,"length":2.2360682,"desc":"A thin red line"}
Run bindgen -- say: hello bindgen funcs test
Run bindgen -- obfusticate: N dhvpx oebja sbk whzcf bire gur ynml qbt
Run bindgen -- lowest_common_multiple: 246
Run bindgen -- sha3_digest: [87 27 231 209 189 105 251 49 159 10 211 250 15 159 154 181 43 218 26 141 56 199 25 45 60 10 20 163 54 211 195 203]
Run bindgen -- keccak_digest: [126 194 241 200 151 116 227 33 216 99 159 22 107 3 177 169 216 191 114 156 174 193 32 159 246 228 245 133 52 75 55 27]
```
