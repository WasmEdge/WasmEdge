# Tensorflow

In this section, we will show you how to create a Tensorflow inference
function in Rust for image classification, and then embed it into 
a Go application. The project source code is [available here](https://github.com/second-state/WasmEdge-go-examples/tree/master/wasmedge-bindgen/go_Mobilenet).

## Rust function compiled into WebAssembly

The Rust function for image classification is [available here](https://github.com/second-state/WasmEdge-go-examples/blob/master/wasmedge-bindgen/go_Mobilenet/rust_mobilenet/src/lib.rs).
It utilizes the [WasmEdge Tensorflow extension API](../../dev/rust/tensorflow.md) as well as the [wasmedge_bindgen](function.md) for passing call parameters.

```rust
#[wasmedge_bindgen]
fn infer(image_data: Vec<u8>) -> Result<Vec<u8>, String> {
    ... ...
    let mut session = wasmedge_tensorflow_interface::Session::new(
        model_data,
        wasmedge_tensorflow_interface::ModelType::TensorFlow,
    );
    session
        .add_input("input", &flat_img, &[1, 224, 224, 3])
        .add_output("MobilenetV2/Predictions/Softmax")
        .run();
    let res_vec: Vec<f32> = session.get_output("MobilenetV2/Predictions/Softmax");
    ... ...
}
```

You can use the standard `Cargo` command to build it into a WebAssembly function.

```bash
$ cd rust_mobilenet
$ cargo build --target wasm32-wasi --release
$ cp target/wasm32-wasi/release/rust_mobilenet.wasm ../
$ cd ../
```

You can use our AOT compiler `wasmedgec` to instrument the WebAssembly file to make 
it run much faster. [Learn more](../../start/universal.md).

```bash
$ wasmedgec rust_mobilenet.wasm rust_mobilenet.wasm
```

## Go host app

The [Go host app](https://github.com/second-state/WasmEdge-go-examples/blob/master/wasmedge-bindgen/go_Mobilenet/mobilenet.go) source code shows how to instantiate a WasmEdge runtime with
the Tensorflow extension, and how to pass the image data to the Rust function
in WasmEdge to run the inference.

```go
func main() {
	/// Expected Args[0]: program name (./mobilenet)
	/// Expected Args[1]: wasm or wasm-so file (rust_mobilenet.wasm)
	/// Expected Args[2]: input image name (solvay.jpg)

	wasmedge.SetLogErrorLevel()

	/// Set Tensorflow not to print debug info
	os.Setenv("TF_CPP_MIN_LOG_LEVEL", "3")
	os.Setenv("TF_CPP_MIN_VLOG_LEVEL", "3")

	var conf = wasmedge.NewConfigure(wasmedge.WASI)
	var vm = wasmedge.NewVMWithConfig(conf)
	var wasi = vm.GetImportObject(wasmedge.WASI)
	wasi.InitWasi(
		os.Args[1:],     /// The args
		os.Environ(),    /// The envs
		[]string{".:."}, /// The mapping preopens
	)

	/// Register WasmEdge-tensorflow
	var tfobj = wasmedge.NewTensorflowImportObject()
	var tfliteobj = wasmedge.NewTensorflowLiteImportObject()
	vm.RegisterImport(tfobj)
	vm.RegisterImport(tfliteobj)

	/// Load and validate the wasm
	vm.LoadWasmFile(os.Args[1])
	vm.Validate()

	// Instantiate the bindgen and vm
	bg := bindgen.Instantiate(vm)

	img, _ := ioutil.ReadFile(os.Args[2])
	if res, err := bg.Execute("infer", img); err != nil {
		fmt.Println(err)
	} else {
		fmt.Println(string(res))
	}

	bg.Release()
	vm.Release()
	conf.Release()
	tfobj.Release()
	tfliteobj.Release()
}
```

## Build and run

> You must have WasmEdge with its tensorflow extension installed on your machine. [Checkout the install guide](../../start/install.md) for details.

The following command builds the Go host application
with the WasmEdge Go SDK and its tensorflow extension.

```bash
$ go build -tags tensorflow
```

Now you can run the Go application. It calls the WebAssembly function in WasmEdge
to run inference on the input image.

```bash
$ ./mobilenet rust_mobilenet.wasm grace_hopper.jpg
```

The results are as follows.

```bash
Go: Args: [./mobilenet rust_mobilenet.wasm grace_hopper.jpg]
RUST: Loaded image in ... 16.522151ms
RUST: Resized image in ... 19.440301ms
RUST: Parsed output in ... 285.83336ms
RUST: index 653, prob 0.43212935
RUST: Finished post-processing in ... 285.995153ms
["military uniform","medium"]
```


