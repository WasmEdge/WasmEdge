# JavaScript

Developers often want to use JavaScript to write business applications in cloud-native use cases. That means we must now support JavaScript in WebAssembly. Furthermore, we should also support calling C/C++ or Rust functions from JavaScript in a WebAssembly runtime to take advantage of WebAssembly’s computational efficiency. So we made this come true. The WasmEdge WebAssembly runtime allows you to do exactly that.

![](dev/javascript.png)

In this section, we will demonstrate how to run JavaScript in WasmEdge.


* [Quick Start](dev/js/quickstart.md) shows how to build a JavaScript engine in WasmEdge.
* [ES6 module](dev/js/es6.md) shows how to run ES6 modules in WasmEdge.
* [CommonJS module](dev/js/cjs.md) shows how to run CommonJS modules in WasmEdge.
* [TensorFlow](dev/js/tensorflow.md) shows how to use WasmEdge‘s Rust TensorFlow SDK from JavaScript via WasmEdge.
* [Networking sockets](dev/js/networking.md) shows how to use WasmEdge‘s Rust TensorFlow SDK from JavaScript via WasmEdge.
* [Use Rust to implement JS API discuss](dev/js/rust.md) tells how to create a complete Rust module and make it available as a JavaScript object API.
* [Async networking](dev/js/async.md)
