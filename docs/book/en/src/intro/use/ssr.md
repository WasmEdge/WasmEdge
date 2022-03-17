# Server-side rendering

> Server-side rendering (SSR) is a popular technique for rendering a client-side single page application (SPA) on the server and then sending a fully rendered page to the client. This allows for dynamic components to be served as static HTML markup.
>
> This approach can be useful for search engine optimization (SEO) when indexing does not handle JavaScript properly. It may also be beneficial in situations where downloading a large JavaScript bundle is impaired by a slow network.

The two paragraphs above are quoted from [How to Enable Server-Side Rendering for a React App](https://www.digitalocean.com/community/tutorials/react-server-side-rendering).
When you have completed that tutorial, you will have a server-side application running on Nodejs. It means if you want to deploy your application to the cloud, the best or the most popular practice should be put it into the container and manage using k8s.

But since WebAssembly is more efficient than the container, is there any solution to implement SSR using WasmEdge? The answer is yes. This article will explore how to run a server-side application on WasmEdge.

## A create-react-app application

### Step 1 — Creating the React App

First, use npx to start up a new React app using the latest version of Create React App.

Let’s call the app, `react-ssr-example`:
```bash
npx create-react-app react-ssr-example
```

Then, `cd` into the new directory:
```bash
cd react-ssr-example
```

Finally, start the new client-side app in order to verify the installation:
```bash
npm start
```

You will observe an example React app displayed in your browser window.

In the app’s index.js file, you will use ReactDOM’s hydrate method instead of render to indicate to the DOM renderer that you intend to rehydrate the app after a server-side render.
So replace the contents of the index.js file with the following code:
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

> Note: You should import `React` redundantly in the `src/App.js`, so the server will recognize it.
```js
import React from 'react';
//...
```
`src/App.js`

That concludes setting up the client-side, you can move on to setting up the server-side.

### Step 2 — Creating an WasmEdge QuickJS Server and Rendering the App Component

Now that you have the app in place, let’s set up a server that will send along a rendered version. You will use WasmEdge QuickJS for the server.

Next, create a new server directory in the project’s root directory:
```bash
mkdir server
```

Then, inside of the server directory, create a new index.js file that will contain the server code:
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

async function handle_req(s, req) {
	print('uri:', req.uri);

	let resp = new http.WasiResponse();
	let content = '';
	if (req.uri == '/') {
		const app = ReactDOMServer.renderToString(<App />);
		content = std.loadFile('./build/index.html');
		content = content.replace('<div id="root"></div>', `<div id="root">${app}</div>`);
	} else if (req.uri.indexOf('/static') === 0) {
		content = std.loadFile('./build' + req.uri);
	} else {
		content = std.loadFile('./public' + req.uri);
	}
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

It is possible to import the `<App>` component from the client app directly from the server.

Three important things are taking place here:

- ReactDOMServer’s renderToString is used to render the app to a static HTML string.
- The static index.html file from the built client app is read. The app’s static content is injected into the `<div>` with an id of `"root"`. This is sent as a response to the request.
- Contents from the build directory is loaded dynamically and served as static files.


### Step 3 — Configuring webpack, Babel, and npm Scripts

For the server code to work, you will need to bundle and transpile it, using webpack and Babel. To accomplish this.

Next, create a new Babel configuration file named `.babelrc.json` in the project’s root directory and add the env and react-app presets:
```json
{
  "presets": [
    "@babel/preset-env",
    "@babel/preset-react"
  ]
}
```

Now, create a webpack config for the server that uses Babel Loader to transpile the code. Start by creating the `webpack.server.js` file in the project’s root directory:
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

Let's add the `svg-url-loader` package by entering the following command in your terminal:
```bash
npm install svg-url-loader --save-dev
```

This completes the dependency installation and webpack and Babel configuration.

Now, revisit `package.json` and add helper npm scripts. Add `dev:build-server`, `dev:start-server` scripts to the `package.json` file to build and serve the SSR application:
```json
"scripts": {
  "dev:build-server": "NODE_ENV=development webpack --config webpack.server.js --mode=development",
  "dev:start-server": "wasmedge --dir .:. wasmedge_quickjs.wasm ./server-build/index.js",
  // ...
},
```

The `dev:build-server` script sets the environment to `"development"` and invokes webpack with the configuration file you created earlier.

The `dev:start-server` script invokes `wasmedge` to serve the built output. The `wasmedge_quickjs.wasm` is compiled from QuickJS runtime.
You can compile it by invoking the following commands and copy the output wasm file to the root of current project's root directory:
```bash
DIR=`pwd`
cd ..
git clone https://github.com/second-state/wasmedge-quickjs
cd wasmedge-quickjs
cargo build --target wasm32-wasi --release
cp target/wasm32-wasi/release/wasmedge_quickjs.wasm $DIR
```

Now you can run the followings to build the client-side app, bundle and transpile the server code, and start up the server on `:8002`:
```bash
npm run build
npm run dev:build-server
npm run dev:start-server
```

Open http://localhost:8002/ in your web browser and observe your server-side rendered app.

Previously, viewing the source code revealed:
```html
Output
<div id="root"></div>
```

But now, with the changes you made, the source code reveals:
```html
Output
<div id="root"><div class="App" data-reactroot="">...</div></div>
```