# Neural Network for WASI

In WasmEdge, we implemented the [WASI-NN](https://github.com/WebAssembly/wasi-nn) (Neural Network for WASI) proposal to allow access the Machine Learning (ML) functions with the fashion of graph loader APIs by the following functions:

* [`Load`](https://github.com/WebAssembly/wasi-nn/blob/master/phases/ephemeral/witx/wasi_ephemeral_nn.witx#L108-L118) a model using variable opaque byte arrays
* [`Init_execution_context`](https://github.com/WebAssembly/wasi-nn/blob/master/phases/ephemeral/witx/wasi_ephemeral_nn.witx#L125-L129) and bind some tensors to it using [`set_input`](https://github.com/WebAssembly/wasi-nn/blob/master/phases/ephemeral/witx/wasi_ephemeral_nn.witx#L134-L142)
* [`Compute`](https://github.com/WebAssembly/wasi-nn/blob/master/phases/ephemeral/witx/wasi_ephemeral_nn.witx#L165-L168) the ML inference using the bound context
* Retrieve the inference result tensors using [`get_output`](https://github.com/WebAssembly/wasi-nn/blob/master/phases/ephemeral/witx/wasi_ephemeral_nn.witx#L147-L160)

You can find more detail about the WASI-NN proposal in [Reference](#reference).

In this section, we will use [the example repository](https://github.com/second-state/WasmEdge-WASINN-examples) to demonstrate how to use the [WASI-NN rust crate](https://crates.io/crates/wasi-nn) to write the WASM and run an image classification demo with WasmEdge WASI-NN plug-in.

* [Prerequisites](#prerequisites)
  * [OpenVINO backend](#get-wasmedge-with-wasi-nn-plug-in-openvino-backend)
  * [PyTorch beckend](#get-wasmedge-with-wasi-nn-plug-in-pytorch-backend)
  * TensorFlow backend (Work in progress)
  * [TensorFlow-Lite backend](#get-wasmedge-with-wasi-nn-plug-in-tensorflow-lite-backend)
* [Write WebAssembly Using WASI-NN](#write-webassembly-using-wasi-nn)
  * [OpenVINO backend example](#using-wasi-nn-with-openvino-backend-in-rust)
  * [PyTorch backend example](#using-wasi-nn-with-pytorch-backend-in-rust)
  * TensorFlow backend example (Work in progress)
  * [TensorFlow-Lite backend example](#using-wasi-nn-with-tensorflow-lite-backend-in-rust)
* [Run the examples](#run)
  * [Run OpenVINO backend example](#openvino-backend-example)
  * [Run PyTorch backend example](#pytorch-backend-example)
  * Run TensorFlow backend example (Work in progress)
  * [Run TensorFlow-Lite backend example](#tensorflow-lite-backend-example)

## Prerequisites

Currently, WasmEdge used OpenVINO™ or PyTorch as the WASI-NN backend implementation. For using WASI-NN on WasmEdge, you need to install [OpenVINO™](https://docs.openvino.ai/2021.4/openvino_docs_install_guides_installing_openvino_linux.html#)(2021) or [PyTorch 1.8.2 LTS](https://pytorch.org/get-started/locally/) for the backend.

In the current status, the [WasmEdge Installer](../../quick_start/install.md) will install the `manylinux2014` version of WasmEdge releases, but the WASI-NN plug-in for WasmEdge only supports `Ubuntu 20.04` or later now. Please refer to the following steps to get the WasmEdge with WASI-NN plug-in.

You can also [build WasmEdge with WASI-NN plug-in from source](../../contribute/build_from_src/plugin_wasi_nn.md).

### Get WasmEdge with WASI-NN Plug-in OpenVINO Backend

First you should [install the OpenVINO dependency](../../contribute/build_from_src/plugin_wasi_nn.md#build-wasmedge-with-wasi-nn-openvino-backend):

```bash
export OPENVINO_VERSION="2021.4.582"
export OPENVINO_YEAR="2021"
curl -sSL https://apt.repos.intel.com/openvino/$OPENVINO_YEAR/GPG-PUB-KEY-INTEL-OPENVINO-$OPENVINO_YEAR | sudo gpg --dearmor > /usr/share/keyrings/GPG-PUB-KEY-INTEL-OPENVINO-$OPENVINO_YEAR.gpg
echo "deb [signed-by=/usr/share/keyrings/GPG-PUB-KEY-INTEL-OPENVINO-$OPENVINO_YEAR.gpg] https://apt.repos.intel.com/openvino/$OPENVINO_YEAR all main" | sudo tee /etc/apt/sources.list.d/intel-openvino-$OPENVINO_YEAR.list
sudo apt update
sudo apt install -y intel-openvino-runtime-ubuntu20-$OPENVINO_VERSION
source /opt/intel/openvino_2021/bin/setupvars.sh
ldconfig
```

And then get the WasmEdge and the WASI-NN plug-in with OpenVINO backend:

```bash
curl -sLO https://github.com/WasmEdge/WasmEdge/releases/download/{{ wasmedge_version }}/WasmEdge-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
tar -zxf WasmEdge-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
rm -f WasmEdge-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
curl -sLO https://github.com/WasmEdge/WasmEdge/releases/download/{{ wasmedge_version }}/WasmEdge-plugin-wasi_nn-openvino-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
tar -zxf WasmEdge-plugin-wasi_nn-openvino-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
rm -f WasmEdge-plugin-wasi_nn-openvino-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
mv libwasmedgePluginWasiNN.so WasmEdge-{{ wasmedge_version }}-Linux/lib/wasmedge
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/WasmEdge-{{ wasmedge_version }}-Linux/lib
export PATH=$PATH:$(pwd)/WasmEdge-{{ wasmedge_version }}-Linux/bin
export WASMEDGE_PLUGIN_PATH=$(pwd)/WasmEdge-{{ wasmedge_version }}-Linux/lib/wasmedge
```

### Get WasmEdge with WASI-NN Plug-in PyTorch Backend

First you should [install the PyTorch dependency](../../contribute/build_from_src/plugin_wasi_nn.md#build-wasmedge-with-wasi-nn-pytorch-backend):

```bash
export PYTORCH_VERSION="1.8.2"
curl -s -L -O --remote-name-all https://download.pytorch.org/libtorch/lts/1.8/cpu/libtorch-cxx11-abi-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip
unzip -q "libtorch-cxx11-abi-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip"
rm -f "libtorch-cxx11-abi-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip"
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$(pwd)/libtorch/lib
```

And then get the WasmEdge and the WASI-NN plug-in with PyTorch backend:

```bash
curl -sLO https://github.com/WasmEdge/WasmEdge/releases/download/{{ wasmedge_version }}/WasmEdge-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
tar -zxf WasmEdge-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
rm -f WasmEdge-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
curl -sLO https://github.com/WasmEdge/WasmEdge/releases/download/{{ wasmedge_version }}/WasmEdge-plugin-wasi_nn-pytorch-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
tar -zxf WasmEdge-plugin-wasi_nn-pytorch-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
rm -f WasmEdge-plugin-wasi_nn-pytorch-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
mv libwasmedgePluginWasiNN.so WasmEdge-{{ wasmedge_version }}-Linux/lib/wasmedge
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/WasmEdge-{{ wasmedge_version }}-Linux/lib
export PATH=$PATH:$(pwd)/WasmEdge-{{ wasmedge_version }}-Linux/bin
export WASMEDGE_PLUGIN_PATH=$(pwd)/WasmEdge-{{ wasmedge_version }}-Linux/lib/wasmedge
```

> The WasmEdge installer would install the `manylinux2014` version for Ubuntu. If you install WasmEdge with the installer or for the `manylinux2014` version, you should get the `manylinux2014` version plug-in and `libtorch`.

### Get WasmEdge with WASI-NN Plug-in TensorFlow-Lite Backend

First you should [install the TensorFlow-Lite dependency](../../contribute/build_from_src/plugin_wasi_nn.md#build-wasmedge-with-wasi-nn-tensorflow-lite-backend):

```bash
curl -s -L -O --remote-name-all https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/{{ wasmedge_version }}/WasmEdge-tensorflow-deps-TFLite-{{ wasmedge_version }}-manylinux2014_x86_64.tar.gz
tar -zxf WasmEdge-tensorflow-deps-TFLite-{{ wasmedge_version }}-manylinux2014_x86_64.tar.gz
rm -f WasmEdge-tensorflow-deps-TFLite-{{ wasmedge_version }}-manylinux2014_x86_64.tar.gz
```

The shared library will be extracted in the current directory `./libtensorflowlite_c.so`.

Then you can move the library to the installation path:

```bash
mv libtensorflowlite_c.so /usr/local/lib
```

Or set the environment variable `export LD_LIBRARY_PATH=$(pwd):${LD_LIBRARY_PATH}`.

And then get the WasmEdge and the WASI-NN plug-in with TensorFlow-Lite backend:

```bash
curl -sLO https://github.com/WasmEdge/WasmEdge/releases/download/{{ wasmedge_version }}/WasmEdge-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
tar -zxf WasmEdge-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
rm -f WasmEdge-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
curl -sLO https://github.com/WasmEdge/WasmEdge/releases/download/{{ wasmedge_version }}/WasmEdge-plugin-wasi_nn-tensorflowlite-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
tar -zxf WasmEdge-plugin-wasi_nn-tensorflowlite-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
rm -f WasmEdge-plugin-wasi_nn-tensorflowlite-{{ wasmedge_version }}-ubuntu20.04_x86_64.tar.gz
mv libwasmedgePluginWasiNN.so WasmEdge-{{ wasmedge_version }}-Linux/lib/wasmedge
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/WasmEdge-{{ wasmedge_version }}-Linux/lib
export PATH=$PATH:$(pwd)/WasmEdge-{{ wasmedge_version }}-Linux/bin
export WASMEDGE_PLUGIN_PATH=$(pwd)/WasmEdge-{{ wasmedge_version }}-Linux/lib/wasmedge
```

> The WasmEdge installer would install the `manylinux2014` version for Ubuntu. If you install WasmEdge with the installer or for the `manylinux2014` version, you should get the `manylinux2014` version plug-in.
>
> We also provided [various TensorFlow-Lite pre-built libraries](https://github.com/second-state/WasmEdge-tensorflow-deps/releases/tag/0.11.1), such as `manylinux2014_aarch64`.

## Write WebAssembly Using WASI-NN

You can refer to the [OpenVINO backend example](https://github.com/second-state/WasmEdge-WASINN-examples/tree/master/openvino-mobilenet-image) and the [PyTorch backend example](https://github.com/second-state/WasmEdge-WASINN-examples/tree/master/pytorch-mobilenet-image).

### (Optional) Rust Installation

If you want to build the example WASM from Rust by yourself, the [Rust inatallation](https://doc.rust-lang.org/book/ch01-01-installation.html) is required.

```bash
curl --proto '=https' --tlsv1.2 https://sh.rustup.rs -sSf | sh -s -- -y
source "$HOME/.cargo/env"
```

And make sure to add `wasm32-wasi` target with the following command:

```bash
rustup target add wasm32-wasi
```

### (Optional) Building the WASM File From Rust Source

First get the example repository:

```bash
git clone https://github.com/second-state/WasmEdge-WASINN-examples.git
cd WasmEdge-WASINN-examples
```

To build the OpenVINO example WASM, run:

```bash
cd openvino-mobilenet-image/rust
cargo build --release --target=wasm32-wasi
```

The outputted [`wasmedge-wasinn-example-mobilenet-image.wasm`](https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/openvino-mobilenet-image/wasmedge-wasinn-example-mobilenet-image.wasm) will be under `openvino-mobilenet-image/rust/target/wasm32-wasi/release/`.

To build the PyTorch example WASM, run:

```bash
cd pytorch-mobilenet-image/rust
cargo build --release --target=wasm32-wasi
```

The outputted [`wasmedge-wasinn-example-mobilenet-image.wasm`](https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/pytorch-mobilenet-image/wasmedge-wasinn-example-mobilenet-image.wasm) will be under `pytorch-mobilenet-image/rust/target/wasm32-wasi/release/`.

To build the TensorFlow-Lite example WASM, run:

```bash
cd tflite-birds_v1-image/rust/tflite-bird
cargo build --release --target=wasm32-wasi
```

The outputted [`wasmedge-wasinn-example-tflite-bird-image.wasm`](https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/tflite-birds_v1-image/wasmedge-wasinn-example-tflite-bird-image.wasm) will be under `tflite-birds_v1-image/rust/tflite-bird/target/wasm32-wasi/release/`.

We can find that the outputted WASM files import the necessary WASI-NN functions by converting into WAT format with tools like [`wasm2wat`](https://webassembly.github.io/wabt/demo/wasm2wat/):

```wasm
 ...
 (import "wasi_ephemeral_nn" "load" (func $_ZN7wasi_nn9generated17wasi_ephemeral_nn4load17hdca997591f45db43E (type 8)))
  (import "wasi_ephemeral_nn" "init_execution_context" (func $_ZN7wasi_nn9generated17wasi_ephemeral_nn22init_execution_context17h2cb3b4398c18d1fdE (type 4)))
  (import "wasi_ephemeral_nn" "set_input" (func $_ZN7wasi_nn9generated17wasi_ephemeral_nn9set_input17h4d10422433f5c246E (type 7)))
  (import "wasi_ephemeral_nn" "get_output" (func $_ZN7wasi_nn9generated17wasi_ephemeral_nn10get_output17h117ce8ea097ddbebE (type 8)))
  (import "wasi_ephemeral_nn" "compute" (func $_ZN7wasi_nn9generated17wasi_ephemeral_nn7compute17h96ef5b407fe8173aE (type 5)))
  ...
```

### Using WASI-NN with OpenVINO Backend in Rust

The [main.rs](https://github.com/second-state/WasmEdge-WASINN-examples/tree/master/openvino-mobilenet-image/rust/src/main.rs) is the full example Rust source.

First, read the model description and weights into memory:

```rust
let args: Vec<String> = env::args().collect();
let model_xml_name: &str = &args[1]; // File name for the model xml
let model_bin_name: &str = &args[2]; // File name for the weights
let image_name: &str = &args[3]; // File name for the input image

let xml = fs::read_to_string(model_xml_name).unwrap();
let weights = fs::read(model_bin_name).unwrap();
```

We should use a helper function to convert the input image into the tensor data (the tensor type is `F32`):

```rust
fn image_to_tensor(path: String, height: u32, width: u32) -> Vec<u8> {
  let pixels = Reader::open(path).unwrap().decode().unwrap();
  let dyn_img: DynamicImage = pixels.resize_exact(width, height, image::imageops::Triangle);
  let bgr_img = dyn_img.to_bgr8();
  // Get an array of the pixel values
  let raw_u8_arr: &[u8] = &bgr_img.as_raw()[..];
  // Create an array to hold the f32 value of those pixels
  let bytes_required = raw_u8_arr.len() * 4;
  let mut u8_f32_arr: Vec<u8> = vec![0; bytes_required];

  for i in 0..raw_u8_arr.len() {
    // Read the number as a f32 and break it into u8 bytes
    let u8_f32: f32 = raw_u8_arr[i] as f32;
    let u8_bytes = u8_f32.to_ne_bytes();

    for j in 0..4 {
      u8_f32_arr[(i * 4) + j] = u8_bytes[j];
    }
  }
  return u8_f32_arr;
}
```

And use this helper funcion to convert the input image:

```rust
let tensor_data = image_to_tensor(image_name.to_string(), 224, 224);
```

Now we can start our inference with WASI-NN:

```rust
// load model
let graph = unsafe {
  wasi_nn::load(
    &[&xml.into_bytes(), &weights],
    wasi_nn::GRAPH_ENCODING_OPENVINO,
    wasi_nn::EXECUTION_TARGET_CPU,
  )
  .unwrap()
};
// initialize the computation context
let context = unsafe { wasi_nn::init_execution_context(graph).unwrap() };
// initialize the input tensor
let tensor = wasi_nn::Tensor {
  dimensions: &[1, 3, 224, 224],
  type_: wasi_nn::TENSOR_TYPE_F32,
  data: &tensor_data,
};
// set_input
unsafe {
  wasi_nn::set_input(context, 0, tensor).unwrap();
}
// Execute the inference.
unsafe {
  wasi_nn::compute(context).unwrap();
}
// retrieve output
let mut output_buffer = vec![0f32; 1001];
unsafe {
  wasi_nn::get_output(
    context,
    0,
    &mut output_buffer[..] as *mut [f32] as *mut u8,
    (output_buffer.len() * 4).try_into().unwrap(),
  )
  .unwrap();
}
```

Where the `wasi_nn::GRAPH_ENCODING_OPENVINO` means using the OpenVINO™ backend, and `wasi_nn::EXECUTION_TARGET_CPU` means running the computation on CPU.

Finally, we sort the output and then print the top-5 classification result:

```rust
let results = sort_results(&output_buffer);
for i in 0..5 {
  println!(
    "   {}.) [{}]({:.4}){}",
    i + 1,
    results[i].0,
    results[i].1,
    imagenet_classes::IMAGENET_CLASSES[results[i].0]
  );
}
```

### Using WASI-NN with PyTorch Backend in Rust

The [main.rs](https://github.com/second-state/WasmEdge-WASINN-examples/tree/master/pytorch-mobilenet-image/rust/src/main.rs) is the full example Rust source.

First, read the model description and weights into memory:

```rust
let args: Vec<String> = env::args().collect();
let model_bin_name: &str = &args[1]; // File name for the pytorch model
let image_name: &str = &args[2]; // File name for the input image

let weights = fs::read(model_bin_name).unwrap();
```

We should use a helper function to convert the input image into the tensor data (the tensor type is `F32`):

```rust
fn image_to_tensor(path: String, height: u32, width: u32) -> Vec<u8> {
  let mut file_img = File::open(path).unwrap();
  let mut img_buf = Vec::new();
  file_img.read_to_end(&mut img_buf).unwrap();
  let img = image::load_from_memory(&img_buf).unwrap().to_rgb8();
  let resized =
      image::imageops::resize(&img, height, width, ::image::imageops::FilterType::Triangle);
  let mut flat_img: Vec<f32> = Vec::new();
  for rgb in resized.pixels() {
    flat_img.push((rgb[0] as f32 / 255. - 0.485) / 0.229);
    flat_img.push((rgb[1] as f32 / 255. - 0.456) / 0.224);
    flat_img.push((rgb[2] as f32 / 255. - 0.406) / 0.225);
  }
  let bytes_required = flat_img.len() * 4;
  let mut u8_f32_arr: Vec<u8> = vec![0; bytes_required];

  for c in 0..3 {
    for i in 0..(flat_img.len() / 3) {
      // Read the number as a f32 and break it into u8 bytes
      let u8_f32: f32 = flat_img[i * 3 + c] as f32;
      let u8_bytes = u8_f32.to_ne_bytes();

      for j in 0..4 {
        u8_f32_arr[((flat_img.len() / 3 * c + i) * 4) + j] = u8_bytes[j];
      }
    }
  }
  return u8_f32_arr;
}
```

And use this helper funcion to convert the input image:

```rust
let tensor_data = image_to_tensor(image_name.to_string(), 224, 224);
```

Now we can start our inference with WASI-NN:

```rust
// load model
let graph = unsafe {
  wasi_nn::load(
    &[&weights],
    wasi_nn::GRAPH_ENCODING_PYTORCH,
    wasi_nn::EXECUTION_TARGET_CPU,
  )
  .unwrap()
};
// initialize the computation context
let context = unsafe { wasi_nn::init_execution_context(graph).unwrap() };
// initialize the input tensor
let tensor = wasi_nn::Tensor {
  dimensions: &[1, 3, 224, 224],
  type_: wasi_nn::TENSOR_TYPE_F32,
  data: &tensor_data,
};
// set_input
unsafe {
  wasi_nn::set_input(context, 0, tensor).unwrap();
}
// Execute the inference.
unsafe {
  wasi_nn::compute(context).unwrap();
}
// retrieve output
let mut output_buffer = vec![0f32; 1001];
unsafe {
  wasi_nn::get_output(
    context,
    0,
    &mut output_buffer[..] as *mut [f32] as *mut u8,
    (output_buffer.len() * 4).try_into().unwrap(),
  )
  .unwrap();
}
```

Where the `wasi_nn::GRAPH_ENCODING_PYTORCH` means using the PyTorch backend, and `wasi_nn::EXECUTION_TARGET_CPU` means running the computation on CPU.

Finally, we sort the output and then print the top-5 classification result:

```rust
let results = sort_results(&output_buffer);
for i in 0..5 {
  println!(
    "   {}.) [{}]({:.4}){}",
    i + 1,
    results[i].0,
    results[i].1,
    imagenet_classes::IMAGENET_CLASSES[results[i].0]
  );
}
```

### Using WASI-NN with TensorFlow-Lite Backend in Rust

The [main.rs](https://github.com/second-state/WasmEdge-WASINN-examples/blob/master/tflite-birds_v1-image/rust/tflite-bird/src/main.rs) is the full example Rust source.

First, read the model description and weights into memory:

```rust
let args: Vec<String> = env::args().collect();
let model_bin_name: &str = &args[1]; // File name for the tflite model
let image_name: &str = &args[2]; // File name for the input image

let weights = fs::read(model_bin_name).unwrap();
```

We should use a helper function to convert the input image into the tensor data (the tensor type is `U8`):

```rust
fn image_to_tensor(path: String, height: u32, width: u32) -> Vec<u8> {
    let pixels = Reader::open(path).unwrap().decode().unwrap();
    let dyn_img: DynamicImage = pixels.resize_exact(width, height, image::imageops::Triangle);
    let bgr_img = dyn_img.to_rgb8();
    // Get an array of the pixel values
    let raw_u8_arr: &[u8] = &bgr_img.as_raw()[..];
    return raw_u8_arr.to_vec();
}
```

And use this helper funcion to convert the input image:

```rust
let tensor_data = image_to_tensor(image_name.to_string(), 224, 224);
```

Now we can start our inference with WASI-NN:

```rust
// load model
let graph = unsafe {
  wasi_nn::load(
    &[&weights],
    4, //wasi_nn::GRAPH_ENCODING_TENSORFLOWLITE
    wasi_nn::EXECUTION_TARGET_CPU,
  )
  .unwrap()
};
// initialize the computation context
let context = unsafe { wasi_nn::init_execution_context(graph).unwrap() };
// initialize the input tensor
let tensor = wasi_nn::Tensor {
  dimensions: &[1, 3, 224, 224],
  r#type: wasi_nn::TENSOR_TYPE_F32,
  data: &tensor_data,
};
// set_input
unsafe {
  wasi_nn::set_input(context, 0, tensor).unwrap();
}
// Execute the inference.
unsafe {
  wasi_nn::compute(context).unwrap();
}
// retrieve output
let mut output_buffer = vec![0f32; 1001];
unsafe {
  wasi_nn::get_output(
    context,
    0,
    &mut output_buffer[..] as *mut [f32] as *mut u8,
    (output_buffer.len() * 4).try_into().unwrap(),
  )
  .unwrap();
}
```

Where the `wasi_nn::GRAPH_ENCODING_TENSORFLOWLITE` means using the PyTorch backend (now use the value `4` instead), and `wasi_nn::EXECUTION_TARGET_CPU` means running the computation on CPU.

> Note: Here we use the `wasi-nn 0.1.0` in current. After the `TENSORFLOWLITE` added into the graph encoding, we'll update this example to use the newer version.

Finally, we sort the output and then print the top-5 classification result:

```rust
let results = sort_results(&output_buffer);
for i in 0..5 {
  println!(
    "   {}.) [{}]({:.4}){}",
    i + 1,
    results[i].0,
    results[i].1,
    imagenet_classes::IMAGENET_CLASSES[results[i].0]
  );
}
```

## Run

### OpenVINO Backend Example

Please [install WasmEdge with the WASI-NN OpenVINO backend plug-in](#get-wasmedge-with-wasi-nn-plug-in-openvino-backend) first.

For the example demo of [Mobilenet](https://arxiv.org/abs/1704.04861), we need the [fixture files](https://github.com/intel/openvino-rs/raw/v0.3.3/crates/openvino/tests/fixtures/mobilenet/):

* `wasmedge-wasinn-example-mobilenet.wasm`: the [built WASM from rust](https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/openvino-mobilenet-image/wasmedge-wasinn-example-mobilenet-image.wasm)
* `mobilenet.xml`: the model description.
* `mobilenet.bin`: the model weights.
* `input.jpg`: the input image (224x224 JPEG).

The above Mobilenet artifacts are generated by [OpenVINO™ Model Optimizer](https://docs.openvino.ai/2021.4/openvino_docs_MO_DG_Deep_Learning_Model_Optimizer_DevGuide.html). Thanks for the amazing jobs done by Andrew Brown, you can find the artifacts and a `build.sh` which can regenerate the artifacts [here](https://github.com/intel/openvino-rs/tree/main/crates/openvino/tests/fixtures/mobilenet).

You can download these files by the following commands:

```bash
curl -sLO https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/openvino-mobilenet-image/wasmedge-wasinn-example-mobilenet-image.wasm
curl -sLO https://github.com/intel/openvino-rs/raw/v0.3.3/crates/openvino/tests/fixtures/mobilenet/mobilenet.bin
curl -sLO https://github.com/intel/openvino-rs/raw/v0.3.3/crates/openvino/tests/fixtures/mobilenet/mobilenet.xml
curl -sL -o input.jpg https://github.com/bytecodealliance/wasi-nn/raw/main/rust/examples/images/1.jpg
```

Then you can use the OpenVINO-enabled WasmEdge which was compiled above to execute the WASM file (in interpreter mode):

```bash
wasmedge --dir .:. wasmedge-wasinn-example-mobilenet-image.wasm mobilenet.xml mobilenet.bin input.jpg
# If you didn't install the project, you should give the `WASMEDGE_PLUGIN_PATH` environment variable for specifying the WASI-NN plugin path.
```

If everything goes well, you should have the terminal output:

```bash
Read graph XML, size in bytes: 143525
Read graph weights, size in bytes: 13956476
Loaded graph into wasi-nn with ID: 0
Created wasi-nn execution context with ID: 0
Read input tensor, size in bytes: 602112
Executed graph inference
   1.) [954](0.9789)banana
   2.) [940](0.0074)spaghetti squash
   3.) [951](0.0014)lemon
   4.) [969](0.0005)eggnog
   5.) [942](0.0005)butternut squash
```

For the AOT mode which is much more quickly, you can compile the WASM first:

```bash
wasmedgec wasmedge-wasinn-example-mobilenet.wasm out.wasm
wasmedge --dir .:. out.wasm mobilenet.xml mobilenet.bin input.jpg
```

### PyTorch Backend Example

Please [install WasmEdge with the WASI-NN PyTorch backend plug-in](#get-wasmedge-with-wasi-nn-plug-in-pytorch-backend) first.

For the example demo of [Mobilenet](https://arxiv.org/abs/1704.04861), we need these following files:

* `wasmedge-wasinn-example-mobilenet.wasm`: the [built WASM from rust](https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/pytorch-mobilenet-image/wasmedge-wasinn-example-mobilenet-image.wasm)
* `mobilenet.pt`: the PyTorch Mobilenet model.
* `input.jpg`: the input image (224x224 JPEG).

The above Mobilenet PyTorch model is generated by [the Python code](https://github.com/second-state/WasmEdge-WASINN-examples/blob/master/pytorch-mobilenet-image/gen_mobilenet_model.py).

You can download these files by the following commands:

```bash
curl -sLO https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/pytorch-mobilenet-image/wasmedge-wasinn-example-mobilenet-image.wasm
curl -sLO https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/pytorch-mobilenet-image/mobilenet.pt
curl -sL -o input.jpg https://github.com/bytecodealliance/wasi-nn/raw/main/rust/examples/images/1.jpg
```

Then you can use the PyTorch-enabled WasmEdge which was compiled above to execute the WASM file (in interpreter mode):

```bash
# Please check that you've already install the libtorch and set the `LD_LIBRARY_PATH`.
wasmedge --dir .:. wasmedge-wasinn-example-mobilenet-image.wasm mobilenet.pt input.jpg
# If you didn't install the project, you should give the `WASMEDGE_PLUGIN_PATH` environment variable for specifying the WASI-NN plugin path.
```

If everything goes well, you should have the terminal output:

```bash
Read torchscript binaries, size in bytes: 14376924
Loaded graph into wasi-nn with ID: 0
Created wasi-nn execution context with ID: 0
Read input tensor, size in bytes: 602112
Executed graph inference
   1.) [954](20.6681)banana
   2.) [940](12.1483)spaghetti squash
   3.) [951](11.5748)lemon
   4.) [950](10.4899)orange
   5.) [953](9.4834)pineapple, ananas
```

For the AOT mode which is much more quickly, you can compile the WASM first:

```bash
wasmedgec wasmedge-wasinn-example-mobilenet.wasm out.wasm
wasmedge --dir .:. out.wasm mobilenet.pt input.jpg
```

### TensorFlow-Lite Backend Example

Please [install WasmEdge with the WASI-NN TensorFlow-Lite backend plug-in](#get-wasmedge-with-wasi-nn-plug-in-tensorflow-lite-backend) first.

For the example demo of [Bird v1](https://tfhub.dev/google/aiy/vision/classifier/birds_V1/1), we need these following files:

* `wasmedge-wasinn-example-tflite-bird-image.wasm`: the [built WASM from rust](https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/tflite-birds_v1-image/wasmedge-wasinn-example-tflite-bird-image.wasm)
* `lite-model_aiy_vision_classifier_birds_V1_3.tflite`: the TensorFlow-Lite bird_v1 model.
* `input.jpg`: the input image (224x224 JPEG).

The above Mobilenet PyTorch model is generated by [the Python code](https://github.com/second-state/WasmEdge-WASINN-examples/blob/master/pytorch-mobilenet-image/gen_mobilenet_model.py).

You can download these files by the following commands:

```bash
curl -sLO https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/tflite-birds_v1-image/wasmedge-wasinn-example-tflite-bird-image.wasm
curl -sLO https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/tflite-birds_v1-image/lite-model_aiy_vision_classifier_birds_V1_3.tflite
curl -sLO https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/tflite-birds_v1-image/bird.jpg
```

Then you can use the PyTorch-enabled WasmEdge which was compiled above to execute the WASM file (in interpreter mode):

```bash
# Please check that you've already install the libtensorflowlite_c.so and set the `LD_LIBRARY_PATH`.
wasmedge --dir .:. wasmedge-wasinn-example-tflite-bird-image.wasm lite-model_aiy_vision_classifier_birds_V1_3.tflite bird.jpg
# If you didn't install the project, you should give the `WASMEDGE_PLUGIN_PATH` environment variable for specifying the WASI-NN plugin path.
```

If everything goes well, you should have the terminal output:

```bash
Read graph weights, size in bytes: 3561598
Loaded graph into wasi-nn with ID: 0
Created wasi-nn execution context with ID: 0
Read input tensor, size in bytes: 150528
Executed graph inference
   1.) [166](198)Aix galericulata
   2.) [158](2)Coccothraustes coccothraustes
   3.) [34](1)Gallus gallus domesticus
   4.) [778](1)Sitta europaea
   5.) [819](1)Anas platyrhynchos
```

For the AOT mode which is much more quickly, you can compile the WASM first:

```bash
wasmedgec wasmedge-wasinn-example-tflite-bird-image.wasm out.wasm
wasmedge --dir .:. out.wasm lite-model_aiy_vision_classifier_birds_V1_3.tflite bird.jpg
```

## Reference

The intoduction of WASI-NN can be referred to [this amazing blog](https://bytecodealliance.org/articles/using-wasi-nn-in-wasmtime) written by Andrew Brown. This demo is greatly adapted from another [demo](https://github.com/radu-matei/wasi-nn-guest).
