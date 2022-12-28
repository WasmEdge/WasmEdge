# `fetch` API

`fetch` API 被广泛用于浏览器和基于 Node 的 JavaScript 应用程序中，用来通过网络获取内容。WasmEdge QuickJS Runtime 在其非阻塞的 Async Socket API 的基础上支持 `fetch` API。这使得很多 JS 的 API 和模块可以开箱即用。

[example_js/wasi_http_fetch.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/wasi_http_fetch.js) 例子展示了如何在 WasmEdge 中使用 `fetch` API。下面的代码片段显示了一个异步的 HTTP GET 请求。当程序等待和处理 GET 内容的时候，它可以开始另一个请求。

```javascript
import { fetch } from 'http'

async function test_fetch() {
    try {
        let r = await fetch("http://152.136.235.225/get?id=1")
        print('test_fetch\n', await r.text())
    } catch (e) {
        print(e)
    }
}
test_fetch()
```

下面的代码片段显示了如何向远程服务器发送同步的 HTTP POST 请求。

```javascript
async function test_fetch_post() {
    try {
        let r = await fetch("http://152.136.235.225/post", { method: 'post', 'body': 'post_body' })
        print('test_fetch_post\n', await r.text())
    } catch (e) {
        print(e)
    }
}
test_fetch_post()
```

如下是一个异步的 HTTP PUT 请求。

```javascript
async function test_fetch_put() {
    try {
        let r = await fetch("http://152.136.235.225/put",
            {
                method: "put",
                body: JSON.stringify({ a: 1 }),
                headers: { 'Context-type': 'application/json' }
            })
        print('test_fetch_put\n', await r.text())
    } catch (e) {
        print(e)
    }
}
test_fetch_put()
```

要运行这些例子，使用以下 WasmEdge CLI 命令。

```bash
cd example_js
wasmedge --dir .:. ../target/wasm32-wasi/release/wasmedge_quickjs.wasm wasi_http_fetch.js
```

你可以看到打印到控制台的 HTTP 响应。
