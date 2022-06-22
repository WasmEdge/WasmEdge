# NodeJS 和 NPM 模块

使用 [rollup.js](https://rollupjs.org/guide/en/)，我们也可以在 WasmEdge 上运行 CommonJS (CJS) 和 NodeJS (NPM) 模块。 [simple_common_js_demo/npm_main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/npm_main.js) 演示展示了它是如何工作的。它使用了第三方的 `md5` 和 `mathjs` 模块。

```javascript
const md5 = require('md5');
console.log('md5(message)=', md5('message'));

const {sqrt} = require('mathjs');
console.log('sqrt(-4)=', sqrt(-4).toString());
```

为运行上述代码，我们必须先使用 [rollup.js](https://rollupjs.org/guide/en/) 工具将所有依赖项构建到一个文件中。在此过程中，`rollup.js` 将 CommonJS 模块转换为 [WasmEdge 兼容的 ES6 模块](es6.md)。构建脚本是 [rollup.config.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/rollup.config.js)。

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

[package.json](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/package.json) 文件指定了 `rollup.js` 依赖项和命令，以构建 [npm_main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/npm_main.js) 演示程序成一个包。

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

运行以下 NPM 命令将 [npm_main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/npm_main.js) 演示程序构建到 `dist/npm_main.mjs` 中。

```bash
npm install
npm run build
```

在 WasmEdge CLI 中运行结果 JS 文件，如下所示。

```bash
$ wasmedge --dir .:. ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm dist/npm_main.mjs
md5(message)= 78e731027d8fd50ed642340b7c9a63b3
sqrt(-4)= 2i
```

你可以通过这种方式在 WasmEdge 中导入和运行任何纯 JS NPM 包。
