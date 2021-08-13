# WasmEdge use cases 

WasmEdge is a cloud-native WebAssembly runtime hosted by the CNCF. It is widely used in edge computing, automotive, Jamstack, serverless, SaaS, service mesh, and even blockchain applications. 

Featuring AOT optimization, WasmEdge is the fastest WebAssembly runtime on the market today. 

* Paper: [A Lightweight Design for High-performance Serverless Computing](https://arxiv.org/abs/2010.07115), published on IEEE Software, Jan 2021. https://arxiv.org/abs/2010.07115
* Article: [Performance Analysis for Arm vs. x86 CPUs in the Cloud](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/), published on infoQ.com, Jan 2021. https://www.infoq.com/articles/arm-vs-x86-cloud-performance/




## Cloud-native runtime (as a lightweight Docker alternative) 

WasmEdge can be embedded into cloud-native infrastructure via its SDKs in [C](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md), [Golang](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/), [Rust](https://github.com/WasmEdge/WasmEdge/tree/master/wasmedge-rs), and [JavaScript](https://www.secondstate.io/articles/getting-started-with-rust-function/). It is also an OCI compliant runtime that can be directly [managed by CRI-O and Docker tools](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/) as a lightweight and high-performance alternative to Docker. 


1. Dapr (Distributed Application Runtime)

* Tutorial(ongoing)
* [Code template](https://github.com/second-state/dapr-wasm)



2. Service mesh (work in progress): Linkerd, MOSN, and Envoy



3. Orchestration and management (work in progress): Kubernetes, KubeEdge, SuperEdge




## Managed JavaScript runtime 

WasmEdge can act as a cloud-native JavaScript runtime by embedding a JS execution engine or interpreter. It is faster and lighter than running a JS engine inside Docker. It also allows embedding JS into other high-performance languages (eg, Rust) or using Rust to implement JS functions. 

* Example(ongoing)
* [WasmEdge’s QuickJS extension](https://github.com/second-state/quickjs-wasi)




## Serverless function-as-a-service in public clouds 

WasmEdge works with existing serverless or Jamstack platforms to provide a high-performance, portable and secure runtime for functions. It offers significant benefits even when it runs inside Docker or microVMs on those platforms. 

### AWS Lambda 

* Tutorial(ongoing)
* [Code template](https://github.com/second-state/aws-lambda-wasm-runtime)


### Tencent Serverless Functions 

* [Tutorial](https://my.oschina.net/u/4532842/blog/5172639)[(In Chinese)](https://my.oschina.net/u/4532842/blog/5172639)
* [Code template](https://github.com/second-state/tencent-scf-wasm-runtime)


### Vercel 

* [Tutorial](https://www.secondstate.io/articles/vercel-wasmedge-webassembly-rust/)
* [Code template](https://github.com/second-state/vercel-wasm-runtime)


### Netlify 

* [Tutorial](https://www.secondstate.io/articles/netlify-wasmedge-webassembly-rust-serverless/)
* [Code template](https://github.com/second-state/netlify-wasm-runtime)


### Second State Functions 

* [Tutorials](https://www.secondstate.io/faas/)



## Automotive and smart factory applications 

WasmEdge is ideally suited to run on mission-critical edge devices or edge networks. 

YoMo is a high-performance data streaming framework for far edge networks. WasmEdge is integrated into YoMo to run user-defined workloads, such as image classification along a factory assembly line. 

* [Tutorial](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/)
* [Code template](https://github.com/yomorun/yomo-wasmedge-tensorflow)


seL4 is a highly secure real-time operating system. WasmEdge is the only WebAssembly runtime that can run on seL4, and it runs at native speed. We also provide a management tool to support the OTA deployment of wasm modules. This work is still in progress. 



## Reactive functions for SaaS 

WasmEdge can support customized SaaS extensions or applications using serverless functions instead of traditional network APIs. That dramatically improves SaaS users' and developers' productivity. 

* [Build a serverless chatbot for Slack](http://reactor.secondstate.info/en/docs/user_guideline.html)
* [Build a serverless chatbot for Lark](http://reactor.secondstate.info/zh/docs/user_guideline.html)


If you have any great ideas on WasmEdge, don't hesitate to open [a GitHub issue](https://github.com/WasmEdge/WasmEdge/issues) to discuss together.




