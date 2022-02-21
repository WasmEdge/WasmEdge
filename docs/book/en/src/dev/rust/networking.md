# Networking sockets

The [wasmedge_wasi_socket](https://github.com/second-state/wasmedge_wasi_socket) crate enables Rust developers to create networking applications and compile them into WebAssembly for WasmEdge Runtime.

## The HTTP client example

The [source code](https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples/http_client) for the HTTP client is available as follows.

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

To compile this source code, you must have the following in your `Cargo.toml`.

```toml
[dependencies]
wasmedge_http_req  = "0.8.1"
```

The following command compiles the Rust program.

```bash
cargo build --target wasm32-wasi --release
```

The following command runs the application in WasmEdge.

```bash
wasmedge target/wasm32-wasi/release/http_client.wasm
```

## The HTTP server example

The [source code](https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples/http_server) for the HTTP server application is available as follows.

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

To compile this source code, you must have the following in your `Cargo.toml`.

```toml
[dependencies]
wasmedge_http_req  = "0.8.1"
```

The following command compiles the Rust program.

```bash
cargo build --target wasm32-wasi --release
```

The following command runs the application in WasmEdge.

```bash
$ wasmedge target/wasm32-wasi/release/http_server.wasm
new connection at 1234
```

To test the HTTP server, you can submit a HTTP request to it via `curl`.

```bash
$ curl -d "name=WasmEdge" -X POST http://127.0.0.1:1234
echo: name=WasmEdge
```
