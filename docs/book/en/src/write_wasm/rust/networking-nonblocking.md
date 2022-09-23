# Non-blocking Networking Sockets

While the simple HTTP connections from the previous chapter are easy to implement, they are not ready for production use. If the program can only have one connection open at a time (e.g., blocking), the fast CPU would be waiting for the slow network. Non-blocking I/O means that the application program can keep multiple connections open at the same time, and process data in and out of those connections as they come in. The program can either alternatingly poll those open connections or wait for incoming data to trigger async functions. That allows I/O intensive programs to run much faster even in a single-threaded environment. In this chapter, we will cover both polling and async programming models.

## A non-blocking HTTP client example

The [source code](https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples/nonblock_http_client) for a non-blocking HTTP client application is available. The following `main()` function starts two HTTP connections. The program keeps both connections open, and alternatingly checks for incoming data from them. In another word, the two connections are not blocking each other. Their data are handled concurrently (or alternatingly) as the data comes in.

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

The following command compiles the Rust program.

```bash
cargo build --target wasm32-wasi --release
```

The following command runs the application in WasmEdge.

```rust
wasmedge target/wasm32-wasi/release/nonblock_http_client.wasm
```

## A non-blocking HTTP server example

The [source code](https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples/http_server) for a non-blocking HTTP server application is available. The following `main()` function starts an HTTP server. It receives events from multiple open connections, and processes those events as they are received by calling the async handler functions registered to each connection. This server can process events from multiple open connections concurrently.

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

The `handle_connection()` function processes the data from those open connections. In this case, it just writes the request body into the response. It is also done asynchronously -- meaning that the `handle_connection()` function creates an event for the response, and puts it in the queue. The main application loop processes the event and sends the response when it is waiting for data from other connections.

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
cargo build --target wasm32-wasi --release
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
