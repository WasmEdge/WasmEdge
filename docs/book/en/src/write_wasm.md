# Write a WebAssembly Application

A key value proposition of WebAssembly is that it supports multiple programming languages. WebAssembly is a "managed runtime" for many programming languages including [C/C++](write_wasm/c.md), [Rust](write_wasm/rust.md), [Go](write_wasm/go.md) and [JavaScript](write_wasm/js.md).

* For compiled languages (e.g., C and Rust), WasmEdge WebAssembly provides a safe, secure, isolated, and containerized runtime as opposed to Native Client (NaCl).
* For interpreted or managed languages (e.g., JavaScript and Python), WasmEdge WebAssembly provides a secure, fast, lightweight, and containerized runtime as opposed to Docker + guest OS + native interpreter.

In this chapter, we will discuss how to compile sources into WebAssembly in different languages and run them in WasmEdge.
