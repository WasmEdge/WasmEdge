# Tensorflow

AI 推理是一个计算密集任务，可以从 Rust 和 WebAssembly 的速度中受益颇多。然而，标准的 WebAssembly 沙箱只提供了对原生操作系统和硬件非常有限的访问权限，比如多核 CPU、GPU 和专用的 AI 推理芯片。对 AI 的负载来说，它也不理想。

WebAssembly 系统接口（WASI）提供了一种设计模式，可以让沙箱中的 WebAssembly 程序安全地访问原生宿主函数。WasmEdge Runtime 拓展了 WASI 的模型，支持在 WebAssembly 程序中直接访问原生 Tensorflow 的库。[WasmEdge Tensorflow Rust SDK](https://github.com/second-state/wasmedge_tensorflow_interface) 提供了安全、便携、易用的方式，以本地速度来运行 Tensorflow。

> 如果你对 Rust 不熟悉，可以尝试我们的[实验性的 DSL AI 推理示例](https://github.com/second-state/wasm-learning/tree/master/cli/classify_yml)和 [JavaScript 示例](../js/tensorflow.md)。

## 目录

* [一个 Rust 示例](#a-rust-example)
* [部署选项](#deployment-options)

## 一个 Rust 示例

### 前置条件

你需要安装 [WasmEdge](../../start/install.md) 和 [Rust](https://www.rust-lang.org/tools/install).

### 构建

克隆示例代码.

```bash
git clone https://github.com/second-state/wasm-learning/
cd cli/tflite
```

使用 Rust `Cargo` 来构建 WebAssembly 目标。

```bash
rustup target add wasm32-wasi
cargo build --target wasm32-wasi --release
```

### 运行

`wasmedge-tensorflow-lite` 工具是 WasmEdge 构建的，包含 Tensorflow 和 Tensorflow Lite 拓展。

```bash
$ wasmedge-tensorflow-lite target/wasm32-wasi/release/classify.wasm < grace_hopper.jpg
It is very likely a <a href='https://www.google.com/search?q=military uniform'>military uniform</a> in the picture
```

### 让它运行得更快

为了让 Tensorflow 推理可以运行得*更快*，你可以将它预先编译（AOT）为原生机器码，然后使用 WasmEdge 沙箱来运行原生代码。

```bash
$ wasmedgec target/wasm32-wasi/release/classify.wasm classify.wasm
$ wasmedge-tensorflow-lite classify.wasm < grace_hopper.jpg
It is very likely a <a href='https://www.google.com/search?q=military uniform'>military uniform</a> in the picture
```

### 浏览代码

使用 WasmEdge Tensorflow API 是很直观的。你可以在 [`main.rs`](https://github.com/second-state/wasm-learning/blob/master/cli/tflite/src/main.rs) 中阅读完整的源代码。

首先，它加载了在 ImageNet 上训练好的 TFLite 模型文件，以及对应的标签文件。标签文件中存储了模型输出的数值与待分类对象的英文名字的映射。

```rust
  let model_data: &[u8] = include_bytes!("models/mobilenet_v1_1.0_224/mobilenet_v1_1.0_224_quant.tflite");
  let labels = include_str!("models/mobilenet_v1_1.0_224/labels_mobilenet_quant_v1_224.txt");
```

接下来，它从 `STDIN` 加载图像，并将其转换为 Tensorflow Lite 模型所要求的格式。

```rust
  let mut buf = Vec::new();
  io::stdin().read_to_end(&mut buf).unwrap();

  let flat_img = wasmedge_tensorflow_interface::load_jpg_image_to_rgb8(&buf, 224, 224);
```

然后，程序运行了 TFLite 模型，输入模型所需的张量（这里指图像），接收模型的输出（这里指一个包含了数字的数组）。每一个数字都对应了标签文件中的一个对象的概率。

```rust
  let mut session = wasmedge_tensorflow_interface::Session::new(&model_data, wasmedge_tensorflow_interface::ModelType::TensorFlowLite);
  session.add_input("input", &flat_img, &[1, 224, 224, 3])
         .run();
  let res_vec: Vec<u8> = session.get_output("MobilenetV1/Predictions/Reshape_1");
```

让我们找到有最大概率的对象，然后在标签文件中查找它的名字。

```rust
  let mut i = 0;
  let mut max_index: i32 = -1;
  let mut max_value: u8 = 0;
  while i < res_vec.len() {
    let cur = res_vec[i];
    if cur > max_value {
      max_value = cur;
      max_index = i as i32;
    }
    i += 1;
  }

  let mut label_lines = labels.lines();
  for _i in 0..max_index {
    label_lines.next();
  }
```

最终，它向 `STDOUT` 输出了结果。

```rust
  let class_name = label_lines.next().unwrap().to_string();
  if max_value > 50 {
    println!("It {} a <a href='https://www.google.com/search?q={}'>{}</a> in the picture", confidence.to_string(), class_name, class_name);
  } else {
    println!("It does not appears to be any food item in the picture.");
  }
```

## 部署选项

以下的所有教程都使用了 [Tensorflow 的 WasmEdge Rust API](https://github.com/second-state/wasmedge_tensorflow_interface) 来创建 AI 推理函数。这些 Rust 函数被编译成 WebAssembly 然后随着 WasmEdge 一起被部署在云上。

### Serverless 函数

以下的教程展示了如何在公有云的 Serverless 平台上部署使用 Rust 编写的 WebAssembly 程序。WasmEdge Runtime 在这些平台上的 Docker 容器中运行。每一个 Serverless 平台都提供了让 WasmEdge Runtime 通过 `STDIN` 和 `STDOUT` 获取并输出数据的接口。

* [Vercel Serverless 函数](https://www.secondstate.io/articles/vercel-wasmedge-webassembly-rust/)
* [Netlify 函数](https://www.secondstate.io/articles/netlify-wasmedge-webassembly-rust-serverless/)
* [AWS Lambda](https://github.com/second-state/aws-lambda-wasm-runtime)
* [腾讯 Serverless 函数](https://github.com/second-state/tencent-scf-wasm-runtime) (中文)

### Second Sate FaaS 和 Node.js

以下的教程展示了如何在 Second State FaaS 平台上部署使用 Rust 编写的 WebAssembly 程序。由于 FaaS 服务在 Node.js 上运行，你可以按照这些教程在你自己的 Node.js 服务器上运行这些函数。

* [Tensorflow：使用 MobileNet 来进行图像分类](https://www.secondstate.io/articles/faas-image-classification/) | [在线演示](https://second-state.github.io/wasm-learning/faas/mobilenet/html/index.html)
* [Tensorflow：使用 MTCNN 模型来进行人脸识别](https://www.secondstate.io/articles/faas-face-detection/) | [在线演示](https://second-state.github.io/wasm-learning/faas/mtcnn/html/index.html)

### 服务网格

以下的教程展示了如何将使用 Rust 编写的 WebAssembly 函数和程序部署为 sidecar 微服务。

* [Dapr 模板](https://github.com/second-state/dapr-wasm)展示了如何使用 Go 和 Rust 来构建、部署 Dapr  sidecar 应用。然后这些 sidecar 应用使用 WasmEdge SDK 来启动处理微服务负载的 WebAssembly 程序。

### 数据流框架

以下的教程展示了如何将使用 Rust 编写的 WebAssembly 函数和程序部署为嵌入在为 AIoT 建立的数据流框架中的处理函数。

* [YoMo 模板](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/)启动了 WasmEdge Runtime 来处理来自一个智能工厂的摄像头数据流中的图像数据。
