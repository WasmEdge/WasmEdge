# Networking for HTTPS

The WasmEdge WASI socket API supports HTTP networking in Wasm apps. In order to achieve the goal of supporting HTTPS requests with the same API as an HTTP request, we now create a Wasmedge plugin using the OpenSSL library. In this chapter, we will give the example of HTTPS requests and explain the design.

## An HTTPS request example

The [example source code](https://github.com/2019zhou/wasmedge_http_req/blob/zhou/httpsreq/examples/get_https.rs) for the HTTPS request is available as follows. The HTTP and HTTPS APIs are the same. The Err messages are presented differently because the HTTP uses the rust code while the HTTPS request uses a wasmedge host function.

```rust
use wasmedge_http_req::request;

fn main() {
    // get request
    let mut writer = Vec::new(); //container for body of a response
    let mut res = request::get("https://httpbin.org/get", &mut writer).unwrap();

    println!("Status: {} {}", res.status_code(), res.reason());
    println!("Headers {}", res.headers());
    //println!("{}", String::from_utf8_lossy(&writer));  // uncomment this line to display the content of writer

    // head request
    res = request::head("https://httpbin.org/head").unwrap();

    println!("Status: {} {}", res.status_code(), res.reason());
    println!("{:?}", res.headers());

    // post request
    writer = Vec::new(); //container for body of a response
    const BODY: &[u8; 27] = b"field1=value1&field2=value2";
    res = request::post("https://httpbin.org/post", BODY, &mut writer).unwrap();

    println!("Status: {} {}", res.status_code(), res.reason());
    println!("Headers {}", res.headers());
    //println!("{}", String::from_utf8_lossy(&writer));  // uncomment this line to display the content of writer
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
wasmedge target/wasm32-wasi/release/get_https.wasm
wasmedge target/wasm32-wasi/release/post_https.wasm
wasmedge target/wasm32-wasi/release/head_https.wasm
```

## Explanation of design

It is observed that the request is first parsed and then added to a stream which sends the parsed request to the server. We remain the first step that is HTTPS request is parsed in the original Rust code. We modify the second step by replacing it with a function called `send_data` that is implemented by the [wasmedge_httpsreq host function](https://github.com/2019zhou/WasmEdge/tree/zhou/httpsreq/plugins/httpsreq). We also let the original Rust source code to process the received content by implementing two additional functions `get_rcv_len` and `get_rcv`.

The advantage of this design is that because the first step is retained, we can use the same API for HTTP and HTTPS request. Besides, one function (i.e. send_data in httpsreq plugin) is needed for all types of requests as long as it is supported in wasmedge_http_req. `send_data` function receives three parameters namely the host, the port and the parsed request. An example for using the send_data function is available as follows.

```Rust
send_data("www.google.com", 443, "GET / HTTP/1.1\nHost: www.google.com\r\nConnection: Close\r\nReferer: https://www.google.com/\r\n\r\n");
```

So, We only do a little change to the original Rust code.

```Rust
if self.inner.uri.scheme() == "https" {
    let buf = &self.inner.parse_msg();
    let body = String::from_utf8_lossy(buf);
    send_data(host, port.into(), &body);

    let output = get_receive();
    let tmp = String::from_utf8(output.rcv_vec).unwrap();
    let res = Response::try_from(tmp.as_bytes(), writer).unwrap();
    return Ok(res);
}
```

To add the host function to a crate that can be used by Rust code, we also implement the [httpsreq module](https://github.com/2019zhou/wasmedge_http_req/blob/zhou/httpsreq/src/httpsreq.rs).

### Implemention of httpsreq host function

The httpsreq host has three functions (i.e. `send_data`, `get_rcv_len` and `get_rcv`)
The `send_data` function uses the OpenSSL library to send the data to the server. The `send_data` function receives three inputs, that is, the host, the port and the parsed request.

```cpp
Expect<void> WasmEdgeHttpsReqSendData::body(Runtime::Instance::MemoryInstance *, uint32_t HostPtr,
                  uint32_t HostLen, uint32_t Port, uint32_t BodyPtr,
                  uint32_t BodyLen);
```

The `get_rcv` function and `get_rcv_len` function pass the received content out of the host function which is later processed by the original Rust code. The get_rcv function receives the pointer while the get_rcv_len function returns the length of the received content.

```cpp
Expect<void> WasmEdgeHttpsReqGetRcv::body(Runtime::Instance::MemoryInstance *, uint32_t BufPtr);

Expect<uint32_t> WasmEdgeHttpsReqGetRcvLen::body(Runtime::Instance::MemoryInstance *);
```

It then [opens the connection](https://github.com/WasmEdge/WasmEdge/blob/14a38e13725965026cd1f404fe552f9c41ad09a3/plugins/httpsreq/httpsreqfunc.cpp#L54-L102). Next, use the `SSL_write` to write the parsed request to the connection. Finally, it receives by using `SSL_read` and prints the receive to the console.
