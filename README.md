<div align="right">

  [中文文档](README-zh.md) | [正體中文文件](README-zh-TW.md)

</div>

<div align="center">
  
![WasmEdge Logo](/docs/wasmedge-runtime-logo.png)

WasmEdge (previously known as SSVM) is a lightweight, high-performance, and extensible WebAssembly runtime for cloud native, edge, and decentralized applications. It is [the fastest Wasm VM](https://ieeexplore.ieee.org/document/9214403) today. WasmEdge is an official sandbox project hosted by the [CNCF](https://www.cncf.io/). Its [use cases](https://wasmedge.org/book/en/intro/use.html) include serverless apps, embedded functions, microservices, smart contracts, and IoT devices.

![build](https://github.com/WasmEdge/WasmEdge/workflows/build/badge.svg)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![CodeQL](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/5059/badge)](https://bestpractices.coreinfrastructure.org/projects/5059)

</div>

# Quick start guides

🚀 [Install](https://wasmedge.org/book/en/start/install.html) WasmEdge \
🤖 [Build](https://wasmedge.org/book/en/extend/build.html) and [contribute to](docs/CONTRIBUTING.md) WasmEdge \
⌨️ [Run](https://wasmedge.org/book/en/index.html#webassembly-examples) a standalone Wasm program or a [JavaScript program](https://wasmedge.org/book/en/dev/js.html) from CLI or [Docker](https://wasmedge.org/book/en/start/docker.html) \
🔌 Embed a Wasm function in your [Node.js](https://wasmedge.org/book/en/embed/node.html), [Go](https://wasmedge.org/book/en/embed/go.html), [Rust](bindings/rust/), or [C](https://wasmedge.org/book/en/embed/c.html) app \
🛠 Manage and orchestrate Wasm runtimes using [Kubernetes](https://wasmedge.org/book/en/kubernetes.html), [data streaming frameworks](https://wasmedge.org/book/en/frameworks/app/yomo.html), and [blockchains](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a)
📚 [Check out our official documentation](https://wasmedge.org/book/en/)

# Introduction

The WasmEdge Runtime provides a well-defined execution sandbox for its contained WebAssembly bytecode program. The runtime offers isolation and protection for operating system resources (e.g., file system, sockets, environment variables, processes) and memory space. The most important use case for WasmEdge is to safely execute user-defined or community-contributed code as plug-ins in a software product (e.g., SaaS, software-defined vehicles, edge nodes, or even blockchain nodes). It enables third-party developers, vendors, suppliers, and community members to extend and customize the software product.

<div align="center">
  
**Checkout the [application use cases](docs/use_cases.md) or the [technical highlights](docs/highlights.md) of WasmEdge.**

</div>

## Performance

* Paper: [A Lightweight Design for High-performance Serverless Computing](https://arxiv.org/abs/2010.07115), published on IEEE Software, Jan 2021. https://arxiv.org/abs/2010.07115
* Article: [Performance Analysis for Arm vs. x86 CPUs in the Cloud](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/), published on infoQ.com, Jan 2021. https://www.infoq.com/articles/arm-vs-x86-cloud-performance/

## Features

WasmEdge can run standard WebAssembly bytecode programs compiled from C/C++, Rust, Swift, AssemblyScript, or Kotlin source code. It also [runs JavaScript](https://wasmedge.org/book/en/dev/js.html) through an embedded [QuickJS engine](https://github.com/second-state/wasmedge-quickjs). WasmEdge supports all standard WebAssembly features and proposed extensions. It also supports a number of extensions tailored for cloud-native and edge computing uses (e.g., the [WasmEdge Tensorflow extension](https://www.secondstate.io/articles/wasi-tensorflow/)). 

* [WebAssembly standard extensions](docs/extensions.md#webassembly-standard-extensions)
* [WasmEdge extensions](docs/extensions.md#wasmedge-extensions)

WasmEdge extensions to WebAssembly are typically available to developers as Rust SDKs or [JavaScript APIs](https://wasmedge.org/book/en/dev/js/quickstart.html). 

## Integrations

WasmEdge and its contained wasm program can be started from the [CLI](https://wasmedge.org/book/en/index.html) as a new process, or from a existing process. If started from an existing process (e.g., from a running [Node.js](https://wasmedge.org/book/en/embed/node.html) or [Go](https://wasmedge.org/book/en/embed/go.html) or [Rust](bindings/rust/wasmedge-rs) program), WasmEdge will simply run inside the process as a function. Currently, WasmEdge is not yet thread-safe. In order to use WasmEdge in your own application or cloud-native frameworks, please refer to the guides below.

* [Embed WasmEdge into a host application](docs/integrations.md#embed-wasmedge-into-a-host-application)
* [Orchestrate and manage WasmEdge instances using container tools](docs/integrations.md#use-wasmedge-as-a-docker-like-container)
* [Call native host functions from WasmEdge](docs/integrations.md#call-native-host-functions-from-wasmedge)

# Community

## Contributing

If you would like to contribute to the WasmEdge project, please refer to our [CONTRIBUTING](docs/CONTRIBUTING.md) document for details. If you are looking for ideas, checkout our [wish list](docs/wish_list.md)!

## Contact

If you have any questions, feel free to open a GitHub issue on a related project or to join the following channels:

* Mailing list: Send an email to [WasmEdge@googlegroups.com](https://groups.google.com/g/wasmedge/)
* Slack: Join the #WasmEdge channel on the [CNCF Slack](https://slack.cncf.io/)
* Twitter: Follow @realwasmedge on [Twitter](https://twitter.com/realwasmedge)

## Community Meeting

We host a monthly community meeting to showcase new features, demo new use cases, and a Q&A part. Everyone is welcome! 

Time: The first Thuesday of each month at 11PM Hong Kong Time/ 7AM PST.

[Public meeting agenda/notes](https://docs.google.com/document/d/1iFlVl7R97Lze4RDykzElJGDjjWYDlkI8Rhf8g4dQ5Rk/edit#) | [Zoom link](https://us06web.zoom.us/j/88282362606?pwd=UFhOdzlVKyswdW43c21BKy9DdkdyUT09)

# License
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_large)
