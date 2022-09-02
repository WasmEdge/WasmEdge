# YoMo

[YoMo](https://yomo.run/) is a programming framework enabling developers to build a distributed cloud system (Geo-Distributed Cloud System). YoMo's communication layer is made on top of the QUIC protocol, which brings high-speed data transmission. In addition, it has a built-in Streaming Serverless "streaming function", which significantly improves the development experience of distributed cloud systems. The distributed cloud system built by YoMo provides an ultra-high-speed communication mechanism between near-field computing power and terminals. It has a wide range of use cases in Metaverse, VR/AR, IoT, etc.

> YoMo is written in the Go language. For streaming Serverless, Golang plugins and shared libraries are used to load users' code dynamically, which also have certain limitations for developers. Coupled with Serverless architecture's rigid demand for isolation, this makes WebAssembly an excellent choice for running user-defined functions.

For example, in the process of real-time AI inference in AR/VR devices or smart factories, the camera sends real-time unstructured data to the computing node in the near-field MEC (multi-access edge computing) device through YoMo.  YoMo sends the AI computing result to the end device in real-time when the AI inference is completed. Thus, the hosted AI inference function will be automatically executed.  

However, a challenge for YoMo is to incorporate and manage handler functions written by multiple outside developers in an edge computing node. It requires runtime isolation for those functions without sacrificing performance. Traditional software container solutions, such as Docker, are not up to the task. They are too heavy and slow to handle real-time tasks.

WebAssembly provides a lightweight and high-performance software container. It is ideally suited as a runtime for YoMo’s data processing handler functions.

In this article, we will show you how to create a Rust function for Tensorflow-based image classification, compile it into WebAssembly, and then use YoMo to run it as a stream data handler. We use [WasmEdge](https://wasmedge.org/) as our WebAssembly runtime because it offers the highest performance and flexibility compared with other WebAssembly runtimes. It is the only WebAssembly VM that reliably supports Tensorflow. YoMo manages WasmEdge VM instances and the contained WebAssembly bytecode apps through [WasmEdge’s Golang API](../../../sdk/go.md).

> Source code: <https://github.com/yomorun/yomo-wasmedge-tensorflow>

Checkout [the WasmEdge image classification function in action in YoMo](https://www.youtube.com/watch?v=E0ltsn6cLIU)

## Prerequisite

Obviously, you will need to have [Golang installed](https://golang.org/doc/install), but I will assume you already did.

> Golang version should be newer than 1.15 for our example to work.

You also need to install the YoMo CLI application. It orchestrates and coordinates data streaming and handler function invocations.

```bash
$ go install github.com/yomorun/cli/yomo@latest
$ yomo version
YoMo CLI version: v0.1.3
```

Next, please install the WasmEdge and its Tensorflow shared libraries. [WasmEdge](https://wasmedge.org/) is a leading WebAssembly runtime hosted by the CNCF. We will use it to embed and run WebAssembly programs from YoMo.

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash
```

Finally, since our demo WebAssembly functions are written in Rust, you will also need a [Rust compiler](https://www.rust-lang.org/tools/install).

For the rest of the demo, fork and clone the [source code repository](https://github.com/yomorun/yomo-wasmedge-tensorflow).

```bash
git clone https://github.com/yomorun/yomo-wasmedge-tensorflow.git
```

## The image classification function

The [image classification function](https://github.com/yomorun/yomo-wasmedge-tensorflow/tree/main/flow/rust_mobilenet_food) to process the YoMo image stream is written in Rust. It utilizes the WasmEdge Tensorflow API to process an input image.

```rust
#[wasmedge_bindgen]
pub fn infer(image_data: Vec<u8>) -> Result<Vec<u8>, String> {
  let start = Instant::now();

  // Load the TFLite model and its meta data (the text label for each recognized object number)
  let model_data: &[u8] = include_bytes!("lite-model_aiy_vision_classifier_food_V1_1.tflite");
  let labels = include_str!("aiy_food_V1_labelmap.txt");

  // Pre-process the image to a format that can be used by this model
  let flat_img = wasmedge_tensorflow_interface::load_jpg_image_to_rgb8(&image_data[..], 192, 192);
  println!("RUST: Loaded image in ... {:?}", start.elapsed());

  // Run the TFLite model using the WasmEdge Tensorflow API
  let mut session = wasmedge_tensorflow_interface::Session::new(&model_data, wasmedge_tensorflow_interface::ModelType::TensorFlowLite);
  session.add_input("input", &flat_img, &[1, 192, 192, 3])
         .run();
  let res_vec: Vec<u8> = session.get_output("MobilenetV1/Predictions/Softmax");

  // Find the object index in res_vec that has the greatest probability
  // Translate the probability into a confidence level
  // Translate the object index into a label from the model meta data food_name
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
  println!("RUST: index {}, prob {}", max_index, max_value);

  let confidence: String;
  if max_value > 200 {
    confidence = "is very likely".to_string();
  } else if max_value > 125 {
    confidence = "is likely".to_string();
  } else {
    confidence = "could be".to_string();
  }

  let ret_str: String;
  if max_value > 50 {
    let mut label_lines = labels.lines();
    for _i in 0..max_index {
      label_lines.next();
    }
    let food_name = label_lines.next().unwrap().to_string();
    ret_str = format!(
      "It {} a <a href='https://www.google.com/search?q={}'>{}</a> in the picture",
      confidence, food_name, food_name
    );
  } else {
    ret_str = "It does not appears to be a food item in the picture.".to_string();
  }

  println!(
    "RUST: Finished post-processing in ... {:?}",
    start.elapsed()
  );
  return Ok(ret_str.as_bytes().to_vec());
}
```

You should add `wasm32-wasi` target to rust to compile this function into WebAssembly bytecode.

```bash
rustup target add wasm32-wasi

cd flow/rust_mobilenet_food
cargo build --target wasm32-wasi --release
# The output WASM will be target/wasm32-wasi/release/rust_mobilenet_food_lib.wasm

# Copy the wasm bytecode file to the flow/ directory
cp target/wasm32-wasi/release/rust_mobilenet_food_lib.wasm ../
```

To release the best performance of WasmEdge, you should enable the AOT mode by compiling the `.wasm` file to the `.so`.

```bash
wasmedgec rust_mobilenet_food_lib.wasm rust_mobilenet_food_lib.so
```

## Integration with YoMo

On the YoMo side, we use the WasmEdge Golang API to start and run WasmEdge VM for the image classification function. The [app.go](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/flow/app.go) file in the source code project is as follows.

```go
package main

import (
  "crypto/sha1"
  "fmt"
  "log"
  "os"
  "sync/atomic"

  "github.com/second-state/WasmEdge-go/wasmedge"
  bindgen "github.com/second-state/wasmedge-bindgen/host/go"
  "github.com/yomorun/yomo"
)

var (
  counter uint64
)

const ImageDataKey = 0x10

func main() {
  // Connect to Zipper service
  sfn := yomo.NewStreamFunction("image-recognition", yomo.WithZipperAddr("localhost:9900"))
  defer sfn.Close()

  // set only monitoring data
  sfn.SetObserveDataID(ImageDataKey)

  // set handler
  sfn.SetHandler(Handler)

  // start
  err := sfn.Connect()
  if err != nil {
    log.Print("❌ Connect to zipper failure: ", err)
    os.Exit(1)
  }

  select {}
}

// Handler process the data in the stream
func Handler(img []byte) (byte, []byte) {
  // Initialize WasmEdge's VM
  vmConf, vm := initVM()
  bg := bindgen.Instantiate(vm)
  defer bg.Release()
  defer vm.Release()
  defer vmConf.Release()

  // recognize the image
  res, err := bg.Execute("infer", img)
  if err == nil {
    fmt.Println("GO: Run bindgen -- infer:", string(res))
  } else {
    fmt.Println("GO: Run bindgen -- infer FAILED")
  }

  // print logs
  hash := genSha1(img)
  log.Printf("✅ received image-%d hash %v, img_size=%d \n", atomic.AddUint64(&counter, 1), hash, len(img))

  return 0x11, nil
}

// genSha1 generate the hash value of the image
func genSha1(buf []byte) string {
  h := sha1.New()
  h.Write(buf)
  return fmt.Sprintf("%x", h.Sum(nil))
}

// initVM initialize WasmEdge's VM
func initVM() (*wasmedge.Configure, *wasmedge.VM) {
  wasmedge.SetLogErrorLevel()
  // Set Tensorflow not to print debug info
  os.Setenv("TF_CPP_MIN_LOG_LEVEL", "3")
  os.Setenv("TF_CPP_MIN_VLOG_LEVEL", "3")

  // Create configure
  vmConf := wasmedge.NewConfigure(wasmedge.WASI)

  // Create VM with configure
  vm := wasmedge.NewVMWithConfig(vmConf)

  // Init WASI
  var wasi = vm.GetImportObject(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],     // The args
    os.Environ(),    // The envs
    []string{".:."}, // The mapping directories
  )

  // Register WasmEdge-tensorflow and WasmEdge-image
  var tfobj = wasmedge.NewTensorflowImportObject()
  var tfliteobj = wasmedge.NewTensorflowLiteImportObject()
  vm.RegisterImport(tfobj)
  vm.RegisterImport(tfliteobj)
  var imgobj = wasmedge.NewImageImportObject()
  vm.RegisterImport(imgobj)

  // Instantiate wasm
  vm.LoadWasmFile("rust_mobilenet_food_lib.so")
  vm.Validate()

  return vmConf, vm
}
```

## In action

Finally, we can start YoMo and see the entire data processing pipeline in action. Start the YoMo CLI application from the project folder. The [yaml file](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/zipper/workflow.yaml) defines port YoMo should listen on and the workflow handler to trigger for incoming data.  Note that the flow name `image-recognition` matches the name in the aforementioned data handler [app.go](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/flow/app.go).

```bash
yomo serve -c ./zipper/workflow.yaml
```

Start the handler function by running the aforementioned [app.go](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/flow/app.go) program.

```bash
cd flow
go run --tags "tensorflow image" app.go
```

[Start a simulated data source](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/source/main.go) by sending a video to YoMo. The video is a series of image frames. The WasmEdge function in [app.go](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/flow/app.go) will be invoked against every image frame in the video.

```bash
# Download a video file
wget -P source 'https://github.com/yomorun/yomo-wasmedge-tensorflow/releases/download/v0.1.0/hot-dog.mp4'

# Stream the video to YoMo
go run ./source/main.go ./source/hot-dog.mp4
```

You can see the output from the WasmEdge handler function in the console. It prints the names of the objects detected in each image frame in the video.

## What's next

In this article, we have seen how to use the WasmEdge Tensorflow API and Golang SDK in YoMo framework to process an image stream in near real-time.

In collaboration with YoMo, we will soon deploy WasmEdge in production in smart factories for a variety of assembly line tasks. WasmEdge is the software runtime for edge computing!
