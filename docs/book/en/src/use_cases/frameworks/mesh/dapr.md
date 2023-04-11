# Dapr

In this article, I will demonstrate how to use WasmEdge as a sidecar application runtime for Dapr. There are two ways to do this:

* **Standalone WasmEdge** is the **recommended approach** is to write a microservice using [Rust](../../../write_wasm/rust/networking-nonblocking.md) or [JavaScript](../../../write_wasm/js/networking.md), and run it in WasmEdge. The WasmEdge application serves web requests and communicates with the sidecar via sockets using the Dapr API. In this case, we can [run WasmEdge as a managed container in k8s](../../kubernetes/quickstart.md).
* Alternatively, Embedded WasmEdge is to create a simple microservice in Rust or Go to listen for web requests and communicate with the Dapr sidecar. It passes the request data to a WasmEdge runtime for processing. The business logic of the microservice is a WebAssembly function created and deployed by an application developer.

> While the first approach (running the entire microservice in WasmEdge) is much preferred, we are still working on a fully fledged Dapr SDKs for WasmEdge. You can track their progress in GitHub issues -- [Rust](https://github.com/WasmEdge/WasmEdge/issues/1571) and [JavaScript](https://github.com/WasmEdge/WasmEdge/issues/1572).

## Quick start

First you need to install [Dapr](https://docs.dapr.io/getting-started/install-dapr-cli) and [WasmEdge](../../../quick_start/install.md). [Go](https://golang.org/doc/install) and [Rust](https://www.rust-lang.org/tools/install) are optional for the standalone WasmEdge approach. However, they are required for the demo app since it showcases both standalone and embedded WasmEdge approaches.

Fork or clone the demo application from Github. You can use this repo as your own application template.

```bash
git clone https://github.com/second-state/dapr-wasm
````

The demo has 4 Dapr sidecar applications. The [web-port](https://github.com/second-state/dapr-wasm/tree/main/web-port) project provides a public web service for a static HTML page. This is the application’s UI. From the static HTML page, the user can select a microservice to turn an input image into grayscale. All 3 microsoervices below perform the same function. They are just implemented using different approaches.

* **Standalone WasmEdge approach:** The [image-api-wasi-socket-rs](https://github.com/second-state/dapr-wasm/tree/main/image-api-wasi-socket-rs) project provides a standalone WasmEdge sidecar microservice that takes the input image and returns the grayscale image. The microservice is written in Rust and compiled into WebAssembly bytecode to run in WasmEdge.
* Embedded WasmEdge approach #1: The [image-api-rs](https://github.com/second-state/dapr-wasm/tree/main/image-api-rs) project provides a simple Rust-based microservice. It embeds a [WasmEdge function](https://github.com/second-state/dapr-wasm/tree/main/functions/grayscale) to turn an input image into a grayscale image.
* Embedded WasmEdge approach #2: The [image-api-go](https://github.com/second-state/dapr-wasm/tree/main/image-api-go) project provides a simple Go-based microservice. It embeds a [WasmEdge function](https://github.com/second-state/dapr-wasm/tree/main/functions/grayscale) to turn an input image into a grayscale image.

You can follow the instructions in the [README](https://github.com/second-state/dapr-wasm/blob/main/README.md) to start the sidecar services. Here are commands to build the WebAssembly functions and start the sidecar services. The first set of commands deploy the static web page service and the standalone WasmEdge service written in Rust. It forms a complete application to turn an input image into grayscale.

```bash
# Build and start the static HTML web page service for the UI and router for sending the uploaded image to the grayscale microservice
cd web-port
go build
./run_web.sh
cd ../

# Build the standalone image grayscale web service for WasmEdge
cd image-api-wasi-socket-rs
cargo build  --target wasm32-wasi
cd ../

# Run the microservice as a Dapr sidecar app
cd image-api-wasi-socket-rs
./run_api_wasi_socket_rs.sh
cd ../
```

The second set of commands create the alternative microservices for the embedded WasmEdge function.

```bash
# Build the grayscale WebAssembly functions, and deploy them to the sidecar projects
cd functions/grayscale
./build.sh
cd ../../

# Build and start the Rust-based microservice for embedding the grayscale WasmEdge function
cd image-api-rs
cargo build --release
./run_api_rs.sh
cd ../

# Build and start the Go-based microservice for embedding the grayscale WasmEdge function
cd image-api-go
go build
./run_api_go.sh
cd ../
```

Finally, you should be able to see the web UI in your browser.

## Recommended: The standalone WasmEdge microservice in Rust

The [standalone WasmEdge microservice](https://github.com/second-state/dapr-wasm/blob/main/image-api-wasi-socket-rs/src/main.rs) starts a non-blocking TCP server inside WasmEdge. The TCP server passes incoming requests to `handle_client()`, which passes HTTP requests to `handle_http()`, which calls `grayscale()` to process the image data in the request.

```rust
fn main() -> std::io::Result<()> {
    let port = std::env::var("PORT").unwrap_or(9005.to_string());
    println!("new connection at {}", port);
    let listener = TcpListener::bind(format!("127.0.0.1:{}", port))?;
    loop {
        let _ = handle_client(listener.accept()?.0);
    }
}

fn handle_client(mut stream: TcpStream) -> std::io::Result<()> {
  ... ...
}

fn handle_http(req: Request<Vec<u8>>) -> bytecodec::Result<Response<String>> {
  ... ...
}

fn grayscale(image: &[u8]) -> Vec<u8> {
    let detected = image::guess_format(&image);
    let mut buf = vec![];
    if detected.is_err() {
        return buf;
    }
    
    let image_format_detected = detected.unwrap();
    let img = image::load_from_memory(&image).unwrap();
    let filtered = img.grayscale();
    match image_format_detected {
        ImageFormat::Gif => {
            filtered.write_to(&mut buf, ImageOutputFormat::Gif).unwrap();
        }
        _ => {
            filtered.write_to(&mut buf, ImageOutputFormat::Png).unwrap();
        }
    };
    return buf;
}
```

> Work in progress: It will soon interact with the Dapr sidecar through the [WasmEdge Dapr SDK in Rust](https://github.com/WasmEdge/WasmEdge/issues/1571).

Now, you can build the microservice. It is a simple matter of compiling from Rust to WebAssembly.

```bash
cd image-api-wasi-socket-rs
cargo build  --target wasm32-wasi
```

Deploy the WasmEdge microservice in Dapr as follows.

```bash
dapr run --app-id image-api-wasi-socket-rs \
         --app-protocol http \
         --app-port 9005 \
         --dapr-http-port 3503 \
         --components-path ../config \
         --log-level debug \
         wasmedge ./target/wasm32-wasi/debug/image-api-wasi-socket-rs.wasm
```

## Alternative: The embedded WasmEdge microservices

The embedded WasmEdge approach requires us to create a WebAssembly function for the business logic (image processing) first, and then embed it into simple Dapr microservices.

### Rust function for image processing

The [Rust function](https://github.com/second-state/dapr-wasm/blob/main/functions/grayscale/src/lib.rs) is simple. It uses the [wasmedge_bindgen](../../../write_wasm/rust/bindgen.md) macro to makes it easy to call the function from a Go or Rust host embedding the WebAssembly function. It takes and returns base64 encoded image data for the web.

```rust
#[wasmedge_bindgen]
pub fn grayscale(image_data: String) -> String {
    let image_bytes = image_data.split(",").map(|x| x.parse::<u8>().unwrap()).collect::<Vec<u8>>();
    return grayscale::grayscale_internal(&image_bytes);
}
```

The Rust function that actually performs the task is as follows.

```rust
pub fn grayscale_internal(image_data: &[u8]) -> String {
    let image_format_detected: ImageFormat = image::guess_format(&image_data).unwrap();
    let img = image::load_from_memory(&image_data).unwrap();
    let filtered = img.grayscale();
    let mut buf = vec![];
    match image_format_detected {
        ImageFormat::Gif => {
            filtered.write_to(&mut buf, ImageOutputFormat::Gif).unwrap();
        }
        _ => {
            filtered.write_to(&mut buf, ImageOutputFormat::Png).unwrap();
        }
    };
    let mut base64_encoded = String::new();
    base64::encode_config_buf(&buf, base64::STANDARD, &mut base64_encoded);
    return base64_encoded.to_string();
}
```

### The Go host wrapper for microservice

The [Go-based microservice](https://github.com/second-state/dapr-wasm/tree/main/image-api-go) embeds the above imaging processing function in WasmEdge. The [microservice itself](https://github.com/second-state/dapr-wasm/blob/main/image-api-go/image_api.go) is a web server and utilizes the Dapr Go SDK.

```go
func main() {
  s := daprd.NewService(":9003")

  if err := s.AddServiceInvocationHandler("/api/image", imageHandlerWASI); err != nil {
    log.Fatalf("error adding invocation handler: %v", err)
  }

  if err := s.Start(); err != nil && err != http.ErrServerClosed {
    log.Fatalf("error listening: %v", err)
  }
}
```

The `imageHandlerWASI()` function [starts a WasmEdge instance](../../../sdk/go/function.md) and calls the image processing (grayscale) function in it via [wasmedge_bindgen](../../../write_wasm/rust/bindgen.md).

Build and deploy the Go microservice to Dapr as follows.

```bash
cd image-api-go
go build
dapr run --app-id image-api-go \
         --app-protocol http \
         --app-port 9003 \
         --dapr-http-port 3501 \
         --log-level debug \
         --components-path ../config \
         ./image-api-go
```

### The Rust host wrapper for microservice

The [Rust-based microservice](https://github.com/second-state/dapr-wasm/tree/main/image-api-rs) embeds the above imaging processing function in WasmEdge. The [microservice itself](https://github.com/second-state/dapr-wasm/blob/main/image-api-rs/src/main.rs) is a Tokio and Warp based web server.

```rust
#[tokio::main]
pub async fn run_server(port: u16) {
    pretty_env_logger::init();
    let home = warp::get().map(warp::reply);

    let image = warp::post()
        .and(warp::path("api"))
        .and(warp::path("image"))
        .and(warp::body::bytes())
        .map(|bytes: bytes::Bytes| {
            let v: Vec<u8> = bytes.iter().map(|&x| x).collect();
            let res = image_process_wasmedge_sys(&v);
            let _encoded = base64::encode(&res);
            Response::builder()
                .header("content-type", "image/png")
                .body(res)
        });

    let routes = home.or(image);
    let routes = routes.with(warp::cors().allow_any_origin());

    let log = warp::log("dapr_wasm");
    let routes = routes.with(log);
    warp::serve(routes).run((Ipv4Addr::UNSPECIFIED, port)).await
}
```

The `image_process_wasmedge_sys()` function [starts a WasmEdge instance](../../../sdk/rust/sys_run_host_func.md) and calls the image processing (grayscale) function in it via [wasmedge_bindgen](../../../write_wasm/rust/bindgen.md).

Build and deploy the Rust microservice to Dapr as follows.

```bash
cd image-api-rs
cargo build --release
dapr stop image-api-rs

# Change this to your own path for WasmEdge
export LD_LIBRARY_PATH=/home/coder/.wasmedge/lib64/

dapr run --app-id image-api-rs \
         --app-protocol http \
         --app-port 9004 \
         --dapr-http-port 3502 \
         --components-path ../config \
         --log-level debug \
         ./target/release/image-api-rs
```

That's it! [Let us know](https://github.com/WasmEdge/WasmEdge/discussions) your cool Dapr microservices in WebAssembly!
