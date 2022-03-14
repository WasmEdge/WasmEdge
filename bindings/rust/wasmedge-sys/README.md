# wasmedge-sys

The [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crate defines a group of low-level Rust APIs, which simply wrap WasmEdge C APIs and provide the safe counterparts. It is recommended that the APIs in `wasmedge-sys` are used to construct high-level libraries, but not to build up applications.

## Overview

This library contains safe Rust bindings for WasmEdge, a lightweight, high-performance, and extensible WebAssembly runtime for cloud native, edge, and decentralized applications.

WasmEdge 0.9.0 is the lowest supported version for the underlying core library.

Most of this documentation is generated from the C API. Until all parts of the documentation have been reviewed there will be incongruities with the actual Rust API.

See also

* [WasmEdge Runtime](https://wasmedge.org/)
* [WasmEdge C API Documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md)
