# ES6 模块

WasmEdge QuickJS runtime 提供了对 ES6 模块的支持。我们在 [React SSR](ssr.md) 例子中使用的 rollup 命令实际上是将  CommonJS 和 NPM 模块转换并且捆绑为 ES6 模块，以便它们可以在 WasmEdge QuickJS 中执行。这篇文章将告诉你如何在 WasmEdge 中使用 ES6 模块。

我们将以 [example_js/es6_module_demo](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/es6_module_demo) 文件夹中的例子为例。该文件夹中的 [module_def.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/es6_module_demo/module_def.js) 文件定义并导出了一个简单的 JS 函数。

```javascript
function hello(){
  console.log('hello from module_def.js');
}

export {hello};
```

[module_def_async.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/es6_module_demo/module_def_async.js) 文件定义并导出了一个 async 函数和一个变量。

```javascript
export async function hello() {
  console.log('hello from module_def_async.js');
  return 'module_def_async.js : return value';
}

export var something = 'async thing';
```

[demo.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/es6_module_demo/demo.js) 文件从这些模块导入函数和变量并执行这些函数。

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

要运行这个例子，你可以在 CLI 上做如下操作。

```javascript
$ cd example_js/es6_module_demo
$ wasmedge --dir .:. ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm demo.js
hello from module_def.js
hello from module_def_async.js
./module_def_async.js `something` is  async thing
```

> 注意：命令行中的 `--dir .:.` 是为了赋予 wasmedge 权限，让其读取文件系统中 `demo.js` 文件的本地目录。
