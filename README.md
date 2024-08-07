<div align="right">

  [中文](README-zh.md) | [正體中文](README-zh-TW.md) | [日本語で読む](README-ja.md)

</div>

<div align="center">
  
![WasmEdge Logo](/docs/wasmedge-runtime-logo.png)

# [🤩 WasmEdge is the easiest and fastest way to run LLMs on your own devices. 🤩](https://www.secondstate.io/articles/wasm-runtime-agi/)

WasmEdge is a lightweight, high-performance, and extensible WebAssembly runtime. It is [the fastest Wasm VM](https://ieeexplore.ieee.org/document/9214403) today. WasmEdge is an official sandbox project hosted by the [CNCF](https://www.cncf.io/). Its [use cases](https://wasmedge.org/book/en/use_cases.html) include modern web application architectures (Isomorphic & Jamstack applications), microservices on the edge cloud, serverless SaaS APIs, embedded functions, smart contracts, and smart devices.

![build](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml/badge.svg)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![CodeQL](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/5059/badge)](https://bestpractices.coreinfrastructure.org/projects/5059)

</div>

# Quick start guides

🚀 [Install](https://wasmedge.org/docs/start/install) WasmEdge \
🤖 [Build](https://wasmedge.org/docs/category/build-wasmedge-from-source) and [contribute to](https://wasmedge.org/docs/contribute/) WasmEdge \
⌨️ [Run](https://wasmedge.org/docs/category/running-with-wasmedge) a standalone Wasm program or a [JavaScript program](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript) from CLI or [Docker](https://wasmedge.org/docs/start/getting-started/quick_start_docker) \
🔌 Embed a Wasm function in your [Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge), [Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge), or [C](https://wasmedge.org/docs/category/c-sdk-for-embedding-wasmedge) app \
🛠 Manage and orchestrate Wasm runtimes using [Kubernetes](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes), [data streaming frameworks](https://wasmedge.org/docs/embed/use-case/yomo), and [blockchains](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a) \
📚 **[Check out our official documentation](https://wasmedge.org/docs/)**

# Introduction

The WasmEdge Runtime provides a well-defined execution sandbox for its contained WebAssembly bytecode program. The runtime offers isolation and protection for operating system resources (e.g., file system, sockets, environment variables, processes) and memory space. The most important use case for WasmEdge is to safely execute user-defined or community-contributed code as plug-ins in a software product (e.g., SaaS, software-defined vehicles, edge nodes, or even blockchain nodes). It enables third-party developers, vendors, suppliers, and community members to extend and customize the software product. **[Learn more here](https://wasmedge.org/docs/contribute/users)**

## Performance

* [A Lightweight Design for High-performance Serverless Computing](https://arxiv.org/abs/2010.07115), published on IEEE Software, Jan 2021. [https://arxiv.org/abs/2010.07115](https://arxiv.org/abs/2010.07115)
* [Performance Analysis for Arm vs. x86 CPUs in the Cloud](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/), published on infoQ.com, Jan 2021. [https://www.infoq.com/articles/arm-vs-x86-cloud-performance/](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)
* [WasmEdge is the fastest WebAssembly Runtime in Suborbital Reactr test suite](https://blog.suborbital.dev/suborbital-wasmedge), Dec 2021

## Features

WasmEdge can run standard WebAssembly bytecode programs compiled from C/C++, Rust, Swift, AssemblyScript, or Kotlin source code. It [runs JavaScript](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript), including 3rd party ES6, CJS, and NPM modules, in a secure, fast, lightweight, portable, and containerized sandbox. It also supports mixing of those languages (e.g., to [use Rust to implement a JavaScript API](https://wasmedge.org/docs/develop/javascript/rust)), the [Fetch](https://wasmedge.org/docs/develop/javascript/networking#fetch-client) API, and [Server-side Rendering (SSR)](https://wasmedge.org/docs/develop/javascript/ssr) functions on edge servers.

WasmEdge supports [all standard WebAssembly features and many proposed extensions](https://wasmedge.org/docs/start/wasmedge/extensions/proposals). It also supports a number of extensions tailored for cloud-native and edge computing uses (e.g., the [WasmEdge network sockets](https://wasmedge.org/docs/category/socket-networking),[Postgres and MySQL-based database driver](https://wasmedge.org/docs/category/database-drivers), and the [WasmEdge AI extension](https://wasmedge.org/docs/category/ai-inference)).

 **Learn more about [technical highlights](https://wasmedge.org/docs/start/wasmedge/features) of WasmEdge.**

## Integrations and management

WasmEdge and its contained wasm program can be started from the [CLI](https://wasmedge.org/docs/category/running-with-wasmedge) as a new process, or from an existing process. If started from an existing process (e.g., from a running [Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge) or [Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge) program), WasmEdge will simply run inside the process as a function. Currently, WasmEdge is not yet thread-safe. In order to use WasmEdge in your own application or cloud-native frameworks, please refer to the guides below.

* [Embed WasmEdge into a host application](https://wasmedge.org/docs/embed/overview)
* [Orchestrate and manage WasmEdge instances using container tools](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes)
* [Run a WasmEdge app as a Dapr microservice](https://wasmedge.org/docs/develop/rust/dapr)

# Community

## Contributing

If you would like to contribute to the WasmEdge project, please refer to our [CONTRIBUTING](https://wasmedge.org/docs/contribute/overview) document for details. If you are looking for ideas, checkout our ["help wanted" issues](https://github.com/WasmEdge/WasmEdge/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22)!

## Roadmap

Check out our [project roadmap](https://github.com/WasmEdge/WasmEdge/blob/master/docs/ROADMAP.md) to see the upcoming features and plans for WasmEdge.

## Contact

If you have any questions, feel free to open a GitHub issue on a related project or to join the following channels:

* Mailing list: Send an email to [WasmEdge@googlegroups.com](https://groups.google.com/g/wasmedge/)
* Discord: Join the [WasmEdge Discord server](https://discord.gg/h4KDyB8XTt)!
* Slack: Join the #WasmEdge channel on the [CNCF Slack](https://slack.cncf.io/)
* Twitter: Follow @realwasmedge on [Twitter](https://twitter.com/realwasmedge)

## Adopters

Check out our [list of Adopters](https://wasmedge.org/docs/contribute/users/) who are using WasmEdge in their projects.

## Community Meeting

We host a monthly community meeting to showcase new features, demo new use cases, and a Q&A part. Everyone is welcome!

Time: The first Tuesday of each month at 11PM Hong Kong Time/ 7AM PST.

[Public meeting agenda/notes](https://docs.google.com/document/d/1iFlVl7R97Lze4RDykzElJGDjjWYDlkI8Rhf8g4dQ5Rk/edit#) | [Zoom link](https://us06web.zoom.us/j/89156807241?pwd=VHl5VW5BbmY2eUtTYkY0Zm9yUHRRdz09)

# License

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_large)
