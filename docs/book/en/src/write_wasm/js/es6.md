# ES6 module support

The WasmEdge QuickJS runtime supports ES6 modules. In fact, the rollup commands we used in the [React SSR](ssr.md) examples convert and bundle CommonJS and NPM modules into ES6 modules so that they can be executed in WasmEdge QuickJS. This article will show you how to use ES6 module in WasmEdge.

We will take the example in [example_js/es6_module_demo](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/es6_module_demo) folder as an example. The [module_def.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/es6_module_demo/module_def.js) file defines and exports a simple JS function.

```javascript
function hello(){
  console.log('hello from module_def.js');
}

export {hello};
```

The [module_def_async.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/es6_module_demo/module_def_async.js) file defines and exports an aysnc function and a variable.

```javascript
export async function hello() {
  console.log('hello from module_def_async.js');
  return 'module_def_async.js : return value';
}

export var something = 'async thing';
```

The [demo.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/es6_module_demo/demo.js) file imports functions and variables from those modules and executes them.

```javascript
import {hello as module_def_hello} from './module_def.js';

module_def_hello();

var f = async () => {
  let {hello, something} = await import('./module_def_async.js');
  await hello();
  console.log('./module_def_async.js `something` is ', something);
};

f();
```

To run the example, you can do the following on the CLI.

```bash
$ wasmedge --dir .:. /path/to/wasmedge_quickjs.wasm example_js/es6_module_demo/demo.js
hello from module_def.js
hello from module_def_async.js
./module_def_async.js `something` is  async thing
```
