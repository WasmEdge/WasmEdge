# Dapr

在这篇文章中，我将演示如何使用 WasmEdge 作为 Dapr sidecar 应用程序的 runtime。我们将使用一个用 Rust 或 Go 编写的简单的 NaCl 用于监听微服务的 API 请求。它会将请求的数据传递给 WebAssembly runtime 进行处理。而微服务的业务逻辑由应用开发者创建和部署的 WebAssembly 函数实现，你也可以观看相关的 [演示视频](https://www.youtube.com/watch?v=t_sQP6Qpf7U) 。

> 更多有关于在 Dapr 上运行 WasmEdge 的内容，请参阅文章 [A Lightweight, Safe, Portable, and High-performance Runtime for Dapr](https://www.secondstate.io/articles/dapr-wasmedge-webassembly/)

## 快速开始

首先，你需要安装 [Go](https://golang.org/doc/install)、[Rust](https://www.rust-lang.org/tools/install)、 [Dapr](https://docs.dapr.io/getting-started/install-dapr-cli)、 [WasmEdge](../../start/install.md)、 和 [rustwasmc](../../dev/rust/bindgen.md) 编译器工具。

接着，从 Github fork 或 clone 应用程序的 demo。你也可以将这个 repo 作为你自己的应用程序模板。

```bash
git clone https://github.com/second-state/dapr-wasm
```

这个 demo 包含 3 个 Dapr sidecar 应用程序，如下：

- [web-port](https://github.com/second-state/dapr-wasm/tree/main/web-port) 项目提供公开的静态 HTML 页面的 Web 服务。web-port 是 demo 的 UI 应用程序。
- [image-api-rs](https://github.com/second-state/dapr-wasm/tree/main/image-api-rs) 项目提供了一个运行 [grayscale](https://github.com/second-state/dapr-wasm/tree/main/functions/grayscale&sa=D&source=editors&ust=1634144162467000&usg=AOvVaw1uNZEDmOfXXXzLHHZWVFyD) 函数的 WasmEdge 微服务。微服务调用 [grayscale](https://github.com/second-state/dapr-wasm/tree/main/functions/grayscale&sa=D&source=editors&ust=1634144162467000&usg=AOvVaw1uNZEDmOfXXXzLHHZWVFyD) 将输入图像转换为 grayscale 图像。image-api-rs 演示了 Rust SDK 在 Dapr 和 WasmEdge 上的使用。
- [image-api-go](https://github.com/second-state/dapr-wasm/tree/main/image-api-go&sa=D&source=editors&ust=1634144162467000&usg=AOvVaw3pG0m0DQap9XTUAfBMyi1s) 项目提供了一个运行 [classify](https://github.com/second-state/dapr-wasm/tree/main/functions/classify&sa=D&source=editors&ust=1634144162468000&usg=AOvVaw0zYdNzIz6MiDkZCAvm_D9q) 函数的 WasmEdge 微服务，对输入图像中的对象进行识别和分类。image-api-go 演示了 Go SDK 在 Dapr 和 WasmEdge 上的使用。

![dapr-wasmedge](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/docs/book/en/src/frameworks/mesh/dapr-wasmedge.png)

如上，Dapr sidecar 微服务在这个 demo 应用程序架构中的位置。

你可以按照 [README](https://github.com/second-state/dapr-wasm/blob/main/README.md) 的说明启动 sidecar 服务。以下是构建 WebAssembly 函数和启动上述 3 个 sidecar 服务的命令。

```bash
# 构建分类和 grayscale WebAssembly 函数，并将它们部署到 Sidecar 项目
cd functions/grayscale
./build.sh
cd ../../
cd functions/classify
./build.sh
cd ../../

# 构建并启动 Web 服务用于应用程序的 UI 
cd web-port
go build
./run_web.sh
cd ../

# 构建并启动用于图像处理（grayscale）的微服务
cd image-api-rs
cargo build
./run_api_rs.sh
cd ../

# 构建并启动基于 tensorflow 的图像分类的微服务
cd image-api-go
go build --tags "tensorflow image"
./run_api_go.sh
cd ../
```

最后，你应该能够通过浏览器看见 Web UI 如下:

![dapr-wasmedge](https://github.com/WasmEdge/WasmEdge/blob/master/docs/book/en/src/frameworks/mesh/dapr-wasmedge-in-action.png)

如上，demo 应用程序正常运行的浏览器截图。

## 两个 WebAssembly 函数

我们有两个用 Rust 编写并被编译的 WebAssembly 函数。它们将部署在 sidecar 微服务中，分别执行图像处理和图像分类的任务。

虽然，我们的示例 WebAssembly 函数是用 Rust 编写的，但你也可以用 C/C++、Swift、Kotlin 和 AssemblyScript 编写并编译成 WebAssembly 函数。此外 WasmEdge 还支持运行 JavaScript 和 DSL 编写的函数。

[grayscale](https://github.com/second-state/dapr-wasm/tree/main/functions/grayscale) 函数是一个 Rust 编写的程序，它通过 `STDIN` 读取图像数据，并将 grayscale 图像写入 `STDOUT`。

```rust
use image::{ImageFormat, ImageOutputFormat};
use std::io::{self, Read, Write};
fn main() {
  let mut buf = Vec::new();
  io::stdin().read_to_end(&mut buf).unwrap();
  let image_format_detected: ImageFormat = image::guess_format(&buf).unwrap();
  let img = image::load_from_memory(&buf).unwrap();
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
  io::stdout().write_all(&buf).unwrap();
  io::stdout().flush().unwrap();
}
```

然后，我们使用 [rustwasmc](https://www.secondstate.io/articles/rustwasmc/) 进行构建，将构建好的 wasm 文件复制到 [image-api-rs](https://github.com/second-state/dapr-wasm/tree/main/image-api-rs) sidecar 中。

```bash
cd functions/grayscale
rustup override set 1.50.0
rustwasmc  build --enable-ext
cp ./pkg/grayscale.wasm ../../image-api-rs/lib
```

[classify](https://github.com/second-state/dapr-wasm/tree/main/functions/classify) 函数是一个 Rust 函数，它将图像数据的字节数组作为输入（参数），调用 WasmEdge TensorFlow API 处理，并返回一个字符串用于分类。

```rust
use wasmedge_tensorflow_interface;
pub fn infer_internal(image_data: &[u8]) -> String {
  let model_data: &[u8] = include_bytes!("models/mobilenet_v1_1.0_224/mobilenet_v1_1.0_224_quant.tflite");
  let labels = include_str!("models/mobilenet_v1_1.0_224/labels_mobilenet_quant_v1_224.txt");
  let flat_img = wasmedge_tensorflow_interface::load_jpg_image_to_rgb8(image_data, 224, 224);
  let mut session = wasmedge_tensorflow_interface::Session::new(
    &model_data,
    wasmedge_tensorflow_interface::ModelType::TensorFlowLite,
  );
  session.add_input("input", &flat_img, &[1, 224, 224, 3])
         .run();
  let res_vec: Vec<u8> = session.get_output("MobilenetV1/Predictions/Reshape_1");
  // ... Map the probabilities in res_vec to text labels in the labels file ...
  if max_value > 50 {
    format!(
      "It {} a <a href='https://www.google.com/search?q={}'>{}</a> in the picture",
      confidence.to_string(),
      class_name,
      class_name
    )
  } else {
    format!("It does not appears to be any food item in the picture.")
  }
}
```

然后，我们使用 [rustwasmc](https://www.secondstate.io/articles/rustwasmc/) 构建，将构建好的 wasm 文件复制到 [image-api-go](https://github.com/second-state/dapr-wasm/tree/main/image-api-go) sidecar。

```bash
cd functions/classify
rustup override set 1.50.0
rustwasmc  build --enable-ext
cp ./pkg/classify_bg.wasm ../../image-api-go/lib/classify_bg.wasm
```

在接下来的三个小节中，我们将探究这三个 sidecar 服务。

## 图像处理 sidecar

[image-api-rs](https://github.com/second-state/dapr-wasm/tree/main/image-api-rs) sidecar 应用程序是用 Rust 编写的。在上一步中，已经通过安装 `lib/grayscale.wasm` 拥有了 WebAssembly 函数。请参考 [functions/bin/install.sh](https://github.com/second-state/dapr-wasm/blob/main/functions/bin/install.sh) 脚本来安装 WasmEdge Runtime 的二进制文件 `lib/wasmedge-tensorflow-lite` 及其依赖项。

Sidecar 微服务运行了一个基于 Tokio 的事件循环，用于侦听 `/api/image` 路径上接收到的 HTTP 请求。

```rust
#[tokio::main]
pub async fn run_server(port: u16) {
  pretty_env_logger::init();
  let home = warp::get().map(warp::reply);
  let image = warp::post().and(warp::path("api"))
                          .and(warp::path("image"))
                          .and(warp::body::bytes())
                          .map(|bytes: bytes::Bytes| {
    let v: Vec<u8> = bytes.iter().map(|&x| x).collect();
    let res = image_process(&v);
    Ok(Box::new(res))
  });
  let routes = home.or(image);
  let routes = routes.with(warp::cors().allow_any_origin());
  let log = warp::log("dapr_wasm");
  let routes = routes.with(log);
  warp::serve(routes).run((Ipv4Addr::UNSPECIFIED, port)).await
}
```

一旦它接收到通过 HTTP POST 请求发送的图像文件，就会调用 WasmEdge 中的 WebAssembly 函数来执行图像处理任务。它创建一个 WasmEdge 实例来与 WebAssembly 程序交互。

```rust
pub fn image_process(buf: &Vec<u8>) -> Vec<u8> {
  let mut child = Command::new("./lib/wasmedge-tensorflow-lite")
    .arg("./lib/grayscale.wasm")
    .stdin(Stdio::piped())
    .stdout(Stdio::piped())
    .spawn()
    .expect("failed to execute child");
  {
    // limited borrow of stdin
    let stdin = child.stdin.as_mut().expect("failed to get stdin");
    stdin.write_all(buf).expect("failed to write to stdin");
  }
  let output = child.wait_with_output().expect("failed to wait on child");
  output.stdout
}
```

执行以下 Dapr CLI 命令将在 Dapr runtime 环境中启动 image-api-rs 微服务。

```bash
cd image-api-rs
sudo dapr run --app-id image-api-rs \
    --app-protocol http \
    --app-port 9004 \
    --dapr-http-port 3502 \
    --components-path ../config \
    --log-level debug \
    ./target/debug/image-api-rs
cd ../
```

## Tensorflow sidecar

[image-api-go](https://github.com/second-state/dapr-wasm/tree/main/image-api-go) sidecar 应用程序是用 Go 编写的。通过之前安装 `lib/classify\_bg.wasm` 的步骤已有 WebAssembly 函数。请参考 [functions/bin/install.sh](https://github.com/second-state/dapr-wasm/blob/main/functions/bin/install.sh) 脚本来安装 WasmEdge Runtime 的 Go SDK。

Sidecar 微服务运行一个事件循环，在 `/api/image` 路径上监听传入的 HTTP 请求。

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

一旦程序接收到通过 HTTP POST 请求发送的图像文件，就会调用 WasmEdge 中的 WebAssembly 函数执行基于 Tensorflow 的图像分类任务。它使用 WasmEdge 的 Go API 与 WebAssembly 程序进行交互。

```go
func imageHandlerWASI(_ context.Context, in *common.InvocationEvent) (out *common.Content, err error) {
  image := in.Data
  var conf = wasmedge.NewConfigure(wasmedge.REFERENCE_TYPES)
  conf.AddConfig(wasmedge.WASI)
  var vm = wasmedge.NewVMWithConfig(conf)
  var wasi = vm.GetImportObject(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],     // The args
    os.Environ(),    // The envs
    []string{".:."}, // The mapping directories
    []string{},      // The preopens will be empty
  )
  // Register WasmEdge-tensorflow and WasmEdge-image
  var tfobj = wasmedge.NewTensorflowImportObject()
  var tfliteobj = wasmedge.NewTensorflowLiteImportObject()
  vm.RegisterImport(tfobj)
  vm.RegisterImport(tfliteobj)
  var imgobj = wasmedge.NewImageImportObject()
  vm.RegisterImport(imgobj)
  vm.LoadWasmFile("./lib/classify_bg.wasm")
  vm.Validate()
  vm.Instantiate()
  res, err := vm.ExecuteBindgen("infer", wasmedge.Bindgen_return_array, image)
  ans := string(res.([]byte))
  
  vm.Delete()
  conf.Delete()
  out = &common.Content{
    Data:    []byte(ans),
    ContentType: in.ContentType,
    DataTypeURL: in.DataTypeURL,
  }
  return out, nil
}
```

执行以下 Dapr CLI 命令会在 Dapr runtime 环境中启动 image-api-go 微服务。

```bash
cd image-api-go
sudo dapr run --app-id image-api-go \
    --app-protocol http \
    --app-port 9003 \
    --dapr-http-port 3501 \
    --log-level debug \
    --components-path ../config \
    ./image-api-go
cd ../
```

## Web UI sidecar

Web UI 服务 [web-port](https://github.com/second-state/dapr-wasm/tree/main/web-port) 是一个用 Go 编写的简单 Web 服务器。它提供静态文件夹中的静态 HTML 和 JavaScript 文件，并将通过 `/api/hello` 上传的图像发送到 [grayscale](https://github.com/second-state/dapr-wasm/tree/main/image-api-rs) 或 [classify](https://github.com/second-state/dapr-wasm/tree/main/image-api-go) sidecar 的 `/api/image` 端点。

```go
func main() {
  http.HandleFunc("/static/", staticHandler)
  http.HandleFunc("/api/hello", imageHandler)
  println("listen to 8080 ...")
  log.Fatal(http.ListenAndServe(":8080", nil))
}

func staticHandler(w http.ResponseWriter, r *http.Request) {
  // ... 读取并返回 HTML CSS 和 JS 文件的内容 ...
}

func imageHandler(w http.ResponseWriter, r *http.Request) {
  // ... ...
  api := r.Header.Get("api")
  if api == "go" {
    daprClientSend(body, w)
  } else {
    httpClientSend(body, w)
  }
}

// 通过 Dapr API 发送到 image-api-go sidecar（分类）
func daprClientSend(image []byte, w http.ResponseWriter) {
  // ... ...
  resp, err := client.InvokeMethodWithContent(ctx, "image-api-go", "/api/image", "post", content)
  // ... ...
}

// 通过 HTTP API 发送到  image-api-rs sidecar（grayscale）
func httpClientSend(image []byte, w http.ResponseWriter) {
  // ... ...
  req, err := http.NewRequest("POST", "http://localhost:3502/v1.0/invoke/image-api-rs/method/api/image", bytes.NewBuffer(image))
  // ... ...
}
```

[page.js](https://github.com/second-state/dapr-wasm/blob/main/web-port/static/page.js) 中的 JavaScript 代码只实现了将图像上传到 [web-port](https://github.com/second-state/dapr-wasm/tree/main/web-port) sidecar 的 `/api/hello` 端点，[web-port](https://github.com/second-state/dapr-wasm/tree/main/web-port) 将根据请求头请求 classify（分类）或 grayscale 微服务的 api。

```javascript
function runWasm(e) {
  const reader = new FileReader();
  reader.onload = function (e) {
    setLoading(true);
    var req = new XMLHttpRequest();
    req.open("POST", '/api/hello', true);
    req.setRequestHeader('api', getApi());
    req.onload = function () {
      // ...  display results ...
    };
    const blob = new Blob([e.target.result], {
      type: 'application/octet-stream'
    });
    req.send(blob);
  };
  console.log(image.file)
  reader.readAsArrayBuffer(image.file);
}
```

执行以下 Dapr CLI 命令启动静态 UI 文件的 Web 服务。

```bash
cd web-port
sudo dapr run --app-id go-web-port \
    --app-protocol http \
    --app-port 8080 \
    --dapr-http-port 3500 \
    --components-path ../config \
    --log-level debug \
    ./web-port
cd ../
```

至此，你现在有一个用两种语言编写的并由三部分组成的分布式应用程序了！
