# Tensorflow

在这一章节，我们将向你展示如何在 Rust 中创建一个用于图像分类的 Tensorflow 推理函数，然后将其嵌入到 Go 应用程序中。项目的源码可以在[这儿](https://github.com/second-state/WasmEdge-go-examples/tree/master/wasmedge-bindgen/go_TfliteFood)查看。

## 将 Rust 函数编译成 WebAssembly

用于图像分类的 Rust 函数在[这儿](https://github.com/second-state/WasmEdge-go-examples/blob/master/wasmedge-bindgen/go_TfliteFood/rust_tflite_food/src/lib.rs)。它使用 [WasmEdge Tensorflow 扩展 API](../../dev/rust/tensorflow.md) 以及 [wasmedge_bindgen](function.md) 来传递所有调用参数。

```rust
#[wasmedge_bindgen]
fn infer(image_data: Vec<u8>) -> Result<Vec<u8>, String> {
  ... ...
  let flat_img = image::imageops::thumbnail(&img, 192, 192);

  let model_data: &[u8] = include_bytes!("lite-model_aiy_vision_classifier_food_V1_1.tflite");
  let labels = include_str!("aiy_food_V1_labelmap.txt");

  let mut session = wasmedge_tensorflow_interface::Session::new(
    model_data,
    wasmedge_tensorflow_interface::ModelType::TensorFlowLite,
  );
  session.add_input("input", &flat_img, &[1, 192, 192, 3]).run();
  let res_vec: Vec<u8> = session.get_output("MobilenetV1/Predictions/Softmax");
  ... ...
}
```

你可以使用标准的 `Cargo` 命令来将其构建成为一个 WebAssembly 函数。

```bash
cd rust_tflite_food
cargo build --target wasm32-wasi --release
cp target/wasm32-wasi/release/rust_tflite_food_lib.wasm ../
cd ../
```

你也可以使用我们的 AOT 编译器 `wasmedgec` 来检测 WebAssembly 文件，以便让其运行的更快。[学习更多](../../start/universal.md)。

```bash
wasmedgec rust_tflite_food_lib.wasm rust_tflite_food_lib.wasm
```

## Go host 应用程序

[Go host 应用程序](https://github.com/second-state/WasmEdge-go-examples/blob/master/wasmedge-bindgen/go_TfliteFood/tflite_food.go)的源码展示了如何用 Tensorflow 来扩展并实例化一个 WasmEdge runtime，以及如何将图片数据传递给 WasmEdge 中的 Rust 函数，以便来执行推理。

```go
func main() {
  // Expected Args[0]: program name (./tflite_food)
  // Expected Args[1]: wasm file (rust_tflite_food_lib.wasm)
  // Expected Args[2]: input image name (food.jpg)

  wasmedge.SetLogErrorLevel()

  // Set Tensorflow not to print debug info
  os.Setenv("TF_CPP_MIN_LOG_LEVEL", "3")
  os.Setenv("TF_CPP_MIN_VLOG_LEVEL", "3")

  var conf = wasmedge.NewConfigure(wasmedge.WASI)
  var vm = wasmedge.NewVMWithConfig(conf)
  var wasi = vm.GetImportModule(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],     // The args
    os.Environ(),    // The envs
    []string{".:."}, // The mapping preopens
  )

  // Register WasmEdge-tensorflow
  var tfmod = wasmedge.NewTensorflowModule()
  var tflitemod = wasmedge.NewTensorflowLiteModule()
  vm.RegisterModule(tfmod)
  vm.RegisterModule(tflitemod)

  // Load and validate the wasm
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
  tfmod.Release()
  tflitemod.Release()
}
```

## 构建和运行

> 你必须要在你的机器上安装带有 tensorflow 扩展的 WasmEdge。详情可查看[安装指南](../../start/install.md)。

如下命令用 WasmEdge Go SDK 和它的 tensorflow 扩展，构建了 Go host 应用程序。

```bash
go build -tags tensorflow
```

现在你就可以运行 Go 应用程序了。它在 WasmEdge 中调用 WebAssembly 函数，以对输入图片进行推理。

```bash
./tflite_food rust_tflite_food_lib.wasm food.jpg
```

结果如下。

```bash
Go: Args: [./tflite_food rust_tflite_food_lib.wasm food.jpg]
It is very likely a Hot dog in the picture
```
