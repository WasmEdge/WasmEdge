# React SSR

[React](https://reactjs.org/) is very popular JavaScript web UI framework. A React application is "compiled" into an HTML and JavaScript static web site. The web UI is rendered through the generated JavaScript code. However, it is often too slow and resource consuming to execute the complex generated JavaScript entirely in the browser to build the interactive HTML DOM objects. [React Server Side Rendering (SSR)](https://medium.com/jspoint/a-beginners-guide-to-react-server-side-rendering-ssr-bf3853841d55) delegates the JavaScript UI rendering to a server, and have the server stream rendered HTML DOM objects to the browser. The WasmEdge JavaScript runtime provides a lightweight and high performance container to run React SSR functions on edge servers. 

In this article, we will show you how to use the WasmEdge QuickJS runtime to implement a React SSR function.
Compared with the Docker + Linux + nodejs + v8 approach, WasmEdge is much lighter (1% of the footprint) and safer, provides better resource isolation and management, and has similar non-JIT (safe) performance.

We will cover both static and streaming rendering in this article. Static rendering is easy to understand and implement. Streaming rendering, on the other hand, provides much better user experience since the user can see partial results while waiting in front of the browser.

## Static rendering

The [example_js/react_ssr](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/react_ssr) folder in the GitHub repo contains the example's source code. It showcases how to compose HTML templates and render them into an HTML string in a JavaScript app running in WasmEdge.

The [component/Home.jsx](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/component/Home.jsx) file is the main page template in React.

```javascript
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
    );
  }
};

export default Home;
```

The `Home.jpx` template includes a [Page.jpx](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/component/Page.jsx) template for part of the page.

```javascript
import React from 'react';

class Page extends React.Component {
  render() {
    const { dataList = [] } = this.props;
    return (
      <div>
        <div>This is page</div>
      </div>
    );
  }
};

export default Page;
```

The [main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/main.js) file calls React to render the templates into HTML.

```javascript
import React from 'react';
import {renderToString} from 'react-dom/server';

import Home from './component/Home.jsx';

const content = renderToString(React.createElement(Home));
console.log(content);
```

The [rollup.config.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/rollup.config.js) and [package.json](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/package.json) files are to build the React SSR dependencies and components into a bundled JavaScript file for WasmEdge. You should use the `npm` command to build it.
The output is in the `dist/main.js` file.

```bash
npm install
npm run build
```

To run the example, do the following on the CLI. You can see that the templates are successfully composed into an HTML string.

```bash
$ cd example_js/react_ssr
$ wasmedge --dir .:. ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm dist/main.js
<div data-reactroot=""><div>This is home</div><div><div>This is page</div></div></div>
```

> Note: the `--dir .:.` on the command line is to give wasmedge permission to read the local directory in the file system for the `dist/main.js` file.

## Streaming rendering

The [example_js/react_ssr_stream](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/react_ssr_stream) folder in the GitHub repo contains the example's source code. It showcases how to streaming render an HTML string from templates in a JavaScript app running in WasmEdge.

The [component/LazyHome.jsx](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr_stream/component/LazyHome.jsx) file is the main page template in React. It "lazy" loads the inner page template after a 2s delay once the outer HTML is rendered and returned to the user.

```javascript
import React, { Suspense } from 'react';

async function sleep(ms) {
  return new Promise((r, _) => {
    setTimeout(() => r(), ms)
  });
}

async function loadLazyPage() {
  await sleep(2000);
  return await import('./LazyPage.jsx');
}

class LazyHome extends React.Component {
  render() {
    let LazyPage1 = React.lazy(() => loadLazyPage());
    return (
      <html lang="en">
        <head>
          <meta charSet="utf-8" />
          <title>Title</title>
        </head>
        <body>
          <div>
            <div> This is LazyHome </div>
            <Suspense fallback={<div> loading... </div>}>
              <LazyPage1 />
            </Suspense>
          </div>
        </body>
      </html>
    );
  }
}

export default LazyHome;
```

The [LazyPage.jsx](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr_stream/component/LazyPage.jsx) is the inner page template. It is rendered 2s after the outer page is already returned to the user.

```javascript
import React from 'react';

class LazyPage extends React.Component {
  render() {
    return (
      <div>
        <div>
          This is lazy page
        </div>
      </div>
    );
  }
}

export default LazyPage;
```

The [main.mjs](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr_stream/main.mjs)
file starts a asynchronous HTTP server, and then renders the HTML page in multiple chuncks to the response. When a HTTP request comes in, the `handle_client()` function is called to render the HTML and to send back the results through the stream.

```javascript
import * as React from 'react';
import { renderToPipeableStream } from 'react-dom/server';
import * as http from 'wasi_http';
import * as net from 'wasi_net';

import LazyHome from './component/LazyHome.jsx';

async function handle_client(s) {
  let resp = new http.WasiResponse();
  resp.headers = {
    "Content-Type": "text/html; charset=utf-8"
  }
  renderToPipeableStream(<LazyHome />).pipe(resp.chunk(s));
}

async function server_start() {
  print('listen 8001...');
  let s = new net.WasiTcpServer(8001);
  for (var i = 0; i < 100; i++) {
    let cs = await s.accept();
    handle_client(cs);
  }
}

server_start();
```

The [rollup.config.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr_stream/rollup.config.js) and [package.json](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr_stream/package.json) files are to build the React SSR dependencies and components into a bundled JavaScript file for WasmEdge. You should use the `npm` command to build it.
The output is in the `dist/main.mjs` file.

```bash
npm install
npm run build
```

To run the example, do the following on the CLI to start the server.

```bash
cd example_js/react_ssr_stream
nohup wasmedge --dir .:. ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm dist/main.mjs &
```

Send the server a HTTP request via `curl` or the browser.

```bash
curl http://localhost:8001
```

The results are as follows. The service first returns an HTML page with an empty inner section (i.e., the `loading` section), and then 2s later, the HTML content for the inner section and the JavaScript to display it.

```bash
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed

  0     0    0     0    0     0      0      0 --:--:-- --:--:-- --:--:--     0
100   211    0   211    0     0   1029      0 --:--:-- --:--:-- --:--:--  1024
100   275    0   275    0     0    221      0 --:--:--  0:00:01 --:--:--   220
100   547    0   547    0     0    245      0 --:--:--  0:00:02 --:--:--   245
100  1020    0  1020    0     0    413      0 --:--:--  0:00:02 --:--:--   413

<!DOCTYPE html><html lang="en"><head><meta charSet="utf-8"/><title>Title</title></head><body><div><div> This is LazyHome </div><!--$?--><template id="B:0"></template><div> loading... </div><!--/$--></div></body></html><div hidden id="S:0"><template id="P:1"></template></div><div hidden id="S:1"><div><div>This is lazy page</div></div></div><script>function $RS(a,b){a=document.getElementById(a);b=document.getElementById(b);for(a.parentNode.removeChild(a);a.firstChild;)b.parentNode.insertBefore(a.firstChild,b);b.parentNode.removeChild(b)};$RS("S:1","P:1")</script><script>function $RC(a,b){a=document.getElementById(a);b=document.getElementById(b);b.parentNode.removeChild(b);if(a){a=a.previousSibling;var f=a.parentNode,c=a.nextSibling,e=0;do{if(c&&8===c.nodeType){var d=c.data;if("/$"===d)if(0===e)break;else e--;else"$"!==d&&"$?"!==d&&"$!"!==d||e++}d=c.nextSibling;f.removeChild(c);c=d}while(c);for(;b.firstChild;)f.insertBefore(b.firstChild,c);a.data="$";a._reactRetry&&a._reactRetry()}};$RC("B:0","S:0")</script>
```

The streaming SSR examples make use of WasmEdge's unique asynchronous networking capabilities and ES6 module support (i.e., the rollup bundled JS file contains ES6 modules). You can learn more about [async networking](networking.md) and [ES6](es6.md) in this book.
