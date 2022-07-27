# Https 相关网络通信

Wasmedge WASI socket API支持HTTP网络通信。为了增加对于HTTPS网络通信的支持, 已经增加了使用openssl实现的Wasmedge插件。在本章中，我们将给出HTTPS请求的示例以及相关代码设计。

## HTTPS请求的示例

HTTPS请求示例的[源代码](https://github.com/2019zhou/wasmedge_http_req/blob/zhou/httpsreq/examples/https.rs)

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

使用如下命令来编译 Rust 程序。
```bash
# build the wasmedge httpsreq plugin module
sudo apt-get install libssl-dev
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_TESTS=OFF -DWASMEDGE_PLUGIN_HTTPSREQ=true  .. && make -j4

cargo build --target wasm32-wasi --release
```

使用如下命令在 WasmEdge 中运行程序。

```bash
wasmedge target/wasm32-wasi/release/http_client.wasm
```

## 代码设计的解释
可以观察到，请求首先被解析，然后被添加到将解析的请求发送到服务器的流中。 代码保留了在原始 Rust 代码中解析 HTTPS 请求的第一步。 并修改第二步，将其替换为一个名为 send_data 的函数，该函数由[Wasmedge host function](https://github.com/2019zhou/WasmEdge/tree/zhou/httpsreq/plugins/httpsreq))实现。

这种设计的好处是，因为保留了第一步，所以我们可以对 HTTP 和 HTTPS 请求使用相同的 API。 此外，只要 wasmedge_http_req原来提供这种请求的支持，任何类型的请求都只需要一个函数（即 httpsreq 插件中的 send_data）。send_data 函数接收三个参数，即主机、端口和解析后的请求。 使用 send_data 函数的示例如下所示。

```Rust
send_data("www.google.com", 443, "GET / HTTP/1.1\nHost: www.google.com\r\nConnection: Close\r\nReferer: https://www.google.com/\r\n\r\n");
```

所以，对于原有的Rust代码仅做了三行的的修改。
```Rust
if self.inner.uri.scheme() == "https" {
    let buf = &self.inner.parse_msg();
    let body = String::from_utf8_lossy(buf);
    send_data(host, port.into(), &body);
    return Err(error::Error::Parse(error::ParseErr::HostFunction));
}
```

为了将host function添加到 Rust 代码可以使用的 crate，使用Rust实现了[httpsreq module](https://github.com/2019zhou/wasmedge_http_req/blob/zhou/httpsreq/src/httpsreq.rs)

### HTTPS host function的具体实现
httpsreq 主机有一个名为 send_data 的函数，它使用 openssl 将数据发送到服务器。
send_data 函数接收三个输入，即主机、端口和解析后的请求

```
SendData::body( Runtime::Instance::MemoryInstance *MemInst,
               uint32_t HostPtr, uint32_t HostLen, uint32_t Port,
               uint32_t BodyPtr, uint32_t BodyLen)
```

之后，首先[打开连接](https://github.com/WasmEdge/WasmEdge/blob/14a38e13725965026cd1f404fe552f9c41ad09a3/plugins/httpsreq/httpsreqfunc.cpp#L54-L102) 使用SSL_write函数将已经解析的函数写入连接。最后使用SSL_read读取接受，将接受打印到控制台。