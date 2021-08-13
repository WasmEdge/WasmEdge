[中文文档](README-zh.md) [正體中文文件](README-zh-TW.md)

# Quick start guides

🚀 [Install](docs/install.md) WasmEdge \
🤖 [Build](docs/build.md) and [contribute to](docs/CONTRIBUTING.md) WasmEdge\
⌨️  Run a standalone Wasm program [from CLI](docs/run.md) or [Node.js](https://github.com/second-state/wasm-learning/tree/master/ssvm/file-example) or [Golang](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_ReadFile) or [CRI-O / Docker](https://github.com/second-state/runw/blob/master/README.md)\
💭 [Deploy a Wasm function](https://www.secondstate.io/articles/getting-started-with-function-as-a-service-in-rust/) as a web service (FaaS)\
🛠 [Embed a user-defined Wasm function](http://reactor.secondstate.info/en/docs/user_guideline.html) in a SaaS platform\
🔩 [Embed a Wasm function](https://www.secondstate.io/articles/getting-started-with-rust-function/) in your Node.js web app\
🔌 [Embed a Wasm function](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_Mobilenet) in your Golang app\
🔗 [Deploy a Wasm function](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a) as a blockchain smart contract

![build](https://github.com/WasmEdge/WasmEdge/workflows/build/badge.svg)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/WasmEdge/WasmEdge.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/WasmEdge/WasmEdge/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/WasmEdge/WasmEdge.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/WasmEdge/WasmEdge/context:cpp)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)

# Introduction

WasmEdge (previously known as SSVM) is a high-performance WebAssembly (Wasm) VM optimized for Edge Computing, including Edge Clouds and Software Defined Vehicles. In its AOT mode, WasmEdge is [the fastest Wasm VM](https://ieeexplore.ieee.org/document/9214403) on the market today.

WasmEdge is an official sandbox project hosted by [CNCF](https://www.cncf.io/) (Cloud Native Computing Foundation).

The most important use case for WasmEdge is to safely execute user-defined or community-contributed code as plug-ins in a software product (e.g., a SaaS, a car OS, an edge node, or even a blockchain node). It enables third-party developers, vendors, suppliers, and community members to extend and customize the software product. With WasmEdge, a software product could become a host platform.

WasmEdge provides a well-defined execution sandbox for its contained Wasm bytecode program. The bytecode program cannot access operating system resources (e.g., file system, sockets, environment variables, processes) without explicit permissions from the VM's runner. The runner specifies the system resources the VM can access in the VM's configuration options upon starting up (a.k.a capability-based security model).

WasmEdge also provides memory protection for its contained bytecode program. If the program attempts to access memory outside of the region allocated to the VM, the VM will terminate with an error message. 

WasmEdge and its contained wasm program can be started from the CLI as a new process, or from a existing process. If started from an existing process (e.g., from a running [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) or [Golang](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_Mobilenet) program), the VM will simply run inside the process as a function. It is also possible to start a WasmEdge VM instance as a thread. Currently, WasmEdge is not yet thread-safe, meaning that VM instances running in different threads in the same process will potentially be able to access each other's memory. In the future, we plan to make WasmEdge thread safe.

# Embed WasmEdge into a host application

A major use case of WasmEdge is to start an VM instance from a host application. In general, you can use the [WasmEdge C API](docs/c_api.md) to do so. You can also refer to the [quick start guide](docs/c_api_quick_start.md) of the WasmEdge C API.

However, the Wasm spec, and the [WasmEdge C API](docs/c_api.md), only supports very limited data types as input parameters and return values for the contained Wasm bytecode functions. In order to pass complex data types, such as a string of an array, as call arguments into the contained function, you should use the bindgen solution provided by the [rustwasmc](https://github.com/second-state/rustwasmc) toolchain.

We currently supports bindgen in the [Node.js host environment](https://www.secondstate.io/articles/getting-started-with-rust-function/) and in [Golang environment](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_Mobilenet). We are working on [supporting interface types](https://github.com/WasmEdge/WasmEdge/issues/264) in place of bindgen for future releases.

# Call native host functions from WasmEdge

Sometimes, the Wasm bytecode alone could prove too limiting for some applications. WasmEdge provides a [host function API](https://github.com/WasmEdge/WasmEdge/blob/master/docs/host_function.md) that allows Wasm bytecode programs to load and call native library functions from the underlying host operating system.

> The host functions break the Wasm sandbox. But the sandbox breaking is done with explicit permission from the system’s operator.

In fact, the extensions to WasmEdge are implemented using native host functions. For example, the [Tensorflow extension](https://www.secondstate.io/articles/wasi-tensorflow/) allows Wasm bytecode to make calls to the native Tensorflow library functions.

# Manage WasmEdge VM instances

With the [WasmEdge C API](https://github.com/WasmEdge/WasmEdge/blob/master/include/api/wasmedge.h.in), you can write a program to start, stop, and manage WasmEdge VM instances in your own applications. For example, 

* When WasmEdge functions are embedded in [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) or in [Slack](http://reactor.secondstate.info/en/docs/user_guideline.html), the VM is launched by the application when there is an incoming request. 
* When WasmEdge functions are plugged into a data flow engine like [YoMo](https://github.com/yomorun/yomo-flow-ssvm-example), the VM is launched when a new data point flows through the system.
* As an OCI compliant runtime, WasmEdge applications could be managed by Docker tools such as CRI-O and Docker Hub. [See how](https://github.com/second-state/runw) We are currently working on Kubernetes support.

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
* [Storage](https://github.com/second-state/wasmedge-storage). The WasmEdge [storage interface](https://github.com/second-state/rust_native_storage_library) allows WebAssembly programs to read and write a key value store.
* [Command interface](https://github.com/second-state/wasmedge_process_interface). WasmEdge enables webassembly functions execute native commands in the host operating system. It supports passing arguments, environment variables, STDIN / STDOUT pipes, and security policies for host access.
* [Ethereum](https://github.com/second-state/wasmedge-evmc). The WasmEdge Ewasm extension supports Ethereum smart contracts compiled to WebAssembly. It is a leading implementation for Ethereum flavored WebAssembly (Ewasm).
* [Substrate](https://github.com/second-state/substrate-ssvm-node). The [Pallet](https://github.com/second-state/pallet-ssvm) allows WasmEdge to act as an Ethereum smart contract execution engine on any Substrate based blockchains.

# Use cases

WasmEdge enables software products to be extended and customized by their users. With WasmEdge, any software product can build a developer ecosystem. Here are some specific use cases from our customers and partners. 

* A *Jamstack application* consists of a static frontend with JavaScript to interact with backend APIs. It is a very popular [modern web application architecture](https://jamstack.org/). The frontend static files can be distributed over CDNs, and the backend functions can be hosted on edge nodes. The [cloud-based WasmEdge](https://www.secondstate.io/faas/) hosts secure and high performance backend serverless functions for Jamstack apps especially on the Edge cloud. 
  * Example: [add a watermark to any image on your web app](https://second-state.github.io/wasm-learning/faas/watermark/html/index.html).
  * Example: [serverless Tensorflow functions for Tencent Cloud](https://github.com/second-state/tencent-tensorflow-scf).
* *SaaS applications* often need be tailored or customized “on the edge” for customer requirements. With WasmEdge, SaaS applications can directly embed and execute user-submitted code as part of the workflow (eg as a callback function to handle events from the SaaS app).
  * Example: [the Slack / Feishu application platform could embed user-submitted serverless functions via WasmEdge to respond to messages (ie conversation bot)](http://reactor.secondstate.info/en/docs/user-create-a-bot.html).
  * Example: [WasmEdge runs custom code to process events in IoT streaming data framework YoMo](https://github.com/yomorun/yomo-wasmedge-tensorflow).
* WasmEdge is adapted to run on a variety of embedded and real time operating systems for *edge devices*. That allows developers to write high performance applications once, in Rust or C, and run them safely on many edge device platforms. 
  * Example: [RISC-V stack from RIOS Lab](https://rioslab.org/).
  * Ongoing: Porting WasmEdge to the SeL4 real-time OS
  * Upcoming: WasmEdge could be used as a RTOS code runtime for software modules in autonomous cars.
* *Blockchain smart contracts* are user submitted code executed by all nodes in the network. WasmEdge is a smart contract execution engine on leading blockchain projects. 
  * Example: [Ethereum flavored WASM smart contracts on Substrate and Polkadot](https://github.com/ParaState/substrate-ssvm-node).

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
