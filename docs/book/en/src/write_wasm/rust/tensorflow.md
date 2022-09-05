# Tensorflow

AI inference is a computationally intensive task that could benefit greatly from the speed of Rust and WebAssembly. However, the standard WebAssembly sandbox provides very limited access to the native OS and hardware, such as multi-core CPUs, GPU and specialized AI inference chips. It is not ideal for the AI workload.

The popular WebAssembly System Interface (WASI) provides a design pattern for sandboxed WebAssembly programs to securely access native host functions. The WasmEdge Runtime extends the WASI model to support access to native Tensorflow libraries from WebAssembly programs. The [WasmEdge Tensorflow Rust SDK](https://github.com/second-state/wasmedge_tensorflow_interface) provides the security, portability, and ease-of-use of WebAssembly and native speed for Tensorflow.

> If you are not familiar with Rust, you can try our [experimental AI inference DSL](https://github.com/second-state/wasm-learning/tree/master/cli/classify_yml) or try our [JavaScript examples](../js/tensorflow.md).

## Table of contents

* [A Rust example](#a-rust-example)
* [Deployment options](#deployment-options)

## A Rust example

### Prerequisite

You need to install [WasmEdge](../../quick_start/install.md) and [Rust](https://www.rust-lang.org/tools/install).

### Build

Check out the example source code.

```bash
git clone https://github.com/second-state/wasm-learning/
cd cli/tflite
```

Use Rust `Cargo` to build the WebAssembly target.

```bash
rustup target add wasm32-wasi
cargo build --target wasm32-wasi --release
```

### Run

The `wasmedge-tensorflow-lite` utility is the WasmEdge build that includes the Tensorflow and Tensorflow Lite extensions.

```bash
$ wasmedge-tensorflow-lite target/wasm32-wasi/release/classify.wasm < grace_hopper.jpg
It is very likely a <a href='https://www.google.com/search?q=military uniform'>military uniform</a> in the picture
```

### Make it run faster

To make Tensorflow inference run *much* faster, you could AOT compile it down to machine native code, and then use WasmEdge sandbox to run the native code.

```bash
$ wasmedgec target/wasm32-wasi/release/classify.wasm classify.wasm
$ wasmedge-tensorflow-lite classify.wasm < grace_hopper.jpg
It is very likely a <a href='https://www.google.com/search?q=military uniform'>military uniform</a> in the picture
```

### Code walkthrough

It is fairly straightforward to use the WasmEdge Tensorflow API. You can see the entire source code in [main.rs](https://github.com/second-state/wasm-learning/blob/master/cli/tflite/src/main.rs).

First, it reads the trained TFLite model file (ImageNet) and its label file. The label file maps numeric output from the model to English names for the classified objects.

```rust
  let model_data: &[u8] = include_bytes!("models/mobilenet_v1_1.0_224/mobilenet_v1_1.0_224_quant.tflite");
  let labels = include_str!("models/mobilenet_v1_1.0_224/labels_mobilenet_quant_v1_224.txt");
```

Next, it reads the image from `STDIN` and converts it to the size and RGB pixel arrangement required by the Tensorflow Lite model.

```rust
  let mut buf = Vec::new();
  io::stdin().read_to_end(&mut buf).unwrap();

  let flat_img = wasmedge_tensorflow_interface::load_jpg_image_to_rgb8(&buf, 224, 224);
```

Then, the program runs the TFLite model with its required input tensor (i.e., the flat image in this case), and receives the model output. In this case, the model output is an array of numbers. Each number corresponds to the probability of an object name in the label text file.

```rust
  let mut session = wasmedge_tensorflow_interface::Session::new(&model_data, wasmedge_tensorflow_interface::ModelType::TensorFlowLite);
  session.add_input("input", &flat_img, &[1, 224, 224, 3])
         .run();
  let res_vec: Vec<u8> = session.get_output("MobilenetV1/Predictions/Reshape_1");
```

Let's find the object with the highest probability, and then look up the name in the labels file.

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

Finally, it prints the result to `STDOUT`.

```rust
  let class_name = label_lines.next().unwrap().to_string();
  if max_value > 50 {
    println!("It {} a <a href='https://www.google.com/search?q={}'>{}</a> in the picture", confidence.to_string(), class_name, class_name);
  } else {
    println!("It does not appears to be any food item in the picture.");
  }
```

## Deployment options

All the tutorials below use the [WasmEdge Rust API for Tensorflow](https://github.com/second-state/wasmedge_tensorflow_interface) to create AI inference functions. Those Rust functions are then compiled to WebAssembly and deployed together with WasmEdge on the cloud.

### Serverless functions

The following tutorials showcase how to deploy WebAssembly programs (written in Rust) on public cloud serverless platforms. The WasmEdge Runtime runs inside a Docker container on those platforms. Each serverless platform provides APIs to get data into and out of the WasmEdge runtime through STDIN and STDOUT.

* [Vercel Serverless Functions](https://www.secondstate.io/articles/vercel-wasmedge-webassembly-rust/)
* [Netlify Functions](https://www.secondstate.io/articles/netlify-wasmedge-webassembly-rust-serverless/)
* [AWS Lambda](https://github.com/second-state/aws-lambda-wasm-runtime)
* [Tencent Serverless Functions](https://github.com/second-state/tencent-scf-wasm-runtime) (in Chinese)

### Second Sate FaaS and Node.js

The following tutorials showcase how to deploy WebAssembly functions (written in Rust) on the Second State FaaS. Since the FaaS service is running on Node.js, you can follow the same tutorials for running those functions in your own Node.js server.

* [Tensorflow: Image classification using the MobileNet models](https://www.secondstate.io/articles/faas-image-classification/) | [Live demo](https://second-state.github.io/wasm-learning/faas/mobilenet/html/index.html)
* [Tensorflow: Face detection using the MTCNN models](https://www.secondstate.io/articles/faas-face-detection/) | [Live demo](https://second-state.github.io/wasm-learning/faas/mtcnn/html/index.html)

### Service mesh

The following tutorials showcase how to deploy WebAssembly functions and programs (written in Rust) as sidecar microservices.

* [The Dapr template](https://github.com/second-state/dapr-wasm) shows how to build and deploy Dapr sidecars in Go and Rust languages. The sidecars then use the WasmEdge SDK to start WebAssembly programs to process workloads to the microservices.

### Data streaming framework

The following tutorials showcase how to deploy WebAssembly functions (written in Rust) as embedded handler functions in data streaming frameworks for AIoT.

* [The YoMo template](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/) starts the WasmEdge Runtime to process image data as the data streams in from a camera in a smart factory.
