# NodeJS and NPM module

With [rollup.js](https://rollupjs.org/guide/en/), we can run CommonJS (CJS) and NodeJS (NPM) modules in WasmEdge too. The [simple_common_js_demo/npm_main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/npm_main.js) demo shows how it works. It utilizes the third-party `md5` and `mathjs` modules.

```javascript
const md5 = require('md5');
console.log('md5(message)=', md5('message'));

const {sqrt} = require('mathjs');
console.log('sqrt(-4)=', sqrt(-4).toString());
```

In order to run it, we must first use the [rollup.js](https://rollupjs.org/guide/en/) tool to build all dependencies into a single file. In the process, `rollup.js` converts CommonJS modules into [WasmEdge-compatible ES6 modules](es6.md). The build script is [rollup.config.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/rollup.config.js).

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

The [package.json](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/package.json) file specifies the `rollup.js` dependencies and the command to build the [npm_main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/npm_main.js) demo program into a single bundle.

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

Run the following NPM commands to build [npm_main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/npm_main.js) demo program into `dist/npm_main.mjs`.

```bash
npm install
npm run build
cd ../../
```

Run the result JS file in WasmEdge CLI as follows.

```bash
$ wasmedge --dir .:. /path/to/wasmedge_quickjs.wasm example_js/simple_common_js_demo/dist/npm_main.mjs
md5(message)= 78e731027d8fd50ed642340b7c9a63b3
sqrt(-4)= 2i
```

You can import and run any NPM packages in WasmEdge this way.
