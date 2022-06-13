# Hello World

N.B. In this example, `wasmedge-sdk v0.1.0`, `wasmedge-sys v0.7.0` and `wasmedge-types v0.1.3` are used.

Let's start off by getting all our imports right away so you can follow along

```rust
// please add this feature if you're using rust of version < 1.63
// #![feature(explicit_generic_args_with_impl_trait)]

use wasmedge_sdk::{params, Executor, ImportObjectBuilder, Module, Store};
use wasmedge_sys::WasmValue;
use wasmedge_types::wat2wasm;
```
