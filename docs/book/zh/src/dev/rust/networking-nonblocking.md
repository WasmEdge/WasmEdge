# 非阻塞的网络通信

尽管上一章节中的 HTTP 连接实现起来很容易，但他们并不适合在生产环境使用。如果程序一次只允许一个连接开启（阻塞），CPU 将不得不等待缓慢的网络。非阻塞 I/O 意味着程序可以同时保持多个连接开启，并处理这些连接传输的数据。程序可以轮询这些开启的连接，或是等待输入的数据触发异步函数。这将让 I/O 密集的程序在单线程环境中运行得非常快。在这一章中，我们将介绍轮询和异步编程模型。

## 非阻塞 HTTP 客户端示例

非阻塞的 HTTP 客户端程序的[源代码在这里](https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples/nonblock_http_client)。下面的 `main()` 函数开启了两个 HTTP 连接。它同时保持两个连接开启，并轮流查看是否有数据传输进来。换句话说，这两个连接并不会相互阻塞。他们的数据在传输进来的时候被同时（或者轮流）处理。

```rust
use httparse::{Response, EMPTY_HEADER};
use std::io::{self, Read, Write};
use std::str::from_utf8;
use wasmedge_wasi_socket::TcpStream;

fn main() {
    let req = "GET / HTTP/1.0\n\n";
    let mut first_connection = TcpStream::connect("127.0.0.1:80").unwrap();
    first_connection.set_nonblocking(true).unwrap();
    first_connection.write_all(req.as_bytes()).unwrap();

    let mut second_connection = TcpStream::connect("127.0.0.1:80").unwrap();
    second_connection.set_nonblocking(true).unwrap();
    second_connection.write_all(req.as_bytes()).unwrap();

    let mut first_buf = vec![0; 4096];
    let mut first_bytes_read = 0;
    let mut second_buf = vec![0; 4096];
    let mut second_bytes_read = 0;

    loop {
        let mut first_complete = false;
        let mut second_complete = false;
        if !first_complete {
            match read_data(&mut first_connection, &mut first_buf, first_bytes_read) {
                Ok((bytes_read, false)) => {
                    first_bytes_read = bytes_read;
                }
                Ok((bytes_read, true)) => {
                    println!("First connection completed");
                    if bytes_read != 0 {
                        parse_data(&first_buf, bytes_read);
                    }
                    first_complete = true;
                }
                Err(e) => {
                    println!("First connection error: {}", e);
                    first_complete = true;
                }
            }
        }
        if !second_complete {
            match read_data(&mut second_connection, &mut second_buf, second_bytes_read) {
                Ok((bytes_read, false)) => {
                    second_bytes_read = bytes_read;
                }
                Ok((bytes_read, true)) => {
                    println!("Second connection completed");
                    if bytes_read != 0 {
                        parse_data(&second_buf, bytes_read);
                    }
                    second_complete = true;
                }
                Err(e) => {
                    println!("Second connection error: {}", e);
                    second_complete = true;
                }
            }
        }
        if first_complete && second_complete {
            break;
        }
    }
}
```

使用如下命令来编译 Rust 程序。

```bash
cargo build --target wasm32-wasi --release
```

使用如下命令在 WasmEdge 中运行程序。

```rust
wasmedge target/wasm32-wasi/release/nonblock_http_client.wasm
```

## 非阻塞 HTTP 服务器示例

非阻塞的 HTTP 服务器程序的 [源代码在这里](https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples/poll_http_server)。下面的 `main()` 函数开启了一个 HTTP 服务器。它同时从多个开启的连接中接收事件，并通过调用注册在每个连接的异步处理函数来处理这些事件。服务器可以同时从多个开启的连接中处理事件。

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

`handle_connection()` 函数处理来自于开启的连接的数据。当前它只是将请求的内容写回响应中。这同样是以异步的形式完成的，意味着 `handle_connection()` 函数为响应创建了一个事件，然后将事件放入了一个队列中。主程序循环处理这些事件，并在等待来自其他连接的数据时发送这些响应。

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

使用如下命令来编译 Rust 程序。

```bash
cargo build --target wasm32-wasi --release
```

使用如下命令在 WasmEdge 中运行程序。

```bash
$ wasmedge target/wasm32-wasi/release/poll_http_server.wasm
new connection at 1234
```

你可以用 `curl` 发送一个 HTTP 请求，来测试 HTTP 服务器。

```bash
$ curl -d "name=WasmEdge" -X POST http://127.0.0.1:1234
echo: name=WasmEdge
```
