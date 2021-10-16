# WasmEdge use cases

WasmEdge is a cloud-native WebAssembly runtime hosted by the CNCF. It is widely used in edge computing, automotive, Jamstack, serverless, SaaS, service mesh, and even blockchain applications. Featuring AOT compiler optimization, WasmEdge is one of the fastest WebAssembly runtimes on the market today.

## Table of contents

* [Cloud-native runtime](#cloud-native-runtime-as-a-lightweight-docker-alternative)
  * [Dapr](#dapr-distributed-application-runtime)
  * [Kubernetes](#orchestration-and-management)
* [JavaScript or DSL runtime](#javascript-or-dsl-runtime)
  * [JavaScript](#javascript)
  * [DSL for image classification](#dsl-for-image-classification)
* [Serverless function-as-a-service in public clouds](#serverless-function-as-a-service-in-public-clouds)
  * [AWS Lambda](#aws-lambda)
  * [Tencent Serverless Functions](#tencent-serverless-functions)
  * [Vercel Serverless Functions](#vercel-serverless-functions)
  * [Netlify Functions](#netlify-functions)
  * [Second State Functions](#second-state-functions)
* [Software defined vehicles and smart factory](#software-defined-vehicles-and-aiot)
  * [YoMo Flow](#yomo-flow)
  * [seL4 micokernel and RTOS](#sel4-microkernel-os)
* [Reactive functions for SaaS](#reactive-functions-for-saas)
  * [Slack](#slack)
  * [Lark](#lark)

## Cloud-native runtime (as a lightweight Docker alternative)

WasmEdge can be embedded into cloud-native infrastructure via its SDKs in [C](c_api.md), [Go](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/), [Rust](../bindings/rust/wasmedge-rs), and [JavaScript](https://www.secondstate.io/articles/getting-started-with-rust-function/). It is also an OCI compliant runtime that can be directly [managed by CRI-O and Docker tools](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/) as a lightweight and high-performance alternative to Docker.

### Dapr (Distributed Application Runtime)

* [Tutorial](https://www.secondstate.io/articles/dapr-wasmedge-webassembly/)
* [Code template](https://github.com/second-state/dapr-wasm)

### Service mesh (work in progress):

* Linkerd
* MOSN
* Envoy

### Orchestration and management:

* [Kubernetes](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/)
* KubeEdge
* SuperEdge

## JavaScript or DSL runtime

In order for WebAssembly to be widely adopted by developers as a runtime, it must support "easy" languages like JavaScript. Or, better yet, through its advanced compiler toolchain, WasmEdge could support high performance DSLs (Domain Specifc Languages), which are low code solutions designed for specific tasks.

### JavaScript

WasmEdge can act as a cloud-native JavaScript runtime by embedding a JS execution engine or interpreter. It is faster and lighter than running a JS engine inside Docker. WasmEdge supports JS APIs to access native extension libraries such as network sockets, tensorflow, and user-defined shared libraries. It also allows embedding JS into other high-performance languages (eg, Rust) or using Rust / C to implement JS functions.

* Tutorials
  * [Run JavaScript](https://www.secondstate.io/articles/run-javascript-in-webassembly-with-wasmedge/)
  * [Embed JavaScript in Rust](https://www.secondstate.io/articles/embed-javascript-in-rust/)
  * [Create JavaScript API using Rust functions](https://www.secondstate.io/articles/embed-rust-in-javascript/)
  * [Call C native shared library functions from JavaScript](https://www.secondstate.io/articles/call-native-functions-from-javascript/)
* [Examples](https://github.com/WasmEdge/WasmEdge/blob/master/tools/wasmedge/examples/js/README.md)
* [WasmEdge’s embedded QuickJS engine](https://github.com/second-state/wasmedge-quickjs)

### DSL for image classification

The image classification DSL is a YAML format that allows the user to specify a tensorflow model and its parameters. WasmEdge takes an image as the input of the DSL and outputs the detected item name / label.

* Example: [Run a YAML to detect food items in an image](https://github.com/second-state/wasm-learning/blob/master/cli/classify_yml/config/food.yml)

### DSL for chatbots

A chatbot DSL function takes an input string and responds with a reply string. The DSL specifies the internal state transtions of the chatbot, as well as AI models for language understanding. This work is in progress.

## Serverless function-as-a-service in public clouds

WasmEdge works with existing serverless or Jamstack platforms to provide a high-performance, portable and secure runtime for functions. It offers significant benefits even when it runs inside Docker or microVMs on those platforms.

### AWS Lambda

* [Tutorial](https://www.cncf.io/blog/2021/08/25/webassembly-serverless-functions-in-aws-lambda/)
* [Code template](https://github.com/second-state/aws-lambda-wasm-runtime)

### Tencent Serverless Functions

* [Tutorial in Chinese](https://my.oschina.net/u/4532842/blog/5172639)
* [Code template](https://github.com/second-state/tencent-scf-wasm-runtime)

### Vercel Serverless Functions

* [Tutorial](https://www.secondstate.io/articles/vercel-wasmedge-webassembly-rust/)
* [Code template](https://github.com/second-state/vercel-wasm-runtime)

### Netlify Functions

* [Tutorial](https://www.secondstate.io/articles/netlify-wasmedge-webassembly-rust-serverless/)
* [Code template](https://github.com/second-state/netlify-wasm-runtime)

### Second State Functions

* [Tutorials](https://www.secondstate.io/faas/)

## Software defined vehicles and AIoT

WasmEdge is ideally suited to run on mission-critical edge devices or edge networks.

### YoMo Flow

YoMo is a high-performance data streaming framework for far edge networks. WasmEdge is integrated into YoMo to run user-defined workloads, such as image classification along a factory assembly line.

* [Tutorial](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/)
* [Code template](https://github.com/yomorun/yomo-wasmedge-tensorflow)

### seL4 microkernel OS

seL4 is a highly secure real-time operating system. WasmEdge is the only WebAssembly runtime that can run on seL4, and it runs at native speed. We also provide a management tool to support the OTA deployment of wasm modules.

* [Demo](https://github.com/second-state/wasmedge-seL4)

## Reactive functions for SaaS

WasmEdge can support customized SaaS extensions or applications using serverless functions instead of traditional network APIs. That dramatically improves SaaS users' and developers' productivity.

### Slack

* [Build a serverless chatbot for Slack](http://reactor.secondstate.info/en/docs/user_guideline.html)

### Lark

It is also known as 飞书 aka the Chinese Slack. It is created by Byte Dance, the parent company of Tiktok.

* [Build a serverless chatbot for Lark](http://reactor.secondstate.info/zh/docs/user_guideline.html)

If you have any great ideas on WasmEdge, don't hesitate to open [a GitHub issue](https://github.com/WasmEdge/WasmEdge/issues) to discuss together.
