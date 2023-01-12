# Introduction

[WasmEdge](https://github.com/WasmEdge/WasmEdge) is a lightweight, high-performance, and extensible [WebAssembly](https://webassembly.org/) runtime for cloud native, edge, and decentralized applications. It powers serverless apps, embedded functions, microservices, smart contracts, and IoT devices. WasmEdge is currently a [CNCF (Cloud Native Computing Foundation)](https://www.cncf.io/) [Sandbox project](https://www.cncf.io/sandbox-projects/).

The WasmEdge Runtime provides a well-defined execution sandbox for its contained WebAssembly bytecode program. The runtime offers isolation and protection for operating system resources (e.g., file system, sockets, environment variables, processes) and memory space. The most important use case for WasmEdge is to safely execute user-defined or community-contributed code as plug-ins in a software product (e.g., SaaS, software-defined vehicles, edge nodes, or even blockchain nodes). It enables third-party developers, vendors, suppliers, and community members to extend and customize the software product.

This book will guide the users and developers to work with WasmEdge and show the commonly use cases.

* [WasmEdge Quick Start](quick_start.md)
* Introduce the [WasmEdge use cases](use_cases.md)
* [WasmEdge Features](features.md)
* [Create WebAssembly program](write_wasm.md) from your programming languages
* How to use the [WasmEdge Library](sdk.md) and the [WasmEdge command line tools (CLI)](cli.md)
* How to [develop a plug-in for WasmEdge](plugin.md)

If you find some issues or have any advisement, welcome to [contribute to WasmEdge](contribute.md)!
