# Async networking apps

The QuickJS WasmEdge Runtime supports the WasmEdge [networking socket extension](https://github.com/second-state/wasmedge_wasi_socket) so that the JavaScript programs can make HTTP connections to the Internet. This article will show you both [HTTP Client](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/wasi_http_client.js) and [HTTP Server](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/wasi_http_echo.js) examples.

> The networking API in WasmEdge is non-blocking and hence supports asynchronous I/O intensive applications. With this API, the JavaScript program can open multiple connections concurrently. It polls those connections, or registers async callback functions, to process data whenever data comes in, without waiting for any one connection to complete its data transfer. That allows the single-threaded application to handle multiple multiple concurrent requests.

## A JavaScript networking client example

Below is an example of JavaScript running an async HTTP client. You could find the code in [example_js/wasi_http_client.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/wasi_http_client.js). The code below shows how to make an async HTTP GET request.

```javascript
async function get_test() {
  try {
    let ss = await net.connect('152.136.235.225:80');
    let req = new http.WasiRequest();
    req.headers = { 'Host': '152.136.235.225' };
    req.uri = '/get?a=123';
    req.method = 'GET';
    ss.write(req.encode());
    print('wait get');
    await handle_response(ss);
    print('get end');

  } catch (e) {
    print('catch:', e);
  }
}
```

The program can open multiple requests while waiting for the servers to respond. Once a server responds, the `handle_response()` function is called asynchronously to process the response and to print out the content.

```javascript
async function handle_response(s) {
  let buf = new http.Buffer();
  let resp = undefined;
  while (true) {
    buf.append(await s.read());
    if (resp == undefined) {
      resp = buf.parseResponse();
    }
    if (resp instanceof http.WasiResponse) {
      let resp_length = resp.bodyLength;
      if (typeof (resp_length) === "number") {
        if (buf.length >= resp.bodyLength) {
          print('resp.body');
          print(newStringFromUTF8(buf.buffer));
          break;
        }
      } else {
        throw new Error('no support');
      }
    }
  }
}
```

To run the JavaScript in the WasmEdge runtime, you can do this on the CLI.

```bash
cd example_js
wasmedge --dir .:. ../target/wasm32-wasi/release/wasmedge_quickjs.wasm wasi_http_client.js
```

The results printed to the console are as follows.

```json
{
  "args": {
    "a": "123"
  }, 
  "data": "hello", 
  "files": {}, 
  "form": {}, 
  "headers": {
    "Content-Length": "5", 
    "Host": "152.136.235.225"
  }, 
  "json": null, 
  "origin": "20.124.39.106", 
  "url": "http://152.136.235.225/post?a=123"
}
```

The demo app does two HTTP requests. One is `GET` and the other is `POST`. The app waits for the responses from those two requests asynchronously, and processes them as they come in. From the console log, you can see how the two request handlers are interweaved.

## A JavaScript networking server example

Below is an example of JavaScript running a TCP server listening at port 8000. The incoming requests are handled asynchronously. You could find the code in [example_js/wasi_net_echo.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/wasi_net_echo.js).

```javascript
import * as net from 'wasi_net';

async function handle_client(cs) {
  try {
    while (true) {
      let d = await cs.read();
      if (d == undefined || d.byteLength <= 0) {
        break;
      }
      let s = newStringFromUTF8(d);
      cs.write('echo:' + s);
    }
  } catch (e) {
    print(e);
  }
}

async function server_start() {
  print('listen 8000 ...');
  try {
    let s = new net.WasiTcpServer(8000);
    for (var i = 0; i < 100; i++) {
      let cs = await s.accept();
      handle_client(cs);
    }
  } catch (e) {
    print(e)
  }
}

server_start();
```

The `server_start()` function starts the server at port 8000. When a request comes in, it accepts immediately and calls the `handle_client()` function to process it asynchronously when the request data is received later. While the `handle_client()` is waiting for the data to arrive from the network, the app could accept another request concurrently.

To run the JavaScript in the WasmEdge runtime, you can do this on the CLI. Since it is a server, you should run it in the background.

```bash
cd example_js
nohup wasmedge --dir .:. ../target/wasm32-wasi/release/wasmedge_quickjs.wasm wasi_net_echo.js &
```

Then you can test the server by querying it over the network.

```bash
$ curl -d "WasmEdge" -X POST http://localhost:8000
echo:WasmEdge
```

The `wasi_net` package provides a flexible asynchronous networking stack for JavaScript applications in WasmEdge. We are wrap it in high-level APIs for more advanced use cases. In the next section, we will show you how to handle HTTP requests with ease. In the [React SSR article](ssr.md), we will discuss how to create a React stream SSR server with the async networking API.

## A JavaScript HTTP server example

If you already knew the server's requests and responses are in the HTTP protocol, there are additional helper functions to help you handle these requests. You could find the code in [example_js/wasi_http_echo.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/wasi_http_echo.js).

```javascript
import * as http from 'wasi_http';
import * as net from 'wasi_net';

async function handle_client(cs, handler_req) {
  let buffer = new http.Buffer();

  while (true) {
    try {
      let d = await cs.read();
      if (d.byteLength <= 0) {
        return;
      }
      buffer.append(d);
      let req = buffer.parseRequest();
      if (req instanceof http.WasiRequest) {
        handler_req(cs, req);
        break;
      }
    } catch (e) {
      print(e);
    }
  }
}

function handler_req(cs, req) {
  print("version=", req.version);
  print("uri=", req.uri);
  print("method=", req.method);
  print("headers=", Object.keys(req.headers));
  print("body=", newStringFromUTF8(req.body));

  let resp = new http.WasiResponse();
  let body = 'echo:' + newStringFromUTF8(req.body);
  let r = resp.encode(body);
  cs.write(r);
}

async function server_start() {
  try {
    let s = new net.WasiTcpServer(8000);
    for (var i = 0; i < 100; i++) {
      let cs = await s.accept();
      try {
        handle_client(cs, handler_req);
      } catch (e) {
        print(e);
      }
    }
  } catch (e) {
    print(e);
  }
}

server_start();
```

The `server_start()` function starts the server at port 8000. When a request comes in, it accepts immediately and calls the `handle_client()` async function to process the request data when the data is received later. Once the request is validated as an HTTP request, the handler function in turn calls `handle_req()` to parse the fields in the HTTP request, compose a HTTP reponse, and then send the response back. While the program is waiting for the request data to arrive from the network, it can accept another request concurrently.

To run the JavaScript in the WasmEdge runtime, you can do this on the CLI. Since it is a server, you should run it in the background.

```bash
cd example_js
nohup wasmedge --dir .:. ../target/wasm32-wasi/release/wasmedge_quickjs.wasm wasi_http_echo.js &
```

Then you can test the server by querying it over the network.

```bash
$ curl -d "WasmEdge" -X POST http://localhost:8000
echo:WasmEdge
```

With async HTTP networking, developers can create I/O intensive applications, such as database-driven microservices, in JavaScript and run them safely and efficiently in WasmEdge.
