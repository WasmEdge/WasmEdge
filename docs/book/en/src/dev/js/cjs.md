# CommonJS module

The WasmEdge QuickJS runtime supports CommonJS (CJS) modules. This article will show you how to use CJS modules in WasmEdge.

The [example_js/simple_common_js_demo](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/simple_common_js_demo) folder in the GitHub repo contains several examples for your reference.

The [other_module/main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/other_module/main.js) file defines and exports a simple CJS module.

```
print('hello other_module')
module.exports = ['other module exports']
```

The [one_module/main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/one_module/main.js) file uses the CJS module.

```
print('hello one_module');
print('dirname:',__dirname);
let other_module_exports = require('../other_module/main.js')
print('other_module_exports=',other_module_exports)
```

Then the [file_module.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/file_module.js) file imports the module and runs it.

```
import * as one from './one_module/main.js'
print('hello file_module')
```

To run the example, you need to build a WasmEdge QuickJS runtime with CJS support.

```
$ cargo build --target wasm32-wasi --release --features=cjs
```

Finally, do the following on the CLI. 

```
$ cd example_js/simple_common_js_demo
$ wasmedge --dir .:. ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm file_module.js
hello one_module
dirname: one_module
hello other_module
other_module_exports= other module exports
hello file_module
```

>  Note, the `--dir .:.` on the command line is to give wasmedge permission to read the local directory in the file system for the `file_module.js` file.
