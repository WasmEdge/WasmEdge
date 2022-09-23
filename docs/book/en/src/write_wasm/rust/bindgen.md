# Bindgen of Rust Functions

If your Rust program has a `main()` function, you could compile it into WebAssembly, and run it using the `wasmedge` CLI tool as a standalone application. However, a far more common use case is to compile a Rust function into WebAssembly, and then call it from a host application. That is known as an embedded WASM function. The host application uses WasmEdge language SDKs (e.g., [Go](../../sdk/go.md), [Rust](../../sdk/rust.md), [C](../../sdk/c.md), [Python](../../sdk/go.md) and [Node.js](../../sdk/node.md)) to call those WASM functions compiled from Rust source code.

All the WasmEdge host language SDKs support simple function calls. However, the WASM spec only supports a few simple data types as call parameters and return values, such as `i32`, `i64`, `f32`, `f64`, and `v128`. The `wasmedge-bindgen` crate would transform parameters and return values of Rust functions into simple integer types when the Rust function is compiled into WASM. For example, a string is automatically converted into two integers, a memory address and a length, which can be handled by the standard WASM spec. It is very easy to do this in Rust source code. Just annotate your function with the `#[wasmedge-bindgen]` macro. You can compile the annotated Rust code using the standard Rust compiler toolchain (e.g., the latest `Cargo`).

```rust
use wasmedge_bindgen::*;
use wasmedge_bindgen_macro::*;

#[wasmedge_bindgen]
pub fn say(s: String) -> Result<Vec<u8>, String> {
  let r = String::from("hello ");
  return Ok((r + s.as_str()).as_bytes().to_vec());
}
```

Of course, once the above Rust code is compiled into WASM, the function `say()` no longer takes the `String` parameter nor returns the `Vec<u8>`. So, the caller (i.e., the host application) must also deconstruct the call parameter into the memory pointer first before the call, and assemble the return value from the memory pointer after the call. These actions can be handled automagically by the WasmEdge language SDKs. To see a complete example, including the Rust WASM function and the Go host application, check out our tutorial in the Go SDK documentation.

**[A complete wasmedge-bindgen example in Rust (WASM) and Go (host)](../../sdk/go/function.md)**

Of course, the developer could choose to do `wasmedge-bindgen`'s work by hand and pass a memory pointer directly. If you are interested in this approach to call Rust compiled WASM functions, check out our [examples in the Go SDK](../../sdk/go/memory.md).
