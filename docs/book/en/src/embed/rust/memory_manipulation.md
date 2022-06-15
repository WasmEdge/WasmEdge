# Memory Manipulation

In this example, we'll present how to manipulate the linear memory with the APIs defined in [wasmedge_sdk::Memory](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Memory.html).

N.B. In this example, `wasmedge-sdk v0.1.1`, `wasmedge-sys v0.7.1` and `wasmedge-types v0.1.3` are used.

## Wasm module

Before talk about the code, let's first see the wasm module we use in this example.

(to do ...)

Let's start off by getting all imports right away so you can follow along

```rust
// please add this feature if you're using rust of version < 1.63
// #![feature(explicit_generic_args_with_impl_trait)]

use wasmedge_sdk::{params, Executor, ImportObjectBuilder, Module, Store};
use wasmedge_sys::WasmValue;
use wasmedge_types::wat2wasm;
```
