# React SSR

[React 服务端渲染（SSR）](https://medium.com/jspoint/a-beginners-guide-to-react-server-side-rendering-ssr-bf3853841d55) 是 JavaScript 在 BFF（为前端提供服务的后端）函数中一种常见的使用场景。
和在浏览器中渲染 HTML DOM 元素的方式不同，这种技术使用 React 框架在服务端就生成了 HTML 元素，以此来加快应用的加载速度。
在 [Jamstack](https://jamstack.org/) 应用中，这是使用 serverless 函数时一种较理想的方式。

在这篇文章中，我们将向你展示如何使用 WasmEdge 的 QuickJS 运行时来实现 React SSR 的能力。与 Docker + Linux + nodejs + v8 的方案相比，WasmEdge 要轻量得多（仅 1% 的占用），也更安全，能够提供更好的资源隔离和管理机制，并有着和非 JIT（同时是安全的）近似的性能。

本文将包含对静态渲染和流式渲染两种渲染方式的介绍。静态渲染相对容易理解和实现。而流式渲染则可以提供更好的用户体验，因为用户在浏览器前等待结果时，可以优先看到生成的部分内容。

## 静态渲染

本示例的源代码可以在 GitHub 仓库的 [example_js/react_ssr](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/react_ssr) 文件夹中找到。它展示了运行于 WasmEdge 的 JavaScript 应用程序，是如何编排 HTML 模板并将其渲染成 HTML 字符串的。

文件 [component/Home.jsx](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/component/Home.jsx) 里是 React 的主页模板。

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

`Home.jsx` 中会包含 [Page.jsx](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/component/Page.jsx) 提供的模板，作为页面的一部分。

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

文件 [main.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/main.js) 会调用 React 将模板渲染成 HTML。

```javascript
import React from 'react';
import {renderToString} from 'react-dom/server';

import Home from './component/Home.jsx';

const content = renderToString(React.createElement(Home));
console.log(content);
```

目录中的 [rollup.config.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/rollup.config.js) 文件和 [package.json](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr/package.json) 文件用于把 React SSR 的所有依赖和组件打包成一个 WasmEdge 可用的 JavaScript 文件。你可以使用 `npm` 命令来进行构建，构建产物会输出到 `dist/main.js` 文件里。

```bash
npm install
npm run build
```

要运行这个例子，请在命令行中执行以下命令。你会看到，所有的模板成功合成了一个 HTML 字符串。

```bash
$ cd example_js/react_ssr
$ wasmedge --dir .:. ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm dist/main.js
<div data-reactroot=""><div>This is home</div><div><div>This is page</div></div></div>
```

> 注意： 命令行里的 `--dir .:.` 会给 WasmEdge 读取本地文件系统下目录的权限，以此读取 `dist/main.js` 文件。

## 流式渲染

本示例的源代码可以在 GitHub 仓库的 [example_js/react_ssr_stream](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/react_ssr_stream) 文件夹中找到。它展示了运行于 WasmEdge 的 JavaScript 应用程序，是如何流式地把 HTML 模板渲染成 HTML 字符串的。

文件 [component/LazyHome.jsx](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr_stream/component/LazyHome.jsx) 是 React 的主页模板。当外层的 HTML 渲染好并返回给用户 2s 之后，它才会开始“懒”加载内层的页面模板。

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

[LazyPage.jsx](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr_stream/component/LazyPage.jsx) 里就是内层的模板。只有在外层页面返回给用户 2s 之后，它才会被渲染。

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

[main.mjs](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr_stream/main.mjs) 文件会启动一个异步的 HTTP 服务器，然后把 HTML 页面渲染成多段放入响应。当一个 HTTP 请求进来的时候，`handle_client()` 函数就会被调用来渲染 HTML，并以流的形式返回结果。

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

目录中的 [rollup.config.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr_stream/rollup.config.js) 文件和 [package.json](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/react_ssr_stream/package.json) 文件用于把 React SSR 的所有依赖和组件打包成一个 WasmEdge 可用的 JavaScript 文件。你可以使用 `npm` 命令来进行构建，构建产物会输出到 `dist/main.js` 文件里。

```bash
npm install
npm run build
```

要运行这个例子，请在命令行上执行以下命令来启动服务器。

```bash
cd example_js/react_ssr_stream
nohup wasmedge --dir .:. ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm dist/main.mjs &
```

然后通过 `curl` 或浏览器发送一个 HTTP 请求。

```bash
curl http://localhost:8001
```

结果如下所示。该服务首先返回一个 HTML 页面，里面包含一个空的内层部分（即 `loading` 部分）。然后在 2s 后返回内层部分的 HTML 内容，以及将它显示出来的 JavaScript 代码。

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

流式 SSR 的示例充分利用了 WasmEdge 独特的处理异步网络的能力以及对 ES6 module 的支持（rollup 打包的 JS 文件中包含了 ES6 的模块）。你可以在本书中阅读更多有关 [异步网络](networking.md) 和 [ES6](es6.md) 的知识。
