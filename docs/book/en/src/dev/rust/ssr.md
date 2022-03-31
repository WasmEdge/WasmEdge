# Server-side rendering

Frontend web frameworks allow developers to create web apps in a high level language and component model. The web app is built into a static web site to be rendered in the browser. While many frontend web frameworks are based on JavaScript, such as React and Vue, Rust-based frameworks are also emerging as the Rust language gains traction among developers. Those web frameworks render the HTML DOM UI using the WebAssembly, which is compiled from Rust source code. They use [wasm-bindgen](https://github.com/rustwasm/wasm-bindgen) to tie the Rust to the HTML DOM.
While all of these frameworks send `.wasm` files to the browser to render the UI on the client-side, some provide the additional choice for [Server-side rendering](https://en.wikipedia.org/wiki/Server-side_scripting). That is to run the WebAssembly code and build the HTML DOM UI on the server, and stream the HTML content to the browser for faster performance and startup time on slow devices and networks.

> If you are interested in JavaScript-based Jamstack and SSR frameworks, such as React, please [checkout our JavaScript SSR chapter](../js/ssr.md).

This article will explore how to render the web UI on the server using WasmEdge.
We pick [Percy](https://github.com/chinedufn/percy) as our framework because it is relatively mature in SSR and [Hydration](https://en.wikipedia.org/wiki/Hydration_(web_development)). Percy already provides an [example](https://github.com/chinedufn/percy/tree/master/examples/isomorphic) for SSR. It's highly recommended to read it first to understand how it works. The default SSR setup with Percy utilizes a native Rust web server. The Rust code is compiled to machine native code for the server. However, in order to host user applications on the server, we need a sandbox. While we could run native code inside a Linux container (Docker), a far more efficient (and safer) approach is to run the compiled code in a WebAssembly VM on the server, especially considerring the rendering code is already compiled into WebAssembly.

Now, let's go through the steps to run a Percy SSR service in a WasmEdge server.

Assuming we are in the `examples/isomorphic` directory, make a new crate beside the existing `server`.

```bash
cargo new server-wasmedge
```

You'll receive a warning to let you put the new crate into the workspace, so insert below into `members` of `[workspace]`. The file is `../../Cargo.toml`.

```toml
"examples/isomorphic/server-wasmedge"
```

With the file open, put these two lines in the bottom:

```toml
[patch.crates-io]
wasm-bindgen = { git = "https://github.com/KernelErr/wasm-bindgen.git", branch = "wasi-compat" }
```

> Why do we need a forked `wasm-bindgen`? That is because `wasm-bindgen` is the required glue between Rust and HTML in the browser. On the server, however, we need to build the Rust code to the `wasm32-wasi` target, which is incompatible with `wasm-bindgen`. Our forked `wasm-bindgen` has conditional configs that removes browser-specific code in the generated `.wasm` file for the `wasm32-wasi` target.

Then replace the crate's `Cargo.toml` with following content.

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

The `wasmedge_wasi_socket` crate is the socket API of WasmEdge. This project is under development. Next copy the `index.html` file into the crate's root.

```bash
cp server/src/index.html server-wasmedge/src/
```

Then let's create some Rust code to start a web service in WasmEdge! The `main.rs` program listens to the request and sends the response via the stream.

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

The `handler.rs` parses the received data to the path and query objects and return the corresponding response.

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

The `response.rs` program packs the response object for static assets and for server rendered content.
For the latter, you could see that SSR happens at `app.render().to_string()`, the result string is put into HTML by replacing the placeholder text.

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

The `mime.rs` program is a map for assets' extension name and the Mime type.

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

That's it! Now let's build and run the web application. If you have tested the original example, you probably have already built the client WebAssembly.

```bash
cd client
./build-wasm.sh
```

Next, build and run the server.

```bash
cd ../server-wasmedge
cargo build --target wasm32-wasi
OUTPUT_CSS="$(pwd)/../client/build/app.css" wasmedge --dir /static:../client/build ../../../target/wasm32-wasi/debug/isomorphic-server-wasmedge.wasm
```

Navigate to `http://127.0.0.1:3000` and you will see the web application in action.

Furthermore, you can place all the steps into a shell script `../start-wasmedge.sh`.

```bash
#!/bin/bash

cd $(dirname $0)

cd ./client

./build-wasm.sh

cd ../server-wasmedge

OUTPUT_CSS="$(pwd)/../client/build/app.css" cargo run -p isomorphic-server-wasmedge
```

Add the following to the `.cargo/config.toml` file.

```toml
[build]
target = "wasm32-wasi"

[target.wasm32-wasi]
runner = "wasmedge --dir /static:../client/build" 
```

After that, a single CLI command `./start-wasmedge.sh` would perform all the tasks to build and run the web application!

We forked the Percy repository and made a ready-to-build [server-wasmedge](https://github.com/second-state/percy/tree/master/examples/isomorphic/server-wasmedge) example project for you. Happy coding!
