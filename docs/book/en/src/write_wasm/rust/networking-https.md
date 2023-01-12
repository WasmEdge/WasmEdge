# Networking for HTTPS

The WasmEdge WASI socket API supports HTTP networking in Wasm apps. In order to achieve the goal of supporting HTTPS requests with the same API as an HTTP request, we now create a WasmEdge plugin using the OpenSSL library. In this chapter, we will give the example of HTTPS requests and explain the design.

## Prerequisites

For installation with the installer, you can follow the commands:

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v {{ wasmedge_version }} --plugins wasmedge_httpsreq
```

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

    // add headers and set version
    let uri = Uri::try_from("http://httpbin.org/get").unwrap();
    // let uri = Uri::try_from("https://httpbin.org/get").unwrap(); // uncomment the line for https request

    // add headers to the request
    let mut headers = Headers::new();
    headers.insert("Accept-Charset", "utf-8");
    headers.insert("Accept-Language", "en-US");
    headers.insert("Host", "rust-lang.org");
    headers.insert("Connection", "Close");

    let mut response = Request::new(&uri)
        .headers(headers)
        .send(&mut writer)
        .unwrap();

    println!("{}", String::from_utf8_lossy(&writer));

    // set version
    response = Request::new(&uri)
        .version(HttpVersion::Http10)
        .send(&mut writer)
        .unwrap();

    println!("{}", String::from_utf8_lossy(&writer));
}
```

The following command compiles the Rust program

```bash
# build the wasmedge httpsreq plugin module
sudo apt-get install libssl-dev
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_TESTS=OFF -DWASMEDGE_PLUGIN_HTTPSREQ=On  .. && make -j4

cargo build --target wasm32-wasi --release
```

The following command runs the application in WasmEdge.

```bash
wasmedge target/wasm32-wasi/release/get_https.wasm
wasmedge target/wasm32-wasi/release/post_https.wasm
wasmedge target/wasm32-wasi/release/head_https.wasm
```

## Explanation of design

It is observed that the request is first parsed and then added to a stream which sends the parsed request to the server. We remain the first step that is HTTPS request is parsed in the original Rust code. We modify the second step by replacing it with a function called `send_data` that is implemented by the [wasmedge_httpsreq host function](https://github.com/WasmEdge/WasmEdge/tree/master/plugins/wasmedge_httpsreq). We also let the original Rust source code to process the received content by implementing two additional functions `get_rcv_len` and `get_rcv`.

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

To add the host function to a crate that can be used by Rust code, we also implement the [httpreq module](https://github.com/second-state/wasmedge_http_req).

### Implementation of httpsreq host function

The httpsreq host has three functions (i.e. `send_data`, `get_rcv_len` and `get_rcv`)
The `send_data` function uses the OpenSSL library to send the data to the server. The `send_data` function receives three inputs, that is, the host, the port and the parsed request.

```cpp
Expect<void> WasmEdgeHttpsReqSendData::body(const Runtime::CallingFrame &Frame,
                                            uint32_t HostPtr, uint32_t HostLen,
                                            uint32_t Port, uint32_t BodyPtr,
                                            uint32_t BodyLen)
```

The `get_rcv` function and `get_rcv_len` function pass the received content out of the host function which is later processed by the original Rust code. The get_rcv function receives the pointer while the get_rcv_len function returns the length of the received content.

```cpp
Expect<void> WasmEdgeHttpsReqGetRcv::body(const Runtime::CallingFrame &Frame,
                                          uint32_t BufPtr)

Expect<uint32_t>
WasmEdgeHttpsReqGetRcvLen::body(const Runtime::CallingFrame &)
```

It then opens the connection. Next, use the `SSL_write` to write the parsed request to the connection. Finally, it receives by using `SSL_read` and prints the receive to the console.
