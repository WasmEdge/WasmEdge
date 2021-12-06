# JavaScript

WebAssembly started as a “JavaScript alternative for browsers”. The idea is to run high-performance applications compiled from languages like C/C++ or Rust safely in browsers. In the browser, WebAssembly runs side by side with JavaScript.

As WebAssembly is increasingly used in the cloud, it is now a universal runtime for cloud-native applications. Compared with Docker-like application containers, WebAssembly runtimes achieve higher performance with lower resource consumption.

In cloud-native use cases, developers often want to use JavaScript to write business applications. That means we must now support JavaScript in WebAssembly. Furthermore, we should support calling C/C++ or Rust functions from JavaScript in a WebAssembly runtime to take advantage of WebAssembly’s computational efficiency. The WasmEdge WebAssembly runtime allows you to do exactly that.

<p align="center">

![](javascript.png)

</p>

In this section, we will demonstrate how to run and enhance JavaScript in WasmEdge.

* [Getting started](js/quickstart.md) demonstrates how to run simple JavaScript programs in WasmEdge.
* [ES6 module](js/es6.md) shows how to run ES6 modules in WasmEdge.
* [CommonJS module](js/cjs.md) shows how to run CommonJS modules in WasmEdge.
* [NodeJS and NPM module](js/npm.md) shows how to run NPM modules in WasmEdge.
* [React SSR](js/ssr.md) shows an example React SSR application in WasmEdge.
* [TensorFlow](js/tensorflow.md) shows how to use WasmEdge's TensorFlow extension from its JavaScript API.
* [Networking sockets](js/networking.md) shows how to create HTTP client and server applications using the WasmEdge networking extension and its JavaScript API.
* [Async networking](js/async.md) shows how to improve HTTP server application performance by supporting asynchronous and non-blocking I/O.
* [Use Rust to implement JS API](js/rust.md) discusses how to use Rust to implement and support a JavaScript API in WasmEdge.

## A note on QuickJS

Now, the choice of QuickJS as our JavaScript engine might raise the question of performance. Isn't QuickJS [a lot slower](https://bellard.org/quickjs/bench.html) than v8 due to a lack of JIT support? Yes, but ...

First of all, QuickJS is a lot smaller than v8. In fact, it only takes 1/40 (or 2.5%) of the runtime resources v8 consumes. You can run a lot more QuickJS functions than v8 functions on a single physical machine.

Second, for most business logic applications, raw performance is not critical. The application may have computationally intensive tasks, such as AI inference on the fly. WasmEdge allows the QuickJS applications to drop to high-performance WebAssembly for these tasks while it is not so easy with v8 to add such extensions modules.

Third, it is known that [many JavaScript security issues arise from JIT](https://www.theregister.com/2021/08/06/edge_super_duper_security_mode/). Maybe turning off JIT in the cloud-native environment is not such a bad idea!


