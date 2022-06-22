# The `fetch` API

The `fetch` API is widely used in browser and node-based JavaScript applications to fetch content over the network. Building on top of its non-blocking aysnc network socket API, the WasmEdge QuickJS runtime supports the `fetch` API. That makes a lot of JS APIs and modules reusable out of the box.

The [example_js/wasi_http_fetch.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/wasi_http_fetch.js) example demonstrates how to use the `fetch` API in WasmEdge. The code snippet below shows an async HTTP GET. While the program waits for and processes the GET content, it can start another request.

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

The code snippet below shows how to do an sync HTTP POST to a remote server.

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

An async HTTP PUT request is as follows.

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

To run those examples, use the following WasmEdge CLI command.

```bash
cd example_js
wasmedge --dir .:. ../target/wasm32-wasi/release/wasmedge_quickjs.wasm wasi_http_fetch.js
```

You can see the HTTP responses printed to the console.
