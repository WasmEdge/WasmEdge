# Server-side rendering

As Rust becomes more popular, many things are being reinvented using Rust. Frontend frameworks are certainly among them.
There are already many frameworks you can choose to build your browser based application.
They all have the same underlying logic: rendering using the WebAssembly, which is built from Rust.
And also, they all use [wasm-bindgen](https://github.com/rustwasm/wasm-bindgen) to tie the Rust to the DOM, which is the only option for now.
While all of these frameworks send .wasm files to the browser and render the page on the client-side, some supply the choice for [Server-side rendering](https://en.wikipedia.org/wiki/Server-side_scripting).

This article will explore how to render the page on the server that runs on the WasmEdge.
We pick [Percy](https://github.com/chinedufn/percy) as our framework because it is relatively mature in SSR and [Hydration](https://en.wikipedia.org/wiki/Hydration_(web_development)).

Percy already provides an [example](https://github.com/chinedufn/percy/tree/master/examples/isomorphic) for SSR. It's highly recommended to read it first to understand how it works.

Then let's dive into changing the server to WasmEdge.

Assuming we are in the `examples/isomorphic` directory, make a new bin crate beside the existing `server`:
```bash
cargo new server-wasmedge
```
You'll receive a warning to let you put the new crate into the workspace, so insert below into `members` of `[workspace]`:
```toml
"examples/isomorphic/server-wasmedge"
```
File: `../../Cargo.toml`

And with the file open, put these two lines in the bottom:
```toml
[patch.crates-io]
wasm-bindgen = { git = "https://github.com/KernelErr/wasm-bindgen.git", branch = "wasi-compat" }
```
File: `../../Cargo.toml`

What does this work for? As `wasm-bindgen` is the glue between Rust and Html, it is designed to work with both frontend and backend, means it should support both  architechture of `wasm32`, that is to say `wasm32-unknown-unknown` and `wasm32-wasi`.
Unfortunately, there is still some problem of the support to `wasm32-wasi`.

In the source code of `wasm-bindgen`, there are some conditinal configure like this:
```rust
 #[cfg(all(target_arch = "wasm32", not(target_os = "emscripten")))]
```
When the built target is native code such as `x86_64`, the code under this configure will generate different path so the `server` in percy's original example will work.
But if the built target is `wasm32-wasi`, the code will generate the same path with what can only work in the browser. And what we want to do is to build the `server-wasmedge` to WebAssembly and run it on WasmEdge.
So we have forked the `wasm-bindgen` repository and made some tiny changes to the conditional configure. The result is the generated path is the same as native code and will not include the logic that only runs in the browser.


Then replace the crate's `Cargo.toml` with following content:
```toml
[package]
name = "isomorphic-server-wasmedge"
version = "0.1.0"
edition = "2021"

# See more keys and their definitions at https://doc.rust-lang.org/cargo/reference/manifest.html

[dependencies]
wasmedge_wasi_socket = "0"
querystring = "1.1.0"
parsed = { version = "0.3", features = ["http"] }
anyhow = "1"
serde = { version = "1.0", features = ["derive"] }
isomorphic-app = { path = "../app" } 
```
The `wasmedge_wasi_socket` crate is the socket API of WasmEdge. This project is under intensive development and the API is not stable.


Next copy the `index.html` into the crate's root:
```bash
cp server/src/index.html server-wasmedge/src/
```

Then let's type some Rust code!

```rust
use std::io::Write;
use wasmedge_wasi_socket::{Shutdown, TcpListener};

mod handler;
mod mime;
mod response;

fn main() {
    let server = TcpListener::bind("127.0.0.1:3000", false).unwrap();
    println!("Server listening on 127.0.0.1:3000");

    // Simple single thread HTTP server
    // For server with Pool support, see https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples/poll_http_server
    loop {
        let (mut stream, addr) = server.accept(0).unwrap();
        println!("Accepted connection from {}", addr);
        match handler::handle_req(&mut stream, addr) {
            Ok((res, binary)) => {
                let res: String = res.into();
                let bytes = res.as_bytes();
                stream.write_all(bytes).unwrap();
                if let Some(binary) = binary {
                    stream.write_all(&binary).unwrap();
                }
            }
            Err(e) => {
                println!("Error: {:?}", e);
            }
        };
        stream.shutdown(Shutdown::Both).unwrap();
    }
}
```
File: `main.rs`

`main.rs` listens to the request and sends the response via the stream.


```rust
use crate::response;
use anyhow::Result;
use parsed::http::Response;
use std::io::Read;
use wasmedge_wasi_socket::{SocketAddr, TcpStream};

pub fn handle_req(stream: &mut TcpStream, addr: SocketAddr) -> Result<(Response, Option<Vec<u8>>)> {
    let mut buf = [0u8; 1024];
    let mut received_data: Vec<u8> = Vec::new();

    loop {
        let n = stream.read(&mut buf)?;
        received_data.extend_from_slice(&buf[..n]);
        if n < 1024 {
            break;
        }
    }

    let mut bs: parsed::stream::ByteStream = match String::from_utf8(received_data) {
        Ok(s) => s.into(),
        Err(_) => return Ok((response::bad_request(), None)),
    };

    let req = match parsed::http::parse_http_request(&mut bs) {
        Some(req) => req,
        None => return Ok((response::bad_request(), None)),
    };

    println!("{:?} request: {:?} {:?}", addr, req.method, req.path);

    let mut path_split = req.path.split("?");
    let path = path_split.next().unwrap_or("/");
    let query_str = path_split.next().unwrap_or("");
    let query = querystring::querify(&query_str);
    let mut init_count: Option<u32> = None;
    for (k, v) in query {
        if k.eq("init") {
            match v.parse::<u32>() {
                Ok(v) => init_count = Some(v),
                Err(_) => return Ok((response::bad_request(), None)),
            }
        }
    }

    let (res, binary) = if path.starts_with("/static") {
        response::file(&path)
    } else {
        // render page
        response::ssr(&path, init_count)
    }
    .unwrap_or_else(|_| response::internal_error());

    Ok((res, binary))
}
```
File: `handler.rs`

`handler.rs` parses the received data to the path and query objects and return the corresponding response.


```rust
use crate::mime::MimeType;
use anyhow::Result;
use parsed::http::{Header, Response};
use std::fs::{read};
use std::path::Path;
use isomorphic_app::App;

const HTML_PLACEHOLDER: &str = "#HTML_INSERTED_HERE_BY_SERVER#";
const STATE_PLACEHOLDER: &str = "#INITIAL_STATE_JSON#";

pub fn ssr(path: &str, init: Option<u32>) -> Result<(Response, Option<Vec<u8>>)> {
    let html = format!("{}", include_str!("./index.html"));

    let app = App::new(init.unwrap_or(1001), path.to_string());
    let state = app.store.borrow();

    let html = html.replace(HTML_PLACEHOLDER, &app.render().to_string());
    let html = html.replace(STATE_PLACEHOLDER, &state.to_json());

    Ok((Response {
        protocol: "HTTP/1.0".to_string(),
        code: 200,
        message: "OK".to_string(),
        headers: vec![
            Header {
                name: "content-type".to_string(),
                value: MimeType::from_ext("html").get(),
            },
            Header {
                name: "content-length".to_string(),
                value: html.len().to_string(),
            },
        ],
        content: html.into_bytes(),
    }, None))
}

/// Get raw file content
pub fn file(path: &str) -> Result<(Response, Option<Vec<u8>>)> {
    let path = Path::new(&path);
    if path.exists() {
        let content_type: MimeType = match path.extension() {
            Some(ext) => MimeType::from_ext(ext.to_str().get_or_insert("")),
            None => MimeType::from_ext(""),
        };
        let content = read(path)?;

        Ok((Response {
            protocol: "HTTP/1.0".to_string(),
            code: 200,
            message: "OK".to_string(),
            headers: vec![
                Header {
                    name: "content-type".to_string(),
                    value: content_type.get(),
                },
                Header {
                    name: "content-length".to_string(),
                    value: content.len().to_string(),
                },
            ],
            content: vec![],
        }, Some(content)))
    } else {
        Ok((Response {
            protocol: "HTTP/1.0".to_string(),
            code: 404,
            message: "Not Found".to_string(),
            headers: vec![],
            content: vec![],
        }, None))
    }
}

/// Bad Request
pub fn bad_request() -> Response {
    Response {
        protocol: "HTTP/1.0".to_string(),
        code: 400,
        message: "Bad Request".to_string(),
        headers: vec![],
        content: vec![],
    }
}

/// Internal Server Error
pub fn internal_error() -> (Response, Option<Vec<u8>>) {
    (Response {
        protocol: "HTTP/1.0".to_owned(),
        code: 500,
        message: "Internal Server Error".to_owned(),
        headers: vec![],
        content: vec![],
    }, None)
}
```
File: `response.rs`

`response.rs` packs the response object for static assets and for server rendered content.
For the latter, you could see that SSR happens at `app.render().to_string()`, the result string is put into Html by replacing the placeholder text.


```rust
pub struct MimeType {
    pub r#type: String,
}

impl MimeType {
    pub fn new(r#type: &str) -> Self {
        MimeType {
            r#type: r#type.to_string(),
        }
    }

    pub fn from_ext(ext: &str) -> Self {
        match ext {
            "html" => MimeType::new("text/html"),
            "css" => MimeType::new("text/css"),
            "map" => MimeType::new("application/json"),
            "js" => MimeType::new("application/javascript"),
            "json" => MimeType::new("application/json"),
            "svg" => MimeType::new("image/svg+xml"),
            "wasm" => MimeType::new("application/wasm"),
            _ => MimeType::new("text/plain"),
        }
    }

    pub fn get(self) -> String {
        self.r#type
    }
}
```
File: `mime.rs`

`mime.rs` is a map for assets' extension name and the Mime type.


That's it! Now let's make a run. If you have tested the original example, you should have built the client WebAssembly:
```bash
cd client
./build-wasm.sh
```

Then build and run the server:
```bash
cd ../server-wasmedge
cargo build --target wasm32-wasi
OUTPUT_CSS="$(pwd)/../client/build/app.css" wasmedge --dir /static:../client/build ../../../target/wasm32-wasi/debug/isomorphic-server-wasmedge.wasm
```
Now navigate to `http://127.0.0.1:3000` you will see the application.


For convenience, you can collect the steps into the following shell script and set up the Cargo config:
```bash
#!/bin/bash

cd $(dirname $0)

cd ./client

./build-wasm.sh

cd ../server-wasmedge

OUTPUT_CSS="$(pwd)/../client/build/app.css" cargo run -p isomorphic-server-wasmedge
```
File: `../start-wasmedge.sh`

```toml
[build]
target = "wasm32-wasi"

[target.wasm32-wasi]
runner = "wasmedge --dir /static:../client/build" 
```
File: `.cargo/config.toml`

After that, one command `./start-wasmedge.sh` will do all for you!


We have forked the percy repository and made a ready-to-build [server-wasmedge](https://github.com/second-state/percy/tree/master/examples/isomorphic/server-wasmedge) for you. Have a enjoy!