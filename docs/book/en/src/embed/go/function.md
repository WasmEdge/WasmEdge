# Embed a Wasm function

The WasmEdge Go SDK allows WebAssembly functions to be embedded into
a Go host app. You can use the Go SDK API to pass call parameters
to the embedded WebAssembly functions, and then capture the return values.
However, the WebAssembly spec only supports a few simple data types out of the box. It [does not support](https://medium.com/wasm/strings-in-webassembly-wasm-57a05c1ea333) types such as string and array. In order to pass rich types in Go to WebAssembly,
we could hand-code memory pointers ([see here](memory.md)), or use an
automated tool to manage the data exchange.
The [wasmedge-bindgen](https://github.com/second-state/wasmedge-bindgen) 
project provides Rust macros for functions to accept and return complex data types, and then for Go functions to call such Rust functions running in WasmEdge.

Work in progress.
See here: https://github.com/second-state/wasmedge-bindgen
