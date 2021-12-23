# Example: React SSR

[React Server-Side Rendering (SSR)](https://medium.com/jspoint/a-beginners-guide-to-react-server-side-rendering-ssr-bf3853841d55)
is a common use of JavaScript in BFF (backend for frontend) functions. 
Instead of rending HTML DOM elements in the browser, 
it uses the React framework
to render HTML elements from the server side to 
speed up the application. It is an ideal use case for serverless functions
in [Jamstack](https://jamstack.org/) applications.

In this article, we will show you how to use the WasmEdge QuickJS runtime
to implement a React SSR function. Compared with the Docker + Linux + nodejs + v8 approach, WasmEdge is much lighter (1% of the footprint) and safer,
provides better resource isolation and management,
and has similar non-JIT (safe) performance.

The [example_js/react_ssr](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/react_ssr) folder in the GitHub repo contains the example's source code.

The [component/Home.jsx](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/component/Home.jsx)
file is the main page template in React.

```
import React from 'react';
import Page from './Page.jsx';
 
class Home extends React.Component {

  render() {
    const { dataList = [] } = this.props;
    return (
      <div>
        <div>This is home</div>
        <Page></Page>
      </div>
    )
  }
}

export default Home;
```

The `Home.jpx` template includes a [Page.jpx](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/component/Page.jsx)
template for part of the page.

```
import React from 'react';

class Page extends React.Component {

  render() {
    const { dataList = [] } = this.props;
    return (
      <div>
        <div>This is page</div>
      </div>
    )
  }
}

export default Page;
```

The [main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/main.js)
file calls React to render the templates into HTML.

```
import Home from './component/Home.jsx';
import {renderToString} from 'react-dom/server';
import React from 'react';

const content = renderToString(React.createElement(Home));
console.log(content)
```

The [rollup.config.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/rollup.config.js)
and [package.json](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/package.json)
files are to build the React SSR dependencies into a single JavaScript file
for WasmEdge. You should use the `npm` command to build it.
The output is in the `dist/main.js` file.

```
$ npm install
$ npm run build
```

To run the example, you need to build a WasmEdge QuickJS runtime with CJS support.

```
$ cargo build --target wasm32-wasi --release --features=cjs
```

Finally, do the following on the CLI. 

```
$ wasmedge --dir .:. ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm dist/main.js
<div data-reactroot=""><div>This is home</div><div><div>This is page</div></div></div>
```

>  Note, the `--dir .:.` on the command line is to give wasmedge permission to read the local directory in the file system for the `dist/main.js` file.


