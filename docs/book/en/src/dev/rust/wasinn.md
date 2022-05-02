# Neural Network for WASI

In WasmEdge, we implemented the [WASI-NN](https://github.com/WebAssembly/wasi-nn) (Neural Network for WASI) proposal to allow access the Machine Learning (ML) functions with the fashion of graph loader APIs by the following functions:

* [`Load`](https://github.com/WebAssembly/wasi-nn/blob/f72b983c4cc91ac575af6babc57b5bccb7db7ba9/phases/ephemeral/witx/wasi_ephemeral_nn.witx#L108-L118) a model using variable opaque byte arrays
* [`Init_execution_context`](https://github.com/WebAssembly/wasi-nn/blob/f72b983c4cc91ac575af6babc57b5bccb7db7ba9/phases/ephemeral/witx/wasi_ephemeral_nn.witx#L125-L129) and bind some tensors to it using [`set_input`](https://github.com/WebAssembly/wasi-nn/blob/f72b983c4cc91ac575af6babc57b5bccb7db7ba9/phases/ephemeral/witx/wasi_ephemeral_nn.witx#L134-L142)
* [`Compute`](https://github.com/WebAssembly/wasi-nn/blob/f72b983c4cc91ac575af6babc57b5bccb7db7ba9/phases/ephemeral/witx/wasi_ephemeral_nn.witx#L165) the ML inference using the bound context
* Retrieve the inference result tensors using [`get_output`](https://github.com/WebAssembly/wasi-nn/blob/f72b983c4cc91ac575af6babc57b5bccb7db7ba9/phases/ephemeral/witx/wasi_ephemeral_nn.witx#L147-L160)

You can find more detail about the WASI-NN proposal in [Reference](#Reference).

In this section, we will use [an Rust example project](https://github.com/second-state/WasmEdge-WASINN-examples) ![badge](https://github.com/second-state/WasmEdge-WASINN-examples/actions/workflows/main.yaml/badge.svg) to demonstrate how to use the WASI-NN api and run a image classification demo.

## Prerequisite

Currently, WasmEdge used OpenVINO™ as the backend implementation. For this demo, you need to install [OpenVINO™](https://docs.openvino.ai/2021.4/openvino_docs_install_guides_installing_openvino_linux.html#)(2021) and [Rust](https://www.rust-lang.org/tools/install).

For installing OpenVINO™ on Ubuntu20.04, we recommend the following commands:

```bash
OPENVINO_VERSION="2021.4.582"
OPENVINO_YEAR="2021"
curl -sSL https://apt.repos.intel.com/openvino/$OPENVINO_YEAR/GPG-PUB-KEY-INTEL-OPENVINO-$OPENVINO_YEAR >./GPG-PUB-KEY-INTEL-OPENVINO-$OPENVINO_YEAR
apt-key add ./GPG-PUB-KEY-INTEL-OPENVINO-$OPENVINO_YEAR
echo "deb https://apt.repos.intel.com/openvino/$OPENVINO_YEAR all main" | tee /etc/apt/sources.list.d/intel-openvino-$OPENVINO_YEAR.list
apt update
apt install -y intel-openvino-runtime-ubuntu20-$OPENVINO_VERSION
```

By default, we don't enable any WASI-NN backend in WasmEdge. To enable the OpenVINO™ backend, we need to [building the WasmEdge from source](../../extend/build.md) with one more compiler flag `-DWASMEDGE_WASINN_BUILD_OPENVINO=ON` to enable the OpenVINO™ backend:

```bash
git clone https://github.com/WasmEdge/WasmEdge.git && cd WasmEdge
# If use docker
docker pull wasmedge/wasmedge
docker run -it --rm \
    -v <path/to/your/wasmedge/source/folder>:/root/wasmedge \
    wasmedge/wasmedge:latest
cd /root/wasmedge
# If you don't use docker then you need to run only the following commands in the cloned repository root
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_TESTS=ON -DWASMEDGE_WASINN_BUILD_OPENVINO=ON .. && make -j
```

(*Make sure you have configured the OpenVINO™ environment correctly with `source /opt/intel/openvino_2021/bin/setupvars.sh`* ). You should have an executable `wasmedge` runtime under `./build/tools/wasmedge/wasmedge`.

For Rust, make sure to add `wasm32-wasi` target with

```bash
rustup target add wasm32-wasi
```

## Run

Download the demo with:

```bash
git clone https://github.com/second-state/WasmEdge-WASINN-examples
cd WasmEdge-WASINN-examples
```

### Quick Start

The demo will download the [fixtures](https://github.com/intel/openvino-rs/raw/v0.3.3/crates/openvino/tests/fixtures/mobilenet/) for the [Mobilenet](https://arxiv.org/abs/1704.04861), and run a image classification:

```bash
./build_mobilenet_base.sh <WASMEDGE RUNTIME PATH>
# for example: 
# ./build_mobilenet_base.sh "<path to WasmEdge repo>/build/tools/wasmedge/wasmedge"
```

If everything goes well, you should have the terminal output:

```bash
# massive rust compiling output above...
Read graph XML, size in bytes: 143525
Read graph weights, size in bytes: 13956476
Loaded graph into wasi-nn with ID: 0
Created wasi-nn execution context with ID: 0
Read input tensor, size in bytes: 602112
Executed graph inference
   1.) [963](0.7113)pizza, pizza pie
   2.) [762](0.0707)restaurant, eating house, eating place, eatery
   3.) [909](0.0364)wok
   4.) [926](0.0155)hot pot, hotpot
   5.) [567](0.0153)frying pan, frypan, skillet
```

### Step by step

The detailed steps in `build_mobilenet_base.sh` is listed below.

To build the demo, run:

```bash
cd ./rust/examples/mobilenet-base
cargo build --release --target=wasm32-wasi
```

The outputted `mobilenet-base-example.wasm` will be under `target/wasm32-wasi/release/`(or [here](https://github.com/second-state/WasmEdge-WASINN-examples/blob/master/wasms/mobilenet-base-example.wasm)), we can find that the WASM file imports the necessary WASI-NN functions by converting into WAT format with tools like [`wasm2wat`](https://webassembly.github.io/wabt/demo/wasm2wat/):

```wasm
 ...
 (import "wasi_ephemeral_nn" "load" (func $_ZN7wasi_nn9generated17wasi_ephemeral_nn4load17hdca997591f45db43E (type 8)))
  (import "wasi_ephemeral_nn" "init_execution_context" (func $_ZN7wasi_nn9generated17wasi_ephemeral_nn22init_execution_context17h2cb3b4398c18d1fdE (type 4)))
  (import "wasi_ephemeral_nn" "set_input" (func $_ZN7wasi_nn9generated17wasi_ephemeral_nn9set_input17h4d10422433f5c246E (type 7)))
  (import "wasi_ephemeral_nn" "get_output" (func $_ZN7wasi_nn9generated17wasi_ephemeral_nn10get_output17h117ce8ea097ddbebE (type 8)))
  (import "wasi_ephemeral_nn" "compute" (func $_ZN7wasi_nn9generated17wasi_ephemeral_nn7compute17h96ef5b407fe8173aE (type 5)))
  ...
```

In order to successfully run this demo, we also need to download the fixtures of the ML model:

* `mobilenet.xml`: the model description
* `mobilenet.bin`: the model weights
* `tensor-1x3x224x224-f32.bgr`: the input image tensor.

The above artifacts are generated by [OpenVINO™ Model Optimizer](https://docs.openvino.ai/2021.4/openvino_docs_MO_DG_Deep_Learning_Model_Optimizer_DevGuide.html). Thanks for the amazing jobs done by Andrew Brown, you can find the artifacts and a `build.sh` which can regenerate the artifacts [here](https://github.com/intel/openvino-rs/tree/main/crates/openvino/tests/fixtures/mobilenet).

We recommend to put the fixtures as well as the `mobilenet-base-example.wasm` under the same path `./rust/examples/classification-example/build/`, and run:

```bash
RUST_BUILD_DIR="./rust/examples/classification-example/build/"
cd $RUST_BUILD_DIR
<WASMEDGE RUNTIME PATH> --dir fixture:$RUST_BUILD_DIR mobilenet-base-example.wasm "fixture/mobilenet.xml" "fixture/mobilenet.bin" "fixture/tensor-1x224x224x3-f32.bgr"
```

* `--dir fixture:$RUST_BUILD_DIR` means binding the `RUST_BUILD_DIR` to the `fixture/` in WASI virtual filesystem.
* The fixtures paths will be passed as `fixture/mobilenet.xml`, `fixture/mobilenet.bin`, `fixture/tensor-1x224x224x3-f32.bgr`

## Code walkthrough

The [main.rs](https://github.com/second-state/WasmEdge-WASINN-examples/blob/master/rust/mobilenet-base/src/main.rs) will be started as the entry point. First, read the model description and weights into memory:

```rust
let args: Vec<String> = env::args().collect();
let model_xml_name: &str = &args[1]; // read filename from commandline
let model_bin_name: &str = &args[2];
let tensor_name: &str = &args[3];

let xml = fs::read_to_string(model_xml_name).unwrap();
let weights = fs::read(model_bin_name).unwrap();
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
let tensor_data = fs::read(tensor_name).unwrap();
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

where `wasi_nn::GRAPH_ENCODING_OPENVINO` means using the OpenVION™ backend, and `wasi_nn::EXECUTION_TARGET_CPU` means running the computation on CPU.

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

## Reference

The intoduction of WASI-NN can be referred to [this amazing blog](https://bytecodealliance.org/articles/using-wasi-nn-in-wasmtime) written by Andrew Brown. This demo is greatly adapted from another [demo](https://github.com/radu-matei/wasi-nn-guest).
