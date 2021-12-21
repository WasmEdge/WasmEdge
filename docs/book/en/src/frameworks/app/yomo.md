# YoMo



[YoMo](https://yomo.run/) is a programming framework enabling developers to build a distributed cloud system (Geo-Distributed Cloud System). YoMo's communication layer is made on top of the QUIC protocol, which brings high-speed data transmission. In addition, it has a built-in Streaming Serverless "streaming function", which significantly improves the development experience of distributed cloud systems. The distributed cloud system built by YoMo provides an ultra-high-speed communication mechanism between near-field computing power and terminals. It has a wide range of use cases in Metaverse, VR/AR, IoT, etc.


> YoMo is written in the Go language. For streaming Serverless, Golang plugins and shared libraries are used to load users' code dynamically, which also have certain limitations for developers. Coupled with Serverless architecture's rigid demand for isolation, this makes WebAssembly an excellent choice for running user-defined functions.



For example, in the process of real-time AI inference in AR/VR devices or smart factories, the camera sends real-time unstructured data to the computing node in the near-field MEC (multi-access edge computing) device through YoMo.  YoMo sends the AI computing result to the end device in real-time when the AI inference is completed. Thus, the hosted AI inference function will be automatically executed.  


However, a challenge for YoMo is to incorporate and manage handler functions written by multiple outside developers in an edge computing node. It requires runtime isolation for those functions without sacrificing performance. Traditional software container solutions, such as Docker, are not up to the task. They are too heavy and slow to handle real-time tasks.

WebAssembly provides a lightweight and high-performance software container. It is ideally suited as a runtime for YoMo’s data processing handler functions.

In this article, we will show you how to create a Rust function for Tensorflow-based image classification, compile it into WebAssembly, and then use YoMo to run it as a stream data handler. We use [WasmEdge](https://wasmedge.org/) as our WebAssembly runtime because it offers the highest performance and flexibility compared with other WebAssembly runtimes. It is the only WebAssembly VM that reliably supports Tensorflow. YoMo manages WasmEdge VM instances and the contained WebAssembly bytecode apps through [WasmEdge’s Golang API](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/).

> Source code: https://github.com/yomorun/yomo-wasmedge-tensorflow

Checkout [the WasmEdge image classification function in action in YoMo](https://www.youtube.com/watch?v=E0ltsn6cLIU)


## Prerequisite

Obviously, you will need to have [Golang installed](https://golang.org/doc/install), but I will assume you already did.


> Golang version should be newer than 1.15 for our example to work.


You also need to install the YoMo CLI application. It orchestrates and coordinates data streaming and handler function invocations. 


```
$ go install github.com/yomorun/cli/yomo@latest
$ yomo version
YoMo CLI version: v0.0.5
```


Next, please install the WasmEdge and its Tensorflow shared libraries. [WasmEdge](https://wasmedge.org/) is a leading WebAssembly runtime hosted by the CNCF. We will use it to embed and run WebAssembly programs from YoMo.


```
# Install WasmEdge
$ wget https://github.com/second-state/WasmEdge-go/releases/download/v0.8.1/install_wasmedge.sh
$ chmod +x ./install_wasmedge.sh
$ sudo ./install_wasmedge.sh /usr/local

# Install WasmEdge Tensorflow extension
$ wget https://github.com/second-state/WasmEdge-go/releases/download/v0.8.1/install_wasmedge_tensorflow_deps.sh
$ wget https://github.com/second-state/WasmEdge-go/releases/download/v0.8.1/install_wasmedge_tensorflow.sh
$ chmod +x ./install_wasmedge_tensorflow_deps.sh
$ chmod +x ./install_wasmedge_tensorflow.sh
$ sudo ./install_wasmedge_tensorflow_deps.sh /usr/local
$ sudo ./install_wasmedge_tensorflow.sh /usr/local

# Install WasmEdge Images extension
$ wget https://github.com/second-state/WasmEdge-go/releases/download/v0.8.1/install_wasmedge_image_deps.sh
$ wget https://github.com/second-state/WasmEdge-go/releases/download/v0.8.1/install_wasmedge_image.sh
$ chmod +x ./install_wasmedge_image_deps.sh
$ chmod +x ./install_wasmedge_image.sh
$ sudo ./install_wasmedge_image_deps.sh /usr/local
$ sudo ./install_wasmedge_image.sh /usr/local
```


Finally, since our demo WebAssembly functions are written in Rust, you will also need a [Rust compiler and the rustwasmc toolchain](https://www.secondstate.io/articles/rustwasmc/).

For the rest of the demo, fork and clone the [source code repository](https://github.com/yomorun/yomo-wasmedge-tensorflow).


```
$ git clone https://github.com/yomorun/yomo-wasmedge-tensorflow.git
```

## The image classification function

The [image classification function](https://github.com/yomorun/yomo-wasmedge-tensorflow/tree/main/flow/rust_mobilenet_food) to process the YoMo image stream is written in Rust. It utilizes the WasmEdge Tensorflow API to process an input image.


```
#[wasm_bindgen]
pub fn infer(image_data: &[u8]) -> String {
    // Load the TFLite model and its meta data (the text label for each recognized object number)
    let model_data: &[u8] = include_bytes!("lite-model_aiy_vision_classifier_food_V1_1.tflite");
    let labels = include_str!("aiy_food_V1_labelmap.txt");

    // Pre-process the image to a format that can be used by this model
    let flat_img = wasmedge_tensorflow_interface::load_jpg_image_to_rgb8(image_data, 192, 192);
    
    // Run the TFLite model using the WasmEdge Tensorflow API
    let mut session = wasmedge_tensorflow_interface::Session::new(&model_data, wasmedge_tensorflow_interface::ModelType::TensorFlowLite);
    session.add_input("input", &flat_img, &[1, 192, 192, 3])
           .run();
    let res_vec: Vec<u8> = session.get_output("MobilenetV1/Predictions/Softmax");

    // Find the object index in res_vec that has the greatest probability
    // Translate the probability into a confidence level
    // Translate the object index into a label from the model meta data food_name
    
    ret_str = format!(
        "It {} a <a href='https://www.google.com/search?q={}'>{}</a> in the picture",
        confidence, food_name, food_name
    );
    return ret_str;
}
```

You can use the [rustwasmc](https://www.secondstate.io/articles/rustwasmc/) tool to compile this function into WebAssembly bytecode. 

> At this time, we require Rust compiler version 1.50 or less in order for WebAssembly functions to work with WasmEdge’s Golang API. We will [catch up to the latest Rust](https://github.com/WasmEdge/WasmEdge/issues/264) compiler version once the Interface Types spec is finalized and supported. 

```
$ rustup default 1.50.0

$ cd flow/rust_mobilenet_food
$ rustwasmc  build `--enable-ext`
# The output WASM will be pkg/rust_mobilenet_food_lib_bg.wasm.

# Copy the wasm bytecode file to the flow/ directory
$ cp pkg/rust_mobilenet_food_lib_bg.wasm ../
```

## Integration with YoMo

On the YoMo side, we use the WasmEdge Golang API to start and run WasmEdge VM for the image classification function. The [app.go](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/flow/app.go) file in the source code project is as follows. 

```
package main

... ...

var (
    vm      *wasmedge.VM
    vmConf  *wasmedge.Configure
    counter uint64
)

func main() {
    // Initialize WasmEdge's VM
    initVM()
    defer vm.Delete()
    defer vmConf.Delete()

    // Connect to Zipper service
    cli, err := client.NewServerless("image-recognition").Connect("localhost", 9000)
    if err != nil {
        log.Print("❌ Connect to zipper failure: ", err)
        return
    }

    defer cli.Close()
    cli.Pipe(Handler)
}

// Handler process the data in the stream
func Handler(rxStream rx.RxStream) rx.RxStream {
    stream := rxStream.
        Subscribe(ImageDataKey).
        OnObserve(decode).
        Encode(0x11)
        
    return stream
}

// decode Decode and perform image recognition
var decode = func(v []byte) (interface{}, error) {
    // get image binary
    p, _, _, err := y3.DecodePrimitivePacket(v)
    if err != nil {
        return nil, err
    }
    img := p.ToBytes()

    // recognize the image
    res, err := vm.ExecuteBindgen("infer", wasmedge.Bindgen_return_array, img)
    
    return hash, nil
}

... ...

// initVM initialize WasmEdge's VM
func initVM() {
    wasmedge.SetLogErrorLevel()
    vmConf = wasmedge.NewConfigure(wasmedge.WASI)
    vm = wasmedge.NewVMWithConfig(vmConf)

    var wasi = vm.GetImportObject(wasmedge.WASI)
    wasi.InitWasi(
        os.Args[1:],     /// The args
        os.Environ(),    /// The envs
        []string{".:."}, /// The mapping directories
        []string{},      /// The preopens will be empty
    )

    /// Register WasmEdge-tensorflow and WasmEdge-image
    var tfobj = wasmedge.NewTensorflowImportObject()
    var tfliteobj = wasmedge.NewTensorflowLiteImportObject()
    vm.RegisterImport(tfobj)
    vm.RegisterImport(tfliteobj)
    var imgobj = wasmedge.NewImageImportObject()
    vm.RegisterImport(imgobj)

    /// Instantiate wasm
    vm.LoadWasmFile("rust_mobilenet_food_lib_bg.wasm")
    vm.Validate()
    vm.Instantiate()
}
```

## In action

Finally, we can start YoMo and see the entire data processing pipeline in action. Start the YoMo CLI application from the project folder. The [yaml file](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/zipper/workflow.yaml) defines port YoMo should listen on and the workflow handler to trigger for incoming data.  Note that the flow name `image-recognition` matches the name in the aforementioned data handler [app.go](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/flow/app.go).

```
$ yomo serve -c ./zipper/workflow.yaml
```

Start the handler function by running the aforementioned [app.go](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/flow/app.go) program. 

```
$ cd flow
$ go run --tags "tensorflow image" app.go
```

[Start a simulated data source](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/source/main.go) by sending a video to YoMo. The video is a series of image frames. The WasmEdge function in [app.go](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/flow/app.go) will be invoked against every image frame in the video.

```
# Download a video file
$ wget -P source 'https://github.com/yomorun/yomo-wasmedge-tensorflow/releases/download/v0.1.0/hot-dog.mp4'

# Stream the video to YoMo
$ go run ./source/main.go ./source/hot-dog.mp4
```

You can see the output from the WasmEdge handler function in the console. It prints the names of the objects detected in each image frame in the video. 

## What's next

In this article, we have seen how to use the WasmEdge Tensorflow API and Golang SDK in YoMo framework to process an image stream in near real-time.

In collaboration with YoMo, we will soon deploy WasmEdge in production in smart factories for a variety of assembly line tasks. WasmEdge is the software runtime for edge computing!
