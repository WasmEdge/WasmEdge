# 调用 Rust 函数

如果你的 Rust 程序包含 `main()` 函数，你可以将它编译成 WASM 字节码，然后像运行一个独立应用程序一样使用 `wasmedge` 命令行工具来运行它。然而，更常见的使用场景是将一个 Rust 函数编译成 WASM 字节码，然后在其他宿主程序中调用它。这被称为嵌入的 WASM 函数。宿主应用程序使用 WasmEdge 语言 SDK （比如，[Go](../../embed/go.md)、 [Rust](../../embed/rust.md)、 [C](../../embed/c.md)、 [Python](../../embed/go.md) 以及 [Node.js](../../embed/node.md)）来运行由 Rust 源代码编译而来的 WASM 函数。

所有的 WasmEdge 语言 SDK 都支持简单的函数调用。但是，WASM 规范只支持一些基础的数据类型作为参数和返回值。当 Rust 函数被编译成 WASM 时，`wasmedge-bindgen` 会把 Rust 函数的调用参数和返回值都转换为简单的整型。比如，字符串将被自动转换为两个整型，它的内存地址和它的长度，这些类型可以被标准的 WASM 规范接受。在 Rust 中这很容易完成，只需要给你的函数添加 `#[wasmedge-bindgen]` 宏就可以。你可以使用标准的 Rust 编译工具链（比如，最新的 `Cargo`）来编译添加宏之后的 Rust 代码。

```rust
use wasmedge_bindgen::*;
use wasmedge_bindgen_macro::*;

#[wasmedge_bindgen]
pub fn say(s: String) -> Result<Vec<u8>, String> {
  let r = String::from("hello ");
  return Ok((r + s.as_str()).as_bytes().to_vec());
}
```

当然，一旦上述 Rust 代码被编译为 WASM，`say()` 函数将不再接受 `String` 作为参数，也不会再返回 `Vec<u8>`。因此，调用者（也就是宿主应用程序）必须在调用前将调用参数解构为内存指针，并在调用之后使用内存指针组装返回值。这些操作可以由 WasmEdge 语言 SDK 自动完成。如果需要一个包含 Rust WASM 函数以及 Go 宿主程序的完整的示例，请参照我们在 Go SDK 文档中的教程。

**[一个完整的 wasmedge-bindgen 示例，使用 Rust (WASM) 和 Go (宿主)](../../embed/go/function.md)**

当然，开发者可以选择自己实现 `wasmdege-bindgen` 的工作，直接传递内存指针。如果你对使用这种方式来调用 Rust 编译成的 WASM 函数感兴趣，请参照我们在 [Go SDK 中的示例](../../embed/go/memory.md)。
