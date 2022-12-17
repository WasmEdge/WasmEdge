# JavaScript or Domain Specific Language Runtime

In order for WebAssembly to be widely adopted by developers as a runtime, it must support "easy" languages like JavaScript. Or, better yet, through its advanced compiler toolchain, WasmEdge could support high performance DSLs (Domain Specific Languages), which are low code solutions designed for specific tasks.

## JavaScript

WasmEdge can act as a cloud-native JavaScript runtime by embedding a JS execution engine or interpreter. It is faster and lighter than running a JS engine inside Docker. WasmEdge supports JS APIs to access native extension libraries such as network sockets, tensorflow, and user-defined shared libraries. It also allows embedding JS into other high-performance languages (eg, Rust) or using Rust / C to implement JS functions.

* Tutorials
  * [Run JavaScript](https://www.secondstate.io/articles/run-javascript-in-webassembly-with-wasmedge/)
  * [Embed JavaScript in Rust](https://www.secondstate.io/articles/embed-javascript-in-rust/)
  * [Create JavaScript API using Rust functions](https://www.secondstate.io/articles/embed-rust-in-javascript/)
  * [Call C native shared library functions from JavaScript](https://www.secondstate.io/articles/call-native-functions-from-javascript/)
* [Examples](../write_wasm/js.md)
* [WasmEdge’s embedded QuickJS engine](https://github.com/second-state/wasmedge-quickjs)

## DSL for image classification

The image classification DSL is a YAML format that allows the user to specify a tensorflow model and its parameters. WasmEdge takes an image as the input of the DSL and outputs the detected item name / label.

* Example: [Run a YAML to detect food items in an image](https://github.com/second-state/wasm-learning/blob/master/cli/classify_yml/config/food.yml)

## DSL for chatbots

A chatbot DSL function takes an input string and responds with a reply string. The DSL specifies the internal state transitions of the chatbot, as well as AI models for language understanding. This work is in progress.
