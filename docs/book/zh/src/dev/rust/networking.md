# 简单网络通信

[wasmedge_wasi_socket](https://github.com/second-state/wasmedge_wasi_socket) 让 Rust 开发者可以创建简单的网络通信应用程序，然后将它编译为在 WasmEdge Runtime 中运行的 WebAssembly。WasmEdge 的一项关键特性便是对非阻塞套接字的支持，可以让一个单线程的 WASM 应用程序处理并发的网络请求。比如，当程序在等待一个连接传输的数据时，它可以开始或处理另一个连接。

在本章节中，我们将从一个简单的 HTTP 客户端以及服务器示例开始。然后在下个章节中，我们会介绍更为复杂的非阻塞示例。也会介绍对于 HTTPS 请求的支持。

## HTTP 客户端示例

HTTP 客户端的[源代码](https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples/http_client) 如下。

```rust
use wasmedge_http_req::request;

fn main() {
  let mut writer = Vec::new(); //container for body of a response
  let res = request::get("http://127.0.0.1:1234/get", &mut writer).unwrap();

  println!("GET");
  println!("Status: {} {}", res.status_code(), res.reason());
  println!("Headers {}", res.headers());
  println!("{}", String::from_utf8_lossy(&writer));

  let mut writer = Vec::new(); //container for body of a response
  const BODY: &[u8; 27] = b"field1=value1&field2=value2";
  // let res = request::post("https://httpbin.org/post", BODY, &mut writer).unwrap();
  // no https , no dns
  let res = request::post("http://127.0.0.1:1234/post", BODY, &mut writer).unwrap();

  println!("POST");
  println!("Status: {} {}", res.status_code(), res.reason());
  println!("Headers {}", res.headers());
  println!("{}", String::from_utf8_lossy(&writer));
}
```

使用如下命令来编译 Rust 程序。

```bash
cargo build --target wasm32-wasi --release
```

使用如下命令在 WasmEdge 中运行程序。

```bash
wasmedge target/wasm32-wasi/release/http_client.wasm
```

## HTTP 服务器示例

HTTP 服务器的[源代码](https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples/http_server) 如下。

```rust
use bytecodec::DecodeExt;
use httpcodec::{HttpVersion, ReasonPhrase, Request, RequestDecoder, Response, StatusCode};
use std::io::{Read, Write};
#[cfg(feature = "std")]
use std::net::{Shutdown, TcpListener, TcpStream};
#[cfg(not(feature = "std"))]
use wasmedge_wasi_socket::{Shutdown, TcpListener, TcpStream};

fn handle_http(req: Request<String>) -> bytecodec::Result<Response<String>> {
  Ok(Response::new(
    HttpVersion::V1_0,
    StatusCode::new(200)?,
    ReasonPhrase::new("")?,
    format!("echo: {}", req.body()),
  ))
}

fn handle_client(mut stream: TcpStream) -> std::io::Result<()> {
  let mut buff = [0u8; 1024];
  let mut data = Vec::new();

  loop {
    let n = stream.read(&mut buff)?;
    data.extend_from_slice(&buff[0..n]);
    if n < 1024 {
      break;
    }
  }

  let mut decoder =
    RequestDecoder::<httpcodec::BodyDecoder<bytecodec::bytes::Utf8Decoder>>::default();

  let req = match decoder.decode_from_bytes(data.as_slice()) {
    Ok(req) => handle_http(req),
    Err(e) => Err(e),
  };

  let r = match req {
    Ok(r) => r,
    Err(e) => {
      let err = format!("{:?}", e);
      Response::new(
        HttpVersion::V1_0,
        StatusCode::new(500).unwrap(),
        ReasonPhrase::new(err.as_str()).unwrap(),
        err.clone(),
      )
    }
  };

  let write_buf = r.to_string();
  stream.write(write_buf.as_bytes())?;
  stream.shutdown(Shutdown::Both)?;
  Ok(())
}

fn main() -> std::io::Result<()> {
  let port = std::env::var("PORT").unwrap_or(1234.to_string());
  println!("new connection at {}", port);
  let listener = TcpListener::bind(format!("0.0.0.0:{}", port))?;
  loop {
    let _ = handle_client(listener.accept()?.0);
  }
}
```

使用如下命令来编译 Rust 程序。

```bash
cargo build --target wasm32-wasi --release
```

使用如下命令在 WasmEdge 中运行程序。

```bash
$ wasmedge target/wasm32-wasi/release/http_server.wasm
new connection at 1234
```

你可以用 `curl` 发送一个 HTTP 请求，来测试 HTTP 服务器。

```bash
$ curl -d "name=WasmEdge" -X POST http://127.0.0.1:1234
echo: name=WasmEdge
```
