# Tensorflow

In this section, we will show you how to create a Tensorflow inference
function in Rust for image classification, and then embed it into 
a Go application. The project source code is [available here](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_Mobilenet).

## Rust function compiled into WebAssembly

The Rust function for image classification is [available here](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_Mobilenet/rust_mobilenet/src/lib.rs).
It utilizes the [WasmEdge Tensorflow extension API](../../dev/rust/tensorflow.md).
You can use the standard `Cargo` command to build it into a WebAssembly function.

```bash
$ cd go_Mobilenet/rust_mobilenet
$ cargo build --target wasm32-wasi --release
$ cp target/wasm32-wasi/release/rust_mobilenet.wasm ../
```

You can use our AOT compiler `wasmedgec` to instrument the WebAssembly file to make 
it run much faster. [Learn more](../../start/universal.md).

```bash
$ wasmedgec rust_mobilenet.wasm rust_mobilenet.wasm
```

## Go host app

The [Go host app](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_Mobilenet/mobilenet.go) source code shows how to instantiate a WasmEdge runtime with
the Tensorflow extension, and how to pass the image data to the Rust function
in WasmEdge to run the inference.

```go
```

## Build and run

> You must have WasmEdge with its tensorflow extension installed on your machine. [Checkout the install guide](../../start/install.md) for details.

The following command builds the Go host application
with the WasmEdge Go SDK and its tensorflow extension.

```bash
$ go get github.com/second-state/WasmEdge-go/wasmedge
$ go build --tags tensorflow
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
GO: Run -- infer: ["military uniform","medium"]
```


