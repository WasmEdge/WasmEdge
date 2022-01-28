# Networking sockets

The [wasmedge_wasi_socket](https://github.com/second-state/wasmedge_wasi_socket) crate enables Rust developers to create networking applications and compile them into WebAssembly for WasmEdge Runtime. One of the key features of WasmEdge is that it supports non-blocking sockets. That allows even a single threaded WASM application to handle concurrent network requests. For example, while the program is waiting for the network response, it can start or handle another request. In this chapter, we will start with simple HTTP client and server examples. and then cover the more complex non-blocking examples.

## An HTTP client example

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

The following command compiles the Rust program.

```bash
$ cargo build --target wasm32-wasi --release
```

The following command runs the application in WasmEdge.

```bash
$ wasmedge target/wasm32-wasi/release/http_client.wasm
```

## An HTTP server example

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

The following command compiles the Rust program.

```bash
$ cargo build --target wasm32-wasi --release
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

## A non-blocking HTTP server example

The [source code](https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples/poll_http_server) for a non-blocking HTTP server application is available. The following `main()` function starts an HTTP server. It receives various events from the network socket, and processes those events asynchronously -- meaning that it can process events from multiple requests at the same time.

```rust
fn main() -> std::io::Result<()> {
    let mut poll = Poll::new();
    let server = TcpListener::bind("127.0.0.1:1234", true)?;
    println!("Listening on 127.0.0.1:1234");
    let mut connections = HashMap::new();
    let mut handlers = HashMap::new();
    const SERVER: Token = Token(0);
    let mut unique_token = Token(SERVER.0 + 1);

    poll.register(&server, SERVER, Interest::Read);

    loop {
        let events = poll.poll().unwrap();

        for event in events {
            match event.token {
                SERVER => loop {
                    let (connection, address) = match server.accept(FDFLAGS_NONBLOCK) {
                        Ok((connection, address)) => (connection, address),
                        Err(ref e) if e.kind() == std::io::ErrorKind::WouldBlock => break,
                        Err(e) => panic!("accept error: {}", e),
                    };

                    println!("Accepted connection from: {}", address);

                    let token = unique_token.add();
                    poll.register(&connection, token, Interest::Read);
                    connections.insert(token, connection);
                },
                token => {
                    let done = if let Some(connection) = connections.get_mut(&token) {
                        let handler = match handlers.get_mut(&token) {
                            Some(handler) => handler,
                            None => {
                                let handler = Handler::new();
                                handlers.insert(token, handler);
                                handlers.get_mut(&token).unwrap()
                            }
                        };
                        handle_connection(&mut poll, connection, handler, &event)?
                    } else {
                        false
                    };
                    if done {
                        if let Some(connection) = connections.remove(&token) {
                            connection.shutdown(Shutdown::Both)?;
                            poll.unregister(&connection);
                            handlers.remove(&token);
                        }
                    }
                }
            }
        }
    }
}
```

The `handle_connection()` function processes the data in each request. In this case, it just writes the request body into the response. It is also done asynchronously -- meaning that the `handle_connection()` function can write to the server socket or even make further network requests, and yield to other tasks while waiting for responses.

```rust
fn handle_connection(
    poll: &mut Poll,
    connection: &mut TcpStream,
    handler: &mut Handler,
    event: &Event,
) -> io::Result<bool> {
    if event.is_readable() {
        let mut connection_closed = false;
        let mut received_data = vec![0; 4096];
        let mut bytes_read = 0;
        loop {
            match connection.read(&mut received_data[bytes_read..]) {
                Ok(0) => {
                    connection_closed = true;
                    break;
                }
                Ok(n) => {
                    bytes_read += n;
                    if bytes_read == received_data.len() {
                        received_data.resize(received_data.len() + 1024, 0);
                    }
                }
                Err(ref err) if would_block(err) => {
                    if bytes_read != 0 {
                        let received_data = &received_data[..bytes_read];
                        let mut bs: parsed::stream::ByteStream =
                            match String::from_utf8(received_data.to_vec()) {
                                Ok(s) => s,
                                Err(_) => {
                                    continue;
                                }
                            }
                            .into();
                        let req = match parsed::http::parse_http_request(&mut bs) {
                            Some(req) => req,
                            None => {
                                break;
                            }
                        };
                        for header in req.headers.iter() {
                            if header.name.eq("Conntent-Length") {
                                let content_length = header.value.parse::<usize>().unwrap();
                                if content_length > received_data.len() {
                                    return Ok(true);
                                }
                            }
                        }
                        println!(
                            "{:?} request: {:?} {:?}",
                            connection.peer_addr().unwrap(),
                            req.method,
                            req.path
                        );
                        let res = Response {
                            protocol: "HTTP/1.1".to_string(),
                            code: 200,
                            message: "OK".to_string(),
                            headers: vec![
                                Header {
                                    name: "Content-Length".to_string(),
                                    value: req.content.len().to_string(),
                                },
                                Header {
                                    name: "Connection".to_string(),
                                    value: "close".to_string(),
                                },
                            ],
                            content: req.content,
                        };

                        handler.response = Some(res.into());

                        poll.reregister(connection, event.token, Interest::Write);
                        break;
                    } else {
                        println!("Empty request");
                        return Ok(true);
                    }
                }
                Err(ref err) if interrupted(err) => continue,
                Err(err) => return Err(err),
            }
        }

        if connection_closed {
            println!("Connection closed");
            return Ok(true);
        }
    }

    if event.is_writable() && handler.response.is_some() {
        let resp = handler.response.clone().unwrap();
        match connection.write(resp.as_bytes()) {
            Ok(n) if n < resp.len() => return Err(io::ErrorKind::WriteZero.into()),
            Ok(_) => {
                return Ok(true);
            }
            Err(ref err) if would_block(err) => {}
            Err(ref err) if interrupted(err) => {
                return handle_connection(poll, connection, handler, event)
            }
            Err(err) => return Err(err),
        }
    }

    Ok(false)
}
```

The following command compiles the Rust program.

```bash
$ cargo build --target wasm32-wasi --release
```

The following command runs the application in WasmEdge.

```bash
$ wasmedge target/wasm32-wasi/release/poll_http_server.wasm
new connection at 1234
```

To test the HTTP server, you can submit a HTTP request to it via `curl`.

```bash
$ curl -d "name=WasmEdge" -X POST http://127.0.0.1:1234
echo: name=WasmEdge
```
