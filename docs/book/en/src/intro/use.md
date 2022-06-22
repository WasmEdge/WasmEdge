# Use cases

WasmEdge is a cloud-native WebAssembly runtime hosted by the CNCF. It is widely used in edge computing, automotive, Jamstack, serverless, SaaS, service mesh, and even blockchain applications. Featuring AOT compiler optimization, WasmEdge is one of the fastest WebAssembly runtimes on the market today.

* [Modern web apps](use/jamstack.md) feature rich UIs that are rendered in the browser and/or on the edge cloud. WasmEdge works with popular web UI frameworks, such as React, Vue, Yew, and Percy, to support isomorphic server-side rendering (SSR) functions on edge servers. It could also support server-side rendering of Unity3D animations and AI-generated interactive videos for web applications on the edge cloud.

* WasmEdge provides a lighweight, secure and high-performance runtime for [microservices](use/microservice.md). It is fully compatible with application service frameworks such as Dapr, and service orchestrators like Kubernetes. WasmEdge microservices can run on edge servers, and have access to distributed cache, to support both stateless and stateful business logic functions for modern web apps. Also related: [Serverless function-as-a-service in public clouds](use/serverless.md).

* [Serverless SaaS functions](use/saas.md) enables users to extend and customize their SaaS experience without operating their own API callback servers. The serverless functions can be embedded into the SaaS or reside on edge servers next to the SaaS servers. Developers simply upload functions to respond to SaaS events or to connect SaaS APIs.

* [Smart device apps](use/device.md) could embed WasmEdge as a middleware runtime to render interactive content on the UI, connect to native device drivers, and access specialized hardware features (i.e, the GPU for AI inference). The benefits of the WasmEdge runtime over native-compiled machine code include security, safety, portability, manageability, and developer productivity. WasmEdge runs on Android, OpenHarmony, and seL4 RTOS devices.

If you have any great ideas on WasmEdge, don't hesitate to open [a GitHub issue](https://github.com/WasmEdge/WasmEdge/issues) to discuss together.
