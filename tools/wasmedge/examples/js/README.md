# Examples for running JavaScript in WasmEdge

These examples showcase how to run JavaScript programs in WasmEdge. WasmEdge provides [a lightweight sandbox](https://www.secondstate.io/articles/run-javascript-in-webassembly-with-wasmedge/) for running and managing JavaScript applications. More examples are available in [this Github repo](https://github.com/second-state/wasmedge-quickjs/).

## Basic examples

The `qjs.wasm` is a JavaScript interpreter compiled into WebAssembly. Below is a very simple JavaScript example in WasmEdge.

```
$ wasmedge --dir .:. qjs.wasm hello.js 1 2 3
```

You can run an interactive JavaScript terminal (read-eval-print loop, or REPL) from the WasmEdge CLI.

```
$ wasmedge --dir .:. qjs.wasm repl.js
```

## Tensorflow examples

The `qjs_tf.wasm` is a JavaScript interpreter with WasmEdge Tensorflow extension compiled into WebAssembly. To run `qjs_tf.wasm`, you must use the `wasmedge-tensorflow-lite` CLI tool, which is a build of WasmEdge with Tensorflow extension built-in. You can [download a full Tensorflow-based JavaScript example](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/tensorflow_lite_demo) to classify images.

```
$ wasmedge-tensorflow-lite --dir .:. qjs_tf.wasm main.js
```

## Learn more

[Running JavaScript in WebAssembly with WasmEdge](https://www.secondstate.io/articles/run-javascript-in-webassembly-with-wasmedge/)

[The WasmEdge QuickJS runtime project](https://github.com/second-state/wasmedge-quickjs/)
* [Simple JavaScript examples](https://github.com/second-state/wasmedge-quickjs/tree/main/)
* [Embed JavaScript in Rust](https://github.com/second-state/wasmedge-quickjs/tree/embed_in_rust/)
* [Native host functions in JavaScript](https://github.com/second-state/wasmedge-quickjs/tree/host_func/)

