# 服务器端渲染

前端框架让开发者可以使用高级语言和组件模型来创建 Web 应用程序。Web 应用程序需要被构建成静态网页，才能在浏览器中渲染。尽管很多前端框架是基于 JavaScript 的，比如 React 和 Vue，但是随着 Rust 吸引了更多的开发者，基于 Rust 的框架也在不断涌现。这些前端框架使用由 Rust 编译而成的 WebAssembly 来渲染 HTML DOM UI。他们使用 [wasm-bindgen](https://github.com/rustwasm/wasm-bindgen) 来绑定 Rust 与 HTML DOM。这些框架都把 `.wasm` 文件发送到浏览器，在客户端渲染 UI，但其中的一些框架提供了对[服务端渲染](https://en.wikipedia.org/wiki/Server-side_scripting)的支持。这意味着我们在服务器上运行 WebAssembly 代码，并构建 HTML DOM UI，然后将 HTML 内容发送到浏览器，以此在较慢的设备和网络环境下获得更好的性能以及更快的启动速度。

> 如果你对 JavaScript 技术栈以及服务端渲染框架感兴趣，比如 React，请查看我们关于 [JavaScript 服务端渲染的章节](../js/ssr.md)。

本文将探索如何在服务器上使用 WasmEdge 来渲染 Web UI。
我们选择使用 [Percy](https://github.com/chinedufn/percy)，因为它在服务端渲染以及[混合开发](https://en.wikipedia.org/wiki/Hydration_(web_development))领域较为成熟。Percy 同样提供了一个服务端渲染的[示例](https://github.com/chinedufn/percy/tree/master/examples/isomorphic)。我们强烈建议你先去阅读这个示例，弄清楚它是如何工作的。Percy 默认的服务端渲染设置使用了一个原生的 Rust Web 服务器。对于服务器来说，Rust 代码被编译为原生机器码。然后，为了在服务器上运行用户的应用程序，我们需要一个沙箱。尽管我们可以在一个 Linux 容器（Docker）中运行原生代码，一个更高效且更快的方法是使用服务器上的 WebAssembly 虚拟机来运行编译好的代码，尤其是考虑到我们渲染的代码已经被编译成了 WebAssembly。

现在，让我们看一下在一个 WasmEdge 服务器上运行一个 Percy 服务端渲染的服务的步骤。

假设我们在 `examples/isomorphic` 文件夹中，创建一个新的包，和已有的 `server` 在同一个文件夹中。

```bash
cargo new server-wasmedge
```

当你把新的包加入到工作区时，你会收到一个警告，因此需要在 `[workspace]` 的 `members` 中插入下面这行。文件位于 `../../Cargo.toml`。

```toml
"examples/isomorphic/server-wasmedge"
```

趁文件还开着，将这两行放在文件底部：

```toml
[patch.crates-io]
wasm-bindgen = { git = "https://github.com/KernelErr/wasm-bindgen.git", branch = "wasi-compat" }
```

> 为什么我们需要一个 fork 的 `wasm-bindgen`？这是因为在浏览器中， `wasm-bindgen` 是将 Rust 和 HTML 连接起来必须的胶水。然而在服务器上，我们需要将 Rust 代码编译为针对 `wasm32-wasi` 目标的代码，这与 `wasm-bindgen` 是不兼容的。我们 fork 版本的 `wasm-bindgen` 有一些条件配置，可以为 `wasm32-wasi` 目标移除其生成的 .wasm 文件中的浏览器特定代码。

然后使用如下内容覆盖我们刚创建包的 `Cargo.toml`。

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

`wasmedge_wasi_socket` 包是 WasmEdge 的 Socket 接口。这个工程还在开发中。下一步将 `index.html` 文件复制到包的根目录。

```bash
cp server/src/index.html server-wasmedge/src/
```

让我们用 Rust 代码在 WasmEdge 中创建一个 Web 服务！ `main.rs` 程序监听到来的请求，并通过流发送响应。

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

`handler.rs` 中的代码解析收到的数据，并返回对应的响应。

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

`response.rs` 中的代码将静态资源和服务器渲染的内容打包成响应对象。
对后者来说，你可以看到服务端渲染发生于 `app.render().to_string()`，产生的字符串替换掉了 HTML 中的占位符。

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

`mime.rs` 中的代码将资源文件的拓展名映射成 MIME 类型。

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

就这么多！ 现在让我们来构建并运行 Web 应用程序。如果你对原来的示例进行了测试，那你可能已经编译好了客户端的 WebAssembly。

```bash
cd client
./build-wasm.sh
```

接下来，构建并运行服务器。

```bash
cd ../server-wasmedge
cargo build --target wasm32-wasi
OUTPUT_CSS="$(pwd)/../client/build/app.css" wasmedge --dir /static:../client/build ../../../target/wasm32-wasi/debug/isomorphic-server-wasmedge.wasm
```

访问 `http://127.0.0.1:3000` 你就会发现 Web 应用程序在正常工作。

并且，你可以将所有这些步骤放进一个脚本 `../start-wasmedge.sh` 里。

```bash
#!/bin/bash

cd $(dirname $0)

cd ./client

./build-wasm.sh

cd ../server-wasmedge

OUTPUT_CSS="$(pwd)/../client/build/app.css" cargo run -p isomorphic-server-wasmedge
```

然后将下面这些内容放入 `.cargo/config.toml` 中。

```toml
[build]
target = "wasm32-wasi"

[target.wasm32-wasi]
runner = "wasmedge --dir /static:../client/build" 
```

在这之后，只需要运行一个命令 `./start-wasmedge.sh` 就可以执行所有任务，构建并运行我们的 Web 应用程序！

我们也 fork 了 Percy 仓库，为你创建了一个可以直接构建的[示例](https://github.com/second-state/percy/tree/master/examples/isomorphic/server-wasmedge)。尽情享受编程的乐趣吧！
