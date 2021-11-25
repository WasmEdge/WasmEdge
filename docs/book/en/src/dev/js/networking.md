# Networking sockets

The QuickJS WasmEdge Runtime supports the WasmEdge [networking socket extension](https://github.com/second-state/wasmedge_wasi_socket) so that the JavaScript programs can make HTTP connections to the Internet. This article will show you both [HTTP Client](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/http_demo.js) and [HTTP Server](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/http_server_demo.js) examples.

## A JavaScript networking client example

Below is an example of JavaScript running a HTTP client. You could find the code in [example_js/http_demo.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/http_demo.js).

```
let r = GET("http://18.235.124.214/get?a=123",{"a":"b","c":[1,2,3]})
print(r.status)
    
let headers = r.headers
print(JSON.stringify(headers))let body = r.body;
let body_str = new Uint8Array(body)
print(String.fromCharCode.apply(null,body_str))
```

To run the JavaScript in the WasmEdge runtime, you can do this on the CLI.

```
$ cd example_js
$ wasmedge --dir .:. ../target/wasm32-wasi/release/wasmedge_quickjs.wasm http_demo.js
```

You should now see the HTTP GET result printed on the console.

## A JavaScript networking server example

Below is an example of JavaScript running a HTTP server listening at port 3000. You could find the code in [example_js/http_server_demo.js](https://github.com/second-state/wasmedge-quickjs/blob/main/example_js/http_server_demo.js).

```
import {HttpServer} from 'http'

let http_server = new HttpServer('0.0.0.0:8000')
print('listen on 0.0.0.0:8000')

while(true){
    http_server.accept((request)=>{
        let body = request.body
        let body_str = String.fromCharCode.apply(null,new Uint8Array(body))
        print(JSON.stringify(request),'\n body_str:',body_str)

        return {
            status:200,
            header:{'Content-Type':'application/json'},
            body:'echo:'+body_str
        }
    });
}
```

To run the JavaScript in the WasmEdge runtime, you can do this on the CLI. Since it is a server, you should run it in the background.

```
$ cd example_js
$ nohup wasmedge --dir .:. ../target/wasm32-wasi/release/wasmedge_quickjs.wasm http_server_demo.js &
```

Then you can test the server by querying it over the network.

```
$ curl -d "WasmEdge" -X POST http://localhost:8000
echo:WasmEdge
```

You should now see the HTTP POST body printed on the console.
