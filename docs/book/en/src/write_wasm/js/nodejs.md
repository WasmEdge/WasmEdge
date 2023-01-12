# Node.js support

Many existing JavaScript apps simply use Node.js built-in APIs. In order to support and reuse these JavaScript apps, we are in the process of implementing many Node.JS APIs for WasmEdge QuickJS. The goal is to have unmodified Node.js programs running in WasmEdge QuickJS.

In order to use Node.js APIs in WasmEdge, you must make the `modules` directory from [wasmedge-quickjs](https://github.com/second-state/wasmedge-quickjs) accessible to the WasmEdge Runtime. The simplest approach is to clone the [wasmedge-quickjs](https://github.com/second-state/wasmedge-quickjs) repo, and run the Node.js app from the repo's top directory.

```bash
git clone https://github.com/second-state/wasmedge-quickjs
cd wasmedge-quickjs
curl -OL https://github.com/second-state/wasmedge-quickjs/releases/download/v0.4.0-alpha/wasmedge_quickjs.wasm
cp -r /path/to/my_node_app .
wasmedge --dir .:. wasmedge_quickjs.wasm my_node_app/index.js
```

> If you want to run `wasmedge` from a directory outside of the repo, you will need to tell it where to find the `modules` directory using the `--dir` option. A typical command will look like this: `wasmedge --dir .:. --dir ./modules:/path/to/modules wasmedge_quickjs.wasm app.js`

The progress of Node.js support in WasmEdge QuickJS is **[tracked in this issue](https://github.com/WasmEdge/WasmEdge/issues/1535).** There are two approaches for supporting Node.js APIs in WasmEdge QuickJS.

## The JavaScript modules

Some Node.js functions can be implemented in pure JavaScript using the [modules](modules.md) approach. For example,

* The [querystring](https://github.com/second-state/wasmedge-quickjs/blob/main/modules/querystring.js) functions just perform string manipulations.
* The [buffer](https://github.com/second-state/wasmedge-quickjs/blob/main/modules/buffer.js) functions manage and encode arrays and memory structures.
* The [encoding](https://github.com/second-state/wasmedge-quickjs/blob/main/modules/encoding.js) and [http](https://github.com/second-state/wasmedge-quickjs/blob/main/modules/http.js) functions support corresponding Node.js APIs by wrapping around [Rust internal modules](rust.md).

## The Rust internal modules

Other Node.js functions must be implemented in Rust using the [internal_module](rust.md) approach. There are two reasons for that. First, some Node.js API functions are CPU intensive (e.g., encoding) and is most efficiently implemented in Rust. Second, some Node.js API functions require access to the underlying system (e.g., networking and file system) through native host functions.

* The [core](https://github.com/second-state/wasmedge-quickjs/blob/main/src/internal_module/core.rs) module provides OS level functions such as `timeout`.
* The [encoding](https://github.com/second-state/wasmedge-quickjs/blob/main/src/internal_module/encoding.rs) module provides high-performance encoding and decoding functions, which are in turn [wrapped into Node.js encoding APIs](https://github.com/second-state/wasmedge-quickjs/blob/main/modules/encoding.js).
* The [wasi_net_module](https://github.com/second-state/wasmedge-quickjs/blob/main/src/internal_module/wasi_net_module.rs) provides JavaScript networking functions implemented via the Rust-based WasmEdge WASI socket API. It is then wrapped into the [Node.js http module](https://github.com/second-state/wasmedge-quickjs/blob/main/modules/http.js).

Node.js compatibility support in WasmEdge QuickJS is a work in progress. It is a great way for new developers to get familiar with WasmEdge QuickJS. Join us!
