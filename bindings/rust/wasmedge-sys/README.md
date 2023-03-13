# Overview

The [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crate defines a group of low-level Rust APIs for WasmEdge, a light-weight, high-performance, and extensible WebAssembly runtime for cloud-native, edge, and decentralized applications.

For developers, it is recommended that the APIs in `wasmedge-sys` are used to construct high-level libraries, while `wasmedge-sdk` is for building up business applications.

* Notice that [wasmedge-sys](https://crates.io/crates/wasmedge-sys) requires **Rust v1.66 or above** in the **stable** channel.

## Versioning Table

The following table provides the versioning information about each crate of WasmEdge Rust bindings.

| wasmedge-sdk  | WasmEdge lib  | wasmedge-sys  | wasmedge-types| wasmedge-macro|
| :-----------: | :-----------: | :-----------: | :-----------: | :-----------: |
| 0.8.0         | 0.12.0        | 0.13.0        | 0.4.0         | 0.3.0         |
| 0.7.1         | 0.11.2        | 0.12.2        | 0.3.1         | 0.3.0         |
| 0.7.0         | 0.11.2        | 0.12          | 0.3.1         | 0.3.0         |
| 0.6.0         | 0.11.2        | 0.11          | 0.3.0         | 0.2.0         |
| 0.5.0         | 0.11.1        | 0.10          | 0.3.0         | 0.1.0         |
| 0.4.0         | 0.11.0        | 0.9           | 0.2.1         | -             |
| 0.3.0         | 0.10.1        | 0.8           | 0.2           | -             |
| 0.1.0         | 0.10.0        | 0.7           | 0.1           | -             |

## Build

To use or build the `wasmedge-sys` crate, the `WasmEdge` library is required. Please refer to [WasmEdge Installation and Uninstallation](https://wasmedge.org/book/en/quick_start/install.html) to install the `WasmEdge` library.

* The following table provides the versioning information about each crate of WasmEdge Rust bindings.

  | wasmedge-sdk  | WasmEdge lib  | wasmedge-sys  | wasmedge-types| wasmedge-macro|
  | :-----------: | :-----------: | :-----------: | :-----------: | :-----------: |
  | 0.8.0         | 0.12.0        | 0.13.0        | 0.4.0         | 0.3.0         |
  | 0.7.1         | 0.11.2        | 0.12.2        | 0.3.1         | 0.3.0         |
  | 0.7.0         | 0.11.2        | 0.12          | 0.3.1         | 0.3.0         |
  | 0.6.0         | 0.11.2        | 0.11          | 0.3.0         | 0.2.0         |
  | 0.5.0         | 0.11.1        | 0.10          | 0.3.0         | 0.1.0         |
  | 0.4.0         | 0.11.0        | 0.9           | 0.2.1         | -             |
  | 0.3.0         | 0.10.1        | 0.8           | 0.2           | -             |
  | 0.1.0         | 0.10.0        | 0.7           | 0.1           | -             |

## See also

* [WasmEdge Runtime Official Website](https://wasmedge.org/)
* [WasmEdge Docs](https://wasmedge.org/book/en/)
* [WasmEdge C API Documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md)
