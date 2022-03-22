# React SSR

[React](https://reactjs.org/) is very popular JavaScript web UI framework. A React application is "compiled" into an HTML and JavaScript static web site. The web UI is rendered through the generated JavaScript code. However, it is often too slow and resource consuming to execute the complex generated JavaScript entirely in the browser to build the interactive HTML DOM objects. [React Server Side Rendering (SSR)](https://medium.com/jspoint/a-beginners-guide-to-react-server-side-rendering-ssr-bf3853841d55) delegates the JavaScript UI rendering to a server, and have the server stream rendered HTML DOM objects to the browser. The WasmEdge JavaScript runtime provides a lightweight and high performance container to run React SSR functions on edge servers. 

In this article, we will show you how to use the WasmEdge QuickJS runtime to implement a React SSR function.
Compared with the Docker + Linux + nodejs + v8 approach, WasmEdge is much lighter (1% of the footprint) and safer, provides better resource isolation and management, and has similar non-JIT ([safe](https://www.zdnet.com/article/edge-super-duper-secure-mode-turns-off-the-javascript-jit-compiler-for-extra-security/)) performance.

We will start from a complete tutorial to create and deploy a simple React SSR web application from the standard template. Then, we will discuss two modes of SSR: static and streaming rendering. Static rendering is easy to understand and implement. Streaming rendering, on the other hand, provides better user experience since the user can see partial results while waiting in front of the browser. We will walk through the key code snippets for both static and streaming rendering.

* [Getting started](#getting-started)
* [Static rendering code example](#static-rendering)
* [Streaming rendering code example](#streaming-rendering)

## Getting started

### Step 1 — Create the React App

First, use `npx` to create a new React app. Let’s name the app `react-ssr-example`.

```bash
npx create-react-app react-ssr-example
```

Then, `cd` into the directory for the newly created app.

```bash
cd react-ssr-example
```

Start the new app in order to verify the installation.

```bash
npm start
```

You should see the example React app displayed in your browser window. At this stage, the app is rendered in the browser. The browser runs the generated React JavaScript to build the HTML DOM UI.

Now in order to prepare for SSR, you will need to make some changes to the app's `index.js` file. Change ReactDOM's `render` method to `hydrate` to indicate to the DOM renderer that you intend to rehydrate the app after it is rendered on the server.
Replace the contents of the `index.js` file with the following.

```javascript
import React from 'react';
import ReactDOM from 'react-dom';
import App from './App';
ReactDOM.hydrate(
  <React.StrictMode>
    <App />
  </React.StrictMode>,
  document.getElementById('root')
);
```

Note: you should import `React` redundantly in the `src/App.js`, so the server will recognize it.

```js
import React from 'react';
//...
```

That concludes setting up the application, you can move on to setting up the server-side rendering functions.

### Step 2 — Create an WasmEdge QuickJS Server and Render the App Component

Now that you have the app in place, let’s set up a server that will render the HTML DOM by running the React JavaScript and then send the rendered elements to the browser. We will use WasmEdge as a secure, high-performance and lightweight container to run React JavaScript.

Create a new `server` directory in the project's root directory.

```bash
mkdir server
```

Then, inside the `server` directory, create a new `index.js` file with the server code.

```javascript
import * as React from 'react';
import ReactDOMServer from 'react-dom/server';
import * as std from 'std';
import * as http from 'wasi_http';
import * as net from 'wasi_net';

import App from '../src/App.js';

async function handle_client(cs) {
	print('open:', cs.peer());
	let buffer = new http.Buffer();

	while (true) {
		try {
			let d = await cs.read();
			if (d == undefined || d.byteLength <= 0) {
				return;
			}
			buffer.append(d);
			let req = buffer.parseRequest();
			if (req instanceof http.WasiRequest) {
				handle_req(cs, req);
				break;
			}
		} catch (e) {
			print(e);
		}
	}
	print('end:', cs.peer());
}

function enlargeArray(oldArr, newLength) {
	let newArr = new Uint8Array(newLength);
	oldArr && newArr.set(oldArr, 0);
	return newArr;
}

async function handle_req(s, req) {
	print('uri:', req.uri)

	let resp = new http.WasiResponse();
	let content = '';
	if (req.uri == '/') {
		const app = ReactDOMServer.renderToString(<App />);
		content = std.loadFile('./build/index.html');
		content = content.replace('<div id="root"></div>', `<div id="root">${app}</div>`);
	} else {
		let chunk = 1000; // Chunk size of each reading
		let length = 0; // The whole length of the file
		let byteArray = null; // File content as Uint8Array
		
		// Read file into byteArray by chunk
		let file = std.open('./build' + req.uri, 'r');
		while (true) {
			byteArray = enlargeArray(byteArray, length + chunk);
			let readLen = file.read(byteArray.buffer, length, chunk);
			length += readLen;
			if (readLen < chunk) {
				break;
			}
		}
		content = byteArray.slice(0, length).buffer;
		file.close();
	}
	let contentType = 'text/html; charset=utf-8';
	if (req.uri.endsWith('.css')) {
		contentType = 'text/css; charset=utf-8';
	} else if (req.uri.endsWith('.js')) {
		contentType = 'text/javascript; charset=utf-8';
	} else if (req.uri.endsWith('.json')) {
		contentType = 'text/json; charset=utf-8';
	} else if (req.uri.endsWith('.ico')) {
		contentType = 'image/vnd.microsoft.icon';
	} else if (req.uri.endsWith('.png')) {
		contentType = 'image/png';
	}
	resp.headers = {
		'Content-Type': contentType
	};

	let r = resp.encode(content);
	s.write(r);
}

async function server_start() {
	print('listen 8002...');
	try {
		let s = new net.WasiTcpServer(8002);
		for (var i = 0; ; i++) {
			let cs = await s.accept();
			handle_client(cs);
		}
	} catch (e) {
		print(e);
	}
}

server_start();
```

The server renders the `<App>` component, and then sends the rendered HTML string back to the browser. Three important things are taking place here.

- ReactDOMServer's `renderToString` is used to render the `<App/>` to an HTML string.
- The `index.html` file from the app's `build` output directory is loaded as a template. The app's content is injected into the `<div>` element with an id of `"root"`. It is then sent back as HTTP response.
- Other files from the `build` directory are read and served as needed at the requests of the browser.


### Step 3 — Build and deploy

For the server code to work, you will need to bundle and transpile it. In this section, we will show you how to use webpack and Babel. In this next section, we will demonstrate an alternative (and potentially easier) approach using rollup.js.

Create a new Babel configuration file named `.babelrc.json` in the project's root directory and add the `env` and `react-app` presets.

```json
{
  "presets": [
    "@babel/preset-env",
    "@babel/preset-react"
  ]
}
```

Create a webpack config for the server that uses Babel Loader to transpile the code. Start by creating the `webpack.server.js` file in the project's root directory.

```js
const path = require('path');
module.exports = {
	entry: './server/index.js',
	externals: [
		{"wasi_http": "wasi_http"},
		{"wasi_net": "wasi_net"},
		{"std": "std"}
	],
	output: {
		path: path.resolve('server-build'),
		filename: 'index.js',
		chunkFormat: "module",
		library: {
			type: "module"
		},
	},
	experiments: {
		outputModule: true
	},
	module: {
		rules: [
			{
				test: /\.js$/,
				use: 'babel-loader'
			},
			{
				test: /\.css$/,
				use: ["css-loader"]
			},
			{
				test: /\.svg$/,
				use: ["svg-url-loader"]
			}
		]
	}
};
```

With this configuration, the transpiled server bundle will be output to the `server-build` folder in a file called `index.js`.

Next, add the `svg-url-loader` package by entering the following commands in your terminal.

```bash
npm install svg-url-loader --save-dev
```

This completes the dependency installation and webpack and Babel configuration.

Now, revisit `package.json` and add helper npm scripts. Add `dev:build-server`, `dev:start-server` scripts to the `package.json` file to build and serve the SSR application.

```json
"scripts": {
  "dev:build-server": "NODE_ENV=development webpack --config webpack.server.js --mode=development",
  "dev:start-server": "wasmedge --dir .:. wasmedge_quickjs.wasm ./server-build/index.js",
  // ...
},
```

The `dev:build-server` script sets the environment to `"development"` and invokes webpack with the configuration file you created earlier.

Note: The `dev:start-server` script invokes `wasmedge` to serve the built output. The `wasmedge_quickjs.wasm` is compiled from QuickJS runtime.
You can compile it using the following commands and copy the output wasm file to the root of current project's root directory.

```bash
DIR=`pwd`
cd ..
git clone https://github.com/second-state/wasmedge-quickjs
cd wasmedge-quickjs
cargo build --target wasm32-wasi --release
cp target/wasm32-wasi/release/wasmedge_quickjs.wasm $DIR
```

Now you can run the following commands to build the client-side app, bundle and transpile the server code, and start up the server on `:8002`.

```bash
npm run build
npm run dev:build-server
npm run dev:start-server
```

Open http://localhost:8002/ in your web browser and observe your server-side rendered app.

Previously, the HTML source in the browser is simply the template with SSR placeholders.

```html
Output
<div id="root"></div>
```

Now, with the SSR function running on the server, the HTML source in the browser is as follows.

```html
Output
<div id="root"><div class="App" data-reactroot="">...</div></div>
```

### Step 4 (alternative) -- build and deploy with rollup.js

Alternatively, you could use the [rollup.js](https://rollupjs.org/guide/en/) tool to [package all application components and library modules](npm.md) into a single file for WasmEdge to execute.

Create a rollup config for the server that uses Babel Loader to transpile the code. Start by creating the `rollup.config.js` file in the project's root directory.

```js
const {babel} = require('@rollup/plugin-babel');
const nodeResolve = require('@rollup/plugin-node-resolve');
const commonjs = require('@rollup/plugin-commonjs');
const replace = require('@rollup/plugin-replace');

const globals = require('rollup-plugin-node-globals');
const builtins = require('rollup-plugin-node-builtins');
const plugin_async = require('rollup-plugin-async');
const css = require("rollup-plugin-import-css");
const svg = require('rollup-plugin-svg');

const babelOptions = {
	babelrc: false,
	presets: [
		'@babel/preset-react'
	],
	babelHelpers: 'bundled'
};

module.exports = [
	{
		input: './server/index.js',
		output: {
			file: 'server-build/index.js',
			format: 'esm',
		},
		external: [ 'std', 'wasi_net','wasi_http'],
		plugins: [
			plugin_async(),
			babel(babelOptions),
			nodeResolve({preferBuiltins: true}),
			commonjs({ignoreDynamicRequires: false}),
			css(),
			svg({base64: true}),
			globals(),
			builtins(),
			replace({
				preventAssignment: true,	
				'process.env.NODE_ENV': JSON.stringify('production'),
				'process.env.NODE_DEBUG': JSON.stringify(''),
			}),
		],
	},
];
```

With this configuration, the transpiled server bundle will be output to the `server-build` folder in a file called `index.js`.

Next, add the dependent packages to the `package.json` then install with `npm`.

```json
  "devDependencies": {
    //...
    "@rollup/plugin-babel": "^5.3.0",
    "@rollup/plugin-commonjs": "^21.0.1",
    "@rollup/plugin-node-resolve": "^7.1.3",
    "@rollup/plugin-replace": "^3.0.0",
    "rollup": "^2.60.1",
    "rollup-plugin-async": "^1.2.0",
    "rollup-plugin-import-css": "^3.0.3",
    "rollup-plugin-node-builtins": "^2.1.2",
    "rollup-plugin-node-globals": "^1.4.0",
    "rollup-plugin-svg": "^2.0.0"
  }
```

```bash
npm install
```

This completes the dependency installation and rollup configuration.

Now, revisit `package.json` and add helper npm scripts. Add `dev:build-server`, `dev:start-server` scripts to the `package.json` file to build and serve the SSR application.

```json
"scripts": {
  "dev:build-server": "rollup -c rollup.config.js",
  "dev:start-server": "wasmedge --dir .:. wasmedge_quickjs.wasm ./server-build/index.js",
  // ...
},
```

The `dev:build-server` script invokes rollup with the configuration file you created earlier.

Note: The `dev:start-server` script invokes `wasmedge` to serve the built output. The `wasmedge_quickjs.wasm` is compiled from QuickJS runtime.
You can compile it using the following commands and copy the output wasm file to the root of current project's root directory.

```bash
DIR=`pwd`
cd ..
git clone https://github.com/second-state/wasmedge-quickjs
cd wasmedge-quickjs
cargo build --target wasm32-wasi --release
cp target/wasm32-wasi/release/wasmedge_quickjs.wasm $DIR
```

Now you can run the following commands to build the client-side app, bundle and transpile the server code, and start up the server on `:8002`.

```bash
npm run build
npm run dev:build-server
npm run dev:start-server
```

Open http://localhost:8002/ in your web browser and observe your server-side rendered app.

Previously, the HTML source in the browser is simply the template with SSR placeholders.

```html
Output
<div id="root"></div>
```

Now, with the SSR function running on the server, the HTML source in the browser is as follows.

```html
Output
<div id="root"><div class="App" data-reactroot="">...</div></div>
```

In the next two sections, we will dive deeper into the source code for two ready-made sample applications for static and streaming rendering of SSR.


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
