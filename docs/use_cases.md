# WasmEdge use cases 

WasmEdge is a cloud-native WebAssembly runtime hosted by the CNCF. It is widely used in edge computing, automotive, Jamstack, serverless, SaaS, service mesh, and even blockchain applications. Featuring AOT compiler optimization, WasmEdge is the fastest WebAssembly runtime on the market today. 

* Paper: [A Lightweight Design for High-performance Serverless Computing](https://arxiv.org/abs/2010.07115), published on IEEE Software, Jan 2021. https://arxiv.org/abs/2010.07115
* Article: [Performance Analysis for Arm vs. x86 CPUs in the Cloud](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/), published on infoQ.com, Jan 2021. https://www.infoq.com/articles/arm-vs-x86-cloud-performance/

## 1 Cloud-native runtime (as a lightweight Docker alternative) 

WasmEdge can be embedded into cloud-native infrastructure via its SDKs in [C](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md), [Golang](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/), [Rust](https://github.com/WasmEdge/WasmEdge/tree/master/wasmedge-rs), and [JavaScript](https://www.secondstate.io/articles/getting-started-with-rust-function/). It is also an OCI compliant runtime that can be directly [managed by CRI-O and Docker tools](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/) as a lightweight and high-performance alternative to Docker. 

### 1.1 Dapr (Distributed Application Runtime)

* Tutorial (to be published)
* [Code template](https://github.com/second-state/dapr-wasm)

### 1.2 Service mesh (work in progress): 

* Linkerd
* MOSN
* Envoy

### 1.3 Orchestration and management (work in progress): 

* Kubernetes
* KubeEdge
* SuperEdge


## 2 JavaScript or DSL runtime 

In order for WebAssembly to be widely adopted by developers as a runtime, it must support "easy" languages like JavaScript. Or, better yet, through its advanced compiler toolchain, WasmEdge could support high performance DSLs (Domain Specifc Languages), which are low code solutions designed for specific tasks.

### 2.1 JavaScript

WasmEdge can act as a cloud-native JavaScript runtime by embedding a JS execution engine or interpreter. It is faster and lighter than running a JS engine inside Docker. It also allows embedding JS into other high-performance languages (eg, Rust) or using Rust to implement JS functions. 

* [Example](https://github.com/WasmEdge/WasmEdge/blob/master/docs/run_javascript.md)
* [WasmEdge’s QuickJS extension](https://github.com/second-state/quickjs-wasi)

### 2.2 DSL for image classification

The image classification DSL is a YAML format that allows the user to specify a tensorflow model and its parameters. WasmEdge takes an image as the input of the DSL and outputs the detected item name / label.

* Example: [Run a YMAL to detect food items in an image](https://github.com/second-state/wasm-learning/blob/master/cli/classify_yml/config/food.yml) 

### 2.3 DSL for chatbots

A chatbot DSL function takes an input string and responds with a reply string. The DSL specifies the internal state transtions of the chatbot, as well as AI models for language understanding. This work is in progress.


## 3 Serverless function-as-a-service in public clouds 

WasmEdge works with existing serverless or Jamstack platforms to provide a high-performance, portable and secure runtime for functions. It offers significant benefits even when it runs inside Docker or microVMs on those platforms. 

### 3.1 AWS Lambda 

* Tutorial(ongoing)
* [Code template](https://github.com/second-state/aws-lambda-wasm-runtime)

### 3.2 Tencent Serverless Functions 

* [Tutorial](https://my.oschina.net/u/4532842/blog/5172639)[(In Chinese)](https://my.oschina.net/u/4532842/blog/5172639)
* [Code template](https://github.com/second-state/tencent-scf-wasm-runtime)

### 3.3 Vercel 

* [Tutorial](https://www.secondstate.io/articles/vercel-wasmedge-webassembly-rust/)
* [Code template](https://github.com/second-state/vercel-wasm-runtime)

### 3.4 Netlify 

* [Tutorial](https://www.secondstate.io/articles/netlify-wasmedge-webassembly-rust-serverless/)
* [Code template](https://github.com/second-state/netlify-wasm-runtime)

### 3.5 Second State Functions 

* [Tutorials](https://www.secondstate.io/faas/)


## 4 Automotive and smart factory applications 

WasmEdge is ideally suited to run on mission-critical edge devices or edge networks.

### 4.1 YoMo Flow

YoMo is a high-performance data streaming framework for far edge networks. WasmEdge is integrated into YoMo to run user-defined workloads, such as image classification along a factory assembly line. 

* [Tutorial](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/)
* [Code template](https://github.com/yomorun/yomo-wasmedge-tensorflow)

### 4.2 seL4 microkernel OS

seL4 is a highly secure real-time operating system. WasmEdge is the only WebAssembly runtime that can run on seL4, and it runs at native speed. We also provide a management tool to support the OTA deployment of wasm modules. This work is still in progress. 


## 5 Reactive functions for SaaS 

WasmEdge can support customized SaaS extensions or applications using serverless functions instead of traditional network APIs. That dramatically improves SaaS users' and developers' productivity. 

### 5.1 Slack

* [Build a serverless chatbot for Slack](http://reactor.secondstate.info/en/docs/user_guideline.html)

### 5.2 Lark (飞书 aka the Chinese Slack)

* [Build a serverless chatbot for Lark](http://reactor.secondstate.info/zh/docs/user_guideline.html)


If you have any great ideas on WasmEdge, don't hesitate to open [a GitHub issue](https://github.com/WasmEdge/WasmEdge/issues) to discuss together.




