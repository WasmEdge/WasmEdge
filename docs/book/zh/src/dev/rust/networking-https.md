# Https 相关网络通信

WasmEdge WASI socket API 支持 HTTP 网络通信。为了增加对于 HTTPS 网络通信的支持并且与 HTTP 请求使用完全一致的 API ，已经增加了使用 openssl 实现的 WasmEdge 插件。在本章中，我们将给出 HTTPS 请求的示例以及相关代码设计。

## HTTPS 请求的示例

HTTPS 请求示例的[源代码](https://github.com/2019zhou/wasmedge_http_req/blob/zhou/httpsreq/examples/get_https.rs) HTTP 和 HTTPS 所使用的 API 是完全一致的。但是由于 HTTPS 的请求使用 host function 处理，报错信息的将直接输出和 HTTP 的报错输出方式不一致。

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
}
```

使用如下命令来编译 Rust 程序。

```bash
# build the wasmedge httpsreq plugin module
sudo apt-get install libssl-dev
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_TESTS=OFF -DWASMEDGE_PLUGIN_HTTPSREQ=On  .. && make -j4

cargo build --target wasm32-wasi --release
```

使用如下命令在 WasmEdge 中运行程序。

```bash
wasmedge target/wasm32-wasi/release/get_https.wasm
wasmedge target/wasm32-wasi/release/post_https.wasm
wasmedge target/wasm32-wasi/release/head_https.wasm
```

## 代码设计的解释

可以观察到，请求首先被解析，然后被添加到将解析的请求发送到服务器的流中。代码保留了在原始 Rust 代码中解析 HTTPS 请求的第一步。并修改第二步，将其替换为一个名为 send_data 的函数，该函数由[WasmEdge host function](https://github.com/2019zhou/WasmEdge/tree/zhou/httpsreq/plugins/httpsreq))实现。我们也使用了原有的 Rust 代码来处理接收到的代码内容，其中涉及并实现了 get_rcv_len 和 get_rcv 两个函数。

这种设计的好处是，因为保留了第一步，所以我们可以对 HTTP 和 HTTPS 请求使用相同的 API 。 此外，只要 wasmedge_http_req 原来提供这种请求的支持，任何类型的请求都只需要一个函数（即 httpsreq 插件中的 send_data ）。send_data 函数接收三个参数，即主机、端口和解析后的请求。 使用 send_data 函数的示例如下所示。

```Rust
send_data("www.google.com", 443, "GET / HTTP/1.1\nHost: www.google.com\r\nConnection: Close\r\nReferer: https://www.google.com/\r\n\r\n");
```

所以，对于原有的 Rust 代码仅做了几行的的修改。

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
}
```

为了将 host function 添加到 Rust 代码可以使用的 crate ，使用 Rust 实现了[httpsreq module](https://github.com/2019zhou/wasmedge_http_req/blob/zhou/httpsreq/src/httpsreq.rs)

### HTTPS host function 的具体实现

httpsreq 主机有一个名为 send_data 的函数，它使用 openssl 将数据发送到服务器。
send_data 函数接收三个输入，即主机、端口和解析后的请求

```cpp
Expect<void> WasmEdgeHttpsReqSendData::body(const Runtime::CallingFrame &Frame,
                                            uint32_t HostPtr, uint32_t HostLen,
                                            uint32_t Port, uint32_t BodyPtr,
                                            uint32_t BodyLen)
```

get_rcv 函数和 get_rcv_len 函数用来将接受到的内容传出 host function , 并且之后被 Rust sdk 原有逻辑处理。get_rcv 函数需要接受第一个指针，get_rcv_len 会返回接受内容的长度。

```cpp
Expect<void> WasmEdgeHttpsReqGetRcv::body(const Runtime::CallingFrame &Frame,
                                          uint32_t BufPtr)

Expect<uint32_t>
WasmEdgeHttpsReqGetRcvLen::body(const Runtime::CallingFrame &)
```

之后，首先[打开连接](https://github.com/WasmEdge/WasmEdge/blob/14a38e13725965026cd1f404fe552f9c41ad09a3/plugins/httpsreq/httpsreqfunc.cpp#L54-L102) 使用 SSL_write 函数将已经解析的函数写入连接。最后使用 SSL_read 读取接受，将接受打印到控制台。
