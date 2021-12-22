# NodeJS and NPM module

With [CommonJS support](cjs.md), we can run NodeJS modules in WasmEdge too. The [simple_common_js_demo/npm_main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/npm_main.js) demo shows how it works. It utilizes the third-party `md5` and `mathjs` modules.

```
import * as std from 'std'

var md5 = require('md5');
console.log(__dirname);
console.log('md5(message)=',md5('message'));
const { sqrt } = require('mathjs')
console.log('sqrt(-4)=',sqrt(-4).toString())

print('write file')
let f = std.open('hello.txt','w')
let x = f.puts("hello wasm")
f.flush()
f.close()
```

In order to run it, we must first use the [vercel ncc](https://www.npmjs.com/package/@vercel/ncc) tool to build all dependencies into a single file. The build script is [package.json](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/package.json).

```
{
  "dependencies": {
    "mathjs": "^9.5.1",
    "md5": "^2.3.0"
  },
  "devDependencies": {
    "@vercel/ncc": "^0.28.6"
  },
  "scripts": {
    "ncc_build": "ncc build npm_main.js"
  }
}
```

Now, install `ncc` and [npm_main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/simple_common_js_demo/npm_main.js) dependencies via NPM, and then build the single JS file in `dist/index.js`.

```
$ npm install
$ npm run ncc_build
ncc: Version 0.28.6
ncc: Compiling file index.js
```

To run the example, you need to build a WasmEdge QuickJS runtime with CJS support.

```
$ cargo build --target wasm32-wasi --release --features=cjs
```

Run the JS file with NodeJS imports in WasmEdge CLI as follows.

```
$ wasmedge --dir .:. ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm dist/index.js
dist
md5(message)= 78e731027d8fd50ed642340b7c9a63b3
sqrt(-4)= 2i
write file
```

>  Note, the `--dir .:.` on the command line is to give wasmedge permission to read the local directory in the file system for the `file_module.js` file.
