# Self-assessment outline

## Table of contents

* [Metadata](#metadata)
  * [Security links](#security-links)
* [Overview](#overview)
  * [Actors](#actors)
  * [Actions](#actions)
  * [Background](#background)
  * [Goals](#goals)
  * [Non-goals](#non-goals)
* [Self-assessment use](#self-assessment-use)
* [Security functions and features](#security-functions-and-features)
* [Project compliance](#project-compliance)
* [Secure development practices](#secure-development-practices)
* [Security issue resolution](#security-issue-resolution)
* [Appendix](#appendix)

## Metadata

|   |  |
| -- | -- |
| Software | https://github.com/WasmEdge/WasmEdge  |
| Security Provider | Yes   |
| Languages | Rust |
| SBOM | https://github.com/WasmEdge/WasmEdge/tree/master/thirdparty |
| | |

### Security links

| Doc | url |
| -- | -- |
| Security file | https://github.com/WasmEdge/WasmEdge/blob/master/SECURITY.md |

## Overview

WasmEdge is a lightweight, high-performance, and extensible WebAssembly runtime for cloud native, edge, and decentralized applications. It powers serverless apps, embedded functions, microservices, smart contracts, and IoT devices.

### Background

The WasmEdge Runtime provides a well-defined execution sandbox for its contained WebAssembly bytecode program. The runtime offers isolation and protection for operating system resources (e.g., file system, sockets, environment variables, processes) and memory space. The most important use case for WasmEdge is to safely execute user-defined or community-contributed code as plug-ins in a software product (e.g., SaaS, software-defined vehicles, edge nodes, or even blockchain nodes). It enables third-party developers, vendors, suppliers, and community members to extend and customize the software product

### Actors

WasmEdge can run standard WebAssembly bytecode programs compiled from C/C++, Rust, Swift, AssemblyScript, or Kotlin source code. It [runs JavaScript](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript), including 3rd party ES6, CJS, and NPM modules, in a secure, fast, lightweight, portable, and containerized sandbox. It also supports mixing of those languages (e.g., to [use Rust to implement a JavaScript API](https://wasmedge.org/docs/develop/javascript/rust)), the [Fetch](https://wasmedge.org/docs/develop/javascript/networking#fetch-client) API, and [Server-side Rendering (SSR)](https://wasmedge.org/docs/develop/javascript/ssr) functions on edge servers.

### Actions

WasmEdge and its contained wasm program can be started from the [CLI](https://wasmedge.org/docs/category/running-with-wasmedge) as a new process, or from a existing process. If started from an existing process (e.g., from a running [Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge) or [Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge) program), WasmEdge will simply run inside the process as a function. Currently, WasmEdge is not yet thread-safe. In order to use WasmEdge in your own application or cloud-native frameworks, please refer to the guides below.

* [Embed WasmEdge into a host application](https://wasmedge.org/docs/embed/overview)
* [Orchestrate and manage WasmEdge instances using container tools](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes)
* [Run a WasmEdge app as a Dapr microservice](https://wasmedge.org/docs/develop/rust/dapr)


### Goals

WasmEdge supports all standard WebAssembly features and many proposed extensions. It also supports a number of extensions tailored for cloud-native and edge computing uses (e.g., the WasmEdge network sockets,Postgres and MySQL-based database driver, and the WasmEdge AI extension).

### Non-goals



## Self-assessment use

- security-policy: https://github.com/WasmEdge/WasmEdge/blob/master/SECURITY.md
- email-contact: security@secondstate.io

## Security functions and features

* Security Relevant. Not only the Dependabot but also Google OSS Fuzz are used to security test

## Project compliance

* Compliance.  List any security standards or sub-sections the project is
  already documented as meeting (PCI-DSS, COBIT, ISO, GDPR, etc.).

## Secure development practices

* Development Pipeline.  A description of the testing and assessment processes that
  the software undergoes as it is developed and built. Be sure to include specific
information such as if contributors are required to sign commits, if any container
images immutable and signed, how many reviewers before merging, any automated checks for
vulnerabilities, etc.
* Communication Channels.  security@secondstate.io
* Ecosystem. How does your software fit into the cloud native ecosystem?  (e.g.
  Flibber is integrated with both Flocker and Noodles which covers
virtualization for 80% of cloud users. So, our small number of "users" actually
represents very wide usage across the ecosystem since every virtual instance uses
Flibber encryption by default.)

## Security issue resolution

For all WasmEdge security-related defects, please send an email to security@secondstate.io. You will receive an acknowledgement mail within 24 hours. After that, we will give a detailed response about the subsequent process within 48 hours. Please do not submit security vulnerabilities directly as Github Issues.

## Appendix

* Known Issues Over Time. List or summarize statistics of past vulnerabilities
  with links. If none have been reported, provide data, if any, about your track
record in catching issues in code review or automated testing.
* [CII Best Practices](https://www.coreinfrastructure.org/programs/best-practices-program/).
  Best Practices. A brief discussion of where the project is at
  with respect to CII best practices and what it would need to
  achieve the badge.
* Case Studies. Check out our [list of Adopters](https://wasmedge.org/docs/contribute/users/) who are using WasmEdge in their projects 
* Related Projects / Vendors. 