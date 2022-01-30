# NodeJS & NPM 模块

在使用 [rollup.js](https://rollupjs.org/guide/en/)的情况下，我们就可以在WasmEdge中运行 CommonJS (CJS)和NodeJS (NPM)模块。我们在 [simple_common_js_demo/npm_main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/npm_main.js) 中展示了我们是如何做的. 在这个Demo中，我们用到了 `md5` 和 `mathjs` 这些第三方模块。

```javascript
const md5 = require('md5');
console.log('md5(message)=', md5('message'));

const {sqrt} = require('mathjs');
console.log('sqrt(-4)=', sqrt(-4).toString());
```

为了执行这段代码，我们首先必须使用 [rollup.js](https://rollupjs.org/guide/en/) 工具将所有的依赖都构建在一个文件中。在这个过程中， `rollup.js` 会将CommonJS模块转换为[WasmEdge所兼容的ES6模块](es6.md)。具体的构建脚本可以查看 [rollup.config.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/rollup.config.js)。

```javascript
const {babel} = require('@rollup/plugin-babel');
const nodeResolve = require('@rollup/plugin-node-resolve');
const commonjs = require('@rollup/plugin-commonjs');
const replace = require('@rollup/plugin-replace');

const globals = require('rollup-plugin-node-globals');
const builtins = require('rollup-plugin-node-builtins');
const plugin_async = require('rollup-plugin-async');

const babelOptions = {
  'presets': ['@babel/preset-react']
};

module.exports = [
  {
    input: './npm_main.js',
    output: {
      inlineDynamicImports: true,
      file: 'dist/npm_main.mjs',
      format: 'esm',
    },
    external: ['process', 'wasi_net','std'],
    plugins: [
      plugin_async(),
      nodeResolve(),
      commonjs({ignoreDynamicRequires: false}),
      babel(babelOptions),
      globals(),
      builtins(),
      replace({
        'process.env.NODE_ENV': JSON.stringify('production'),
        'process.env.NODE_DEBUG': JSON.stringify(''),
      }),
    ],
  },
];
```

[package.json](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/package.json) 文件中指定了 `rollup.js` 依赖以及用于将 [npm_main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/npm_main.js) 示例程序构建为单个bundle文件的命令。

```json
{
  "dependencies": {
    "mathjs": "^9.5.1",
    "md5": "^2.3.0"
  },
  "devDependencies": {
    "@babel/core": "^7.16.5",
    "@babel/preset-env": "^7.16.5",
    "@babel/preset-react": "^7.16.5",
    "@rollup/plugin-babel": "^5.3.0",
    "@rollup/plugin-commonjs": "^21.0.1",
    "@rollup/plugin-node-resolve": "^7.1.3",
    "@rollup/plugin-replace": "^3.0.0",
    "rollup": "^2.60.1",
    "rollup-plugin-babel": "^4.4.0",
    "rollup-plugin-node-builtins": "^2.1.2",
    "rollup-plugin-node-globals": "^1.4.0",
    "rollup-plugin-async": "^1.2.0"
  },
  "scripts": {
    "build": "rollup -c rollup.config.js"
  }
}
```

执行下方的NPM命令将 [npm_main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/npm_main.js) 示例程序构建为 `dist/npm_main.mjs`。

```bash
$ npm install
$ npm run build
```

下方展示的是如何在WasmEdge CLI中执行上方我们生成的JS文件。

```bash
$ wasmedge --dir .:. ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm dist/npm_main.mjs
md5(message)= 78e731027d8fd50ed642340b7c9a63b3
sqrt(-4)= 2i
```

你可以通过这种方式在WasmEdge中导入和运行任何纯JS编写的NPM包。