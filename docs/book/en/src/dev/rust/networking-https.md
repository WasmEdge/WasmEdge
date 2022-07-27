# Networking for Https

The WasmEdge WASI socket API supports HTTP networking in Wasm apps. In order to achieve the goal of supporting HTTPS requests, we now create a Wasmedge plugin using openssl library. In this chapter, we will give the example of HTTPs requests and explain the design.

## An HTTPS request example

The [source code](https://github.com/2019zhou/wasmedge_http_req/blob/zhou/httpsreq/examples/https.rs) for the HTTPS request is available as follows.

```rust
use wasmedge_http_req::request;

fn main() {
    let mut writer = Vec::new(); //container for body of a response

    // get request
    request::get("https://httpbin.org", &mut writer).unwrap();

    // head request
    request::head("https://httpbin.org").unwrap();

    // post request
    let mut writer = Vec::new(); //container for body of a response
    const BODY: &[u8; 27] = b"field1=value1&field2=value2";
    request::post("https://httpbin.org/post", BODY, &mut writer).unwrap();

    //no res, res.status_code(), res.headers(), res.reason()
}
```

The following command compiles the Rust program

```bash
# build the wasmedge httpsreq plugin module
sudo apt-get install libssl-dev
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_TESTS=OFF -DWASMEDGE_PLUGIN_HTTPSREQ=true  .. && make -j4

cargo build --target wasm32-wasi --release
```

The following command runs the application in WasmEdge.

```bash
wasmedge target/wasm32-wasi/release/https.wasm
```

## Explanation of design
It is observed that the request is first parsed and then added to a stream which sends the parsed request to the server. We remain the first step that is HTTPs request is parsed in the original Rust code. We modify the second step by replacing it with a function called send_data that is implemented by the [wasmedge_httpsreq host function](https://github.com/2019zhou/WasmEdge/tree/zhou/httpsreq/plugins/httpsreq).

The advantage of this design is that because the first step is retained, we can use the same API for HTTP and HTTPS request. Besides, one function (i.e. send_data in httpsreq plugin) is needed for all types of requests as long as it is supported in wasmedge_http_req. ```send_data``` function receives three parameters namely the host, the port and the parsed request. An example for using the send_data function is available as follows.

```Rust
send_data("www.google.com", 443, "GET / HTTP/1.1\nHost: www.google.com\r\nConnection: Close\r\nReferer: https://www.google.com/\r\n\r\n");
```

So, We only do a 3 lines change to the original Rust code. 

```Rust
if self.inner.uri.scheme() == "https" {
    let buf = &self.inner.parse_msg();
    let body = String::from_utf8_lossy(buf);
    send_data(host, port.into(), &body);
    return Err(error::Error::Parse(error::ParseErr::HostFunction));
}
```

To add the host function to a crate that can be used by Rust code, we also implement the [httpreq module](https://github.com/2019zhou/wasmedge_http_req/blob/zhou/httpsreq/src/httpsreq.rs).


### Implemention of httpsreq host function
The httpsreq host has one function called send_data which uses openssl to send the data to the server.
The send_data function receives three inputs, that is, the host, the port and the parsed request.

```
SendData::body( Runtime::Instance::MemoryInstance *MemInst,
               uint32_t HostPtr, uint32_t 
               HostLen, uint32_t Port,
               uint32_t BodyPtr, uint32_t BodyLen)
```

It then [opens the connection](https://github.com/WasmEdge/WasmEdge/blob/14a38e13725965026cd1f404fe552f9c41ad09a3/plugins/httpsreq/httpsreqfunc.cpp#L54-L102). Next, use the ```SSL_write``` to write the parsed request to connection. Finally, it receives by using ```SSL_read``` and prints the receive to the console.




