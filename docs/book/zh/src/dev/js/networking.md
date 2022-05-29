# 网络

QuickJS WasmEdge Runtime 支持 WasmEdge 的[网络 sockets 拓展](https://github.com/second-state/wasmedge_wasi_socket)， 所以 JavaScript 程序也可以在网络上建立 HTTP 连接。此文将向你展示相关的 [HTTP 客户端](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/wasi_http_client.js)和 [HTTP 服务端](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/wasi_http_echo.js)例子.

> WasmEdge 的网络 API 是非阻塞的，所以能够开发出强异步 I/O 交互的应用。当网络请求 handler 正在创建一个对外的请求并等待服务应答的时候，应用仍然可以处理另外一个进来的请求。这让单线程应用可以并发处理多个请求。

## JavaScript 客户端网络通讯例子

以下是一个使用 JavaScript 编写的异步客户端的例子。你可以在 [example_js/wasi_http_client.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/wasi_http_client.js) 中找到源码。以下的代码会向你展示如何发送一个异步 HTTP GET 请求。

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

以上代码可以在等待服务端应答的同时处理其他任务。当服务端返回数据后，`handle_response()` 会被异步调用，处理好数据后就会将内容打印出来。

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

使用以下 CLI 命令，就可以在 WasmEdge runtime 中运行以上的 JavaScript 代码。

```bash
cd example_js
wasmedge --dir .:. ../target/wasm32-wasi/release/wasmedge_quickjs.wasm wasi_http_client.js
```

将会有如下内容被打印出来。

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

以上应用例子发出了两个 HTTP 请求，一个是 `GET` 请求另一个是 `POST` 请求。该应用会异步等待这两个请求的应答数据，并且哪一个先从服务端返回就先会处理哪个。从日志中你可以看到这两个请求的 handlers 是交错执行的。

## JavaScript 网络服务例子

以下的例子是使用 JavaScript 运行了一个监听 8000 端口的 TCP 服务器。接收到的网络请求都会被异步处理。你可以在 [example_js/wasi_net_echo.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/wasi_net_echo.js) 中找到源码。

```javascript
import * as net from 'wasi_net';

async function handle_client(cs) {
  while (true) {
    try {
      let d = await cs.read();
      if (d.byteLength <= 0) {
        break;
      }
      let s = newStringFromUTF8(d);
      cs.write('echo:' + s);
    } catch (e) {
      print(e);
    }
  }
}

async function server_start() {
  let s = new net.WasiTcpServer(8000);
  for (var i = 0; i < 100; i++) {
    let cs = await s.accept();
    handle_client(cs);
  }
}

server_start();
```

调用 `server_start()` 方法会在 8000 端口启动一个监听服务。当一个请求进入，会异步传给 `handle_client()` function 函数处理。这意味着当应用返回应答数据后，它又可以处理下一个进来的请求了。

使用以下 CLI 命令，就可以在 WasmEdge runtime 中运行这段 JavaScript 代码。因为它将作为一个服务运行，你最好是以后台应用的形式启动。

```bash
cd example_js
nohup wasmedge --dir .:. ../target/wasm32-wasi/release/wasmedge_quickjs.wasm wasi_net_echo.js &
```

然后你就可以向它发出网络请求，观察运行效果。

```bash
$ curl -d "WasmEdge" -X POST http://localhost:8000
echo:WasmEdge
```

WasmEdge 的 `wasi_net` 包为 JavaScript 应用提供了一种自适应的动态网络栈。在很多高级用法中，我们基于这个包，设计了很多抽象良好的 API。在下一章节，我们会带着具体的常见应用，向你展示如何处理 HTTP 请求。在 [React 服务器渲染文章](ssr.md)中，我们还将会讨论一下如何基于这种异步网络的 API 来创建一个 React 服务器渲染功能。

## JavaScript HTTP 服务器例子

假如你已经知道服务器的请问和应答都是基于 HTTP 协议的，这里有一些增强方法可以帮到你更好地处理这些请求。你可以在 [example_js/wasi_http_echo.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/wasi_http_echo.js) 中找到源码。

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

`server_start()` 方法会启动一个监听 8000 端口的服务。当请求进来，会被传给 `handle_client()` 方法来处理。当请求是合法的 HTTP 请求，对应的 handler 方法会调用 `handle_req()` 来解析对应的字段，组装新的 HTTP 应答，然后异步把应答数据发送回去。这意味着当应用发送完数据，又能继续处理下一个进来的请求了。

使用以下 CLI 命令，就可以在 WasmEdge runtime 中运行这段 JavaScript 代码。因为它将作为一个服务运行，你最好是以后台应用的形式运行。

```bash
cd example_js
nohup wasmedge --dir .:. ../target/wasm32-wasi/release/wasmedge_quickjs.wasm wasi_http_echo.js &
```

然后你就可以向它发出网络请求，观察运行效果。

```bash
$ curl -d "WasmEdge" -X POST http://localhost:8000
echo:WasmEdge
```

在异步 HTTP 网络编程中，开发者可以安全并高效地在 WasmEdge 中使用 JavaScript 创建强交互的应用，例如数据库驱动的微服务。
