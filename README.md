[中文文档](README-zh.md) [正體中文文件](README-zh-TW.md)

# Quick start guides

🚀 [Install](docs/install.md) WasmEdge \
🤖 [Build](docs/build.md) and [contribute to](docs/CONTRIBUTING.md) WasmEdge \
⌨️ [Run](docs/run.md) a standalone Wasm program or a [JavaScript program](docs/run_javascript.md) from CLI \
🔌 Embed a Wasm function in your [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/), [Go](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/), or [Rust](https://github.com/super-node/WasmEdge/tree/master/wasmedge-rs) apps \
🛠 Manage and orchestrate Wasm runtimes using [Docker tools](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/), [data streaming frameworks](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/), and [blockchains](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a) \
💭 Checkout the [use cases](docs/use_cases.md) of WasmEdge

![build](https://github.com/WasmEdge/WasmEdge/workflows/build/badge.svg)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/WasmEdge/WasmEdge.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/WasmEdge/WasmEdge/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/WasmEdge/WasmEdge.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/WasmEdge/WasmEdge/context:cpp)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/5059/badge)](https://bestpractices.coreinfrastructure.org/projects/5059)

# Introduction

WasmEdge (previously known as SSVM) is a high-performance WebAssembly (Wasm) VM optimized for Edge Computing, including Edge Clouds and Software Defined Vehicles. In its AOT mode, WasmEdge is [the fastest Wasm VM](https://ieeexplore.ieee.org/document/9214403) on the market today.

WasmEdge is an official sandbox project hosted by [CNCF](https://www.cncf.io/) (Cloud Native Computing Foundation).

The most important use case for WasmEdge is to safely execute user-defined or community-contributed code as plug-ins in a software product (e.g., a SaaS, a car OS, an edge node, or even a blockchain node). It enables third-party developers, vendors, suppliers, and community members to extend and customize the software product. With WasmEdge, a software product could become a host platform.

WasmEdge provides a well-defined execution sandbox for its contained Wasm bytecode program. The bytecode program cannot access operating system resources (e.g., file system, sockets, environment variables, processes) without explicit permissions from the VM's runner. The runner specifies the system resources the VM can access in the VM's configuration options upon starting up (a.k.a capability-based security model). WasmEdge also provides memory protection for its contained bytecode program. If the program attempts to access memory outside of the region allocated to the VM, the VM will terminate with an error message.

WasmEdge can run standard WebAssembly bytecode programs compiled from C/C++, Rust, Swift, AssemblyScript, or Kotlin source code. It also [runs JavaScript](docs/run_javascript.md) through an embedded [QuickJS engine](https://github.com/second-state/wasmedge-quickjs). WasmEdge extensions to WebAssembly are typically available as [Rust SDKs](https://www.secondstate.io/articles/wasi-tensorflow/) or [JavaScript APIs](docs/run_javascript.md).

WasmEdge and its contained wasm program can be started from the CLI as a new process, or from a existing process. If started from an existing process (e.g., from a running [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) or [Go](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/) or [Rust](https://github.com/super-node/WasmEdge/tree/master/wasmedge-rs) program), the VM will simply run inside the process as a function. It is also possible to start a WasmEdge VM instance as a thread. Currently, WasmEdge is not yet thread-safe, meaning that VM instances running in different threads in the same process will potentially be able to access each other's memory. In the future, we plan to make WasmEdge thread safe.

# Embed WasmEdge into a host application

A major use case of WasmEdge is to start an VM instance from a host application. In general, you can use the [WasmEdge C API](docs/c_api.md) to do so. You can also refer to the [quick start guide](docs/c_api_quick_start.md) of the WasmEdge C API.

However, the Wasm spec, and the [WasmEdge C API](docs/c_api.md), only supports very limited data types as input parameters and return values for the contained Wasm bytecode functions. In order to pass complex data types, such as a string of an array, as call arguments into the contained function, you should use the bindgen solution provided by the [rustwasmc](https://github.com/second-state/rustwasmc) toolchain.

We currently supports bindgen in the [Node.js host environment](https://www.secondstate.io/articles/getting-started-with-rust-function/) and in [Go environment](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/). We are working on [supporting interface types](https://github.com/WasmEdge/WasmEdge/issues/264) in place of bindgen for future releases.

# Call native host functions from WasmEdge

Sometimes, the Wasm bytecode alone could prove too limiting for some applications. WasmEdge provides a [host function API](https://github.com/WasmEdge/WasmEdge/blob/master/docs/host_function.md) that allows Wasm bytecode programs to load and call native library functions from the underlying host operating system.

Alternatively, you can register external functions as callbacks from the WasmEdge runtime. [Here is an example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_ExternRef) to register external functions in Go.

> The host functions break the Wasm sandbox. But the sandbox breaking is done with explicit permission from the system’s operator.

In fact, the extensions to WasmEdge are implemented using native host functions. For example, the [Tensorflow extension](https://www.secondstate.io/articles/wasi-tensorflow/) allows Wasm bytecode to make calls to the native Tensorflow library functions.


# Support wasm standard extensions

WasmEdge supports optional WebAssembly features and proposals. Those proposals are likely to become official WebAssembly specifications in the future. WasmEdge supports the following proposals.

* [WASI (WebAssembly Systems Interface) spec](https://github.com/WebAssembly/WASI). WasmEdge has supported the WASI spec for WebAssembly programs to interact with the host Linux operating system securely.
* [Reference Types](https://webassembly.github.io/reference-types/core/). It allows WebAssembly programs to exchange data with host applications and operating systems.
* [Bulk memory operations](https://github.com/WebAssembly/bulk-memory-operations/blob/master/proposals/bulk-memory-operations/Overview.md). The WebAssembly program sees faster memory access and performs better with bulk memory operations.
* [SIMD (Single instruction, multiple data)](https://github.com/second-state/SSVM/blob/master/docs/simd.md). For modern devices with multiple CPU cores, the SIMD allows data processing programs to take advantage of the CPUs fully. SIMD could significantly enhance the performance of data applications.

Meanwhile, the WasmEdge team is [exploring the wasi-socket proposal](https://github.com/second-state/w13e_wasi_socket) to support network access in WebAssembly programs. 

# WasmEdge extensions

A key differentiator of WasmEdge from other WebAssembly VMs is its support for non-standard extensions. The WASI spec provides a mechanism for developers to extend WebAssembly VMs efficiently and securely. The WasmEdge team created the following WASI-like extensions based on real-world customer demands.

* [Tensorflow](https://github.com/second-state/wasmedge-tensorflow). Developers can write Tensorflow inference functions using [a simple Rust API](https://crates.io/crates/ssvm_tensorflow_interface), and then run the function securely and at native speed inside WasmEdge.
* Other AI frameworks. Besides Tensorflow, the Second State team is building WASI-like extensions for AI frameworks such as ONNX and Tengine for WasmEdge.
* [Image processing](https://github.com/second-state/WasmEdge-image). WasmEdge uses native libraries to manipulate images for computer vision tasks.
* [KV Storage](https://github.com/second-state/wasmedge-storage). The WasmEdge [storage interface](https://github.com/second-state/rust_native_storage_library) allows WebAssembly programs to read and write a key value store.
* [Command interface](https://github.com/second-state/wasmedge_process_interface). WasmEdge enables webassembly functions execute native commands in the host operating system. It supports passing arguments, environment variables, STDIN / STDOUT pipes, and security policies for host access.
* [Ethereum](https://github.com/second-state/wasmedge-evmc). The WasmEdge Ewasm extension supports Ethereum smart contracts compiled to WebAssembly. It is a leading implementation for Ethereum flavored WebAssembly (Ewasm).
* [Substrate](https://github.com/second-state/substrate-ssvm-node). The [Pallet](https://github.com/second-state/pallet-ssvm) allows WasmEdge to act as an Ethereum smart contract execution engine on any Substrate based blockchains.

## Community

### Contributing

If you would like to contribute to the WasmEdge project, please refer to our [CONTRIBUTING](docs/CONTRIBUTING.md) document for details. If you are looking for ideas, checkout our [wish list](docs/wish_list.md)!

### Contact

If you have any questions, feel free to open a GitHub issue on a related project or to join the following channels:

* Mailing list: Send an email to [WasmEdge@googlegroups.com](https://groups.google.com/g/wasmedge/)
* Slack: Join the #WasmEdge channel on the [CNCF Slack](https://slack.cncf.io/)
* Twitter: Follow @realwasmedge on [Twitter](https://twitter.com/realwasmedge)

## License
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_large)
