# System modules

The WasmEdge QuickJS runtime supports [ES6](es6.md) and [NPM](npm.md) modules for application developers. However, those approaches are too cumbersome for system developers. They need an easier way to add multiple JavaScript modules and APIs into the runtime without having to go through build tools like rollup.js. The WasmEdge QuickJS modules system allow developers to just drop JavaScript files into a `modules` folder, and have the JavaScript functions defined in the files immediately available to all JavaScript programs in the runtime. A good use case for this modules system is to support [Node.js](nodejs.md) APIs in WasmEdge.

The module system is just a collection of JavaScript files in the `modules` directory in the WasmEdge QuickJS distribution. To use the JavaScript functions and APIs defined in those modules, you just need to map this directory to the `/modules` directory inside the WasmEdge Runtime instance. The following example shows how to do this on the WasmEdge CLI. You can do this with any of the host language SDKs that support embedded use of WasmEdge.

```bash
$ ls modules

    buffer.js encoding.js events.js http.js
    ... JavaScript files for the modules ...

$ wasmedge --dir .:. target/wasm32-wasi/release/wasmedge_quickjs.wasm example_js/hello.js WasmEdge Runtime
```

The [module_demo](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/module_demo) shows how you can use the modules system to add your own JavaScript APIs. To run the demo, first copy the two files in the demo's [modules](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/module_demo/modules) directory to your WasmEdge QuickJS's `modules` directory.

```bash
cp example_js/module_demo/modules/* modules/
```

The two JavaScript files in the `modules` directory provide two simple functions. Below is the [modules/my_mod_1.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/module_demo/modules/my_mod_1.js) file.

```javascript
export function hello_mod_1(){
  console.log('hello from "my_mod_1.js"')
}
```

And the [modules/my_mod_2.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/module_demo/modules/my_mod_2.js) file.

```javascript
export function hello_mod_2(){
  console.log('hello from "my_mod_2.js"')
}
```

Then, just run the [demo.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/module_demo/demo.js) file to call the two exported functions from the modules.

```javascript
import { hello_mod_1 } from 'my_mod_1'
import { hello_mod_2 } from 'my_mod_2'

hello_mod_1()
hello_mod_2()
```

Here is the command to run the demo and the output.

```bash
$ wasmedge --dir .:. target/wasm32-wasi/release/wasmedge_quickjs.wasm example_js/module_demo/demo.js

hello from "my_mod_1.js"
hello from "my_mod_2.js"
```

Following the above tutorials, you can easily add third-party JavaScript functions and APIs into your WasmEdge QuickJS runtime. For the official distribution, we included JavaScript files to support [Node.js APIs](nodejs.md). You can use [those files](https://github.com/second-state/wasmedge-quickjs/tree/main/modules) as further examples.
