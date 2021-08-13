## Quick Start JavaScript(QuickJs) for the WasmEdge

The following is an example for running a JavaScript file with qjs.wasm.

Get WASM file form [`qjs.wasm`](https://github.com/L-jasmine/quickjs-wasi),and the js file `hello.js` is as following:

```js
// hello.js
console.log("hello js");
```

Then you can run:

```bash
$ wasmedge --dir .:. qjs.wasm hello.js
hello js
```

Or run a qjs repl:

get repl.js from [quickjs-wasi](https://github.com/L-jasmine/quickjs-wasi)

```bash
$ wasmedge --dir .:. qjs.wasm repl.js
QuickJS - Type "\h" for help
qjs >
```