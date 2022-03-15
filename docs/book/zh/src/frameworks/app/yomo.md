# YoMo

[YoMo](https://yomo.run/) 是一个让开发者能够构建分布式云系统（地理分布式云系统）的编程框架。YoMo 的通信层是建立在 QUIC 协议之上，QUIC 协议带来了高速的数据传输。另外，YoMo 内置了流式 Serverless 的 “流式函数”，这可以显著提升分布式云系统的开发体验。YoMo 构建的分布式云系统提供了近场算力与终端之间的超高速通信机制， 使得它在元宇宙、VR/AR、IoT 等领域有着广泛的应用。

> YoMo 是使用 Go 语言编写的。对于流式 Serverless，Go 语言的插件和共享库可以动态加载用户的代码，这也对开发者带来了一些限制。加上 Serverless 架构对隔离性的刚性需求，这使得 WebAssembly 成为运行用户定义函数的最佳选择。

举个例子， 在 AR/VR 设备或智能工厂的 AI 推理过程中，摄像头通过  YoMo 向近场 MEC（对接入边缘计算）设备中的计算节点发送实时非结构化数据。当 AI 推理完成之后，YoMo 将 AI 计算结果实时发送到终端设备。因此，托管的 AI 推理功能实现了自动化。

然而，YoMo 面临的挑战是在边缘计算节点中整合和管理多个外部开发者开发的 handler 函数。这需要在不影响性能的情况下对这些函数进行 runtime 隔离。传统的软件容器解决方案，比如 Docker，难以胜任。因为传统的软件容器太重太慢，无法处理实时任务。

WebAssembly 提供了一个轻量级和高性能的软件容器。 它非常适合作为 YoMo 数据处理 handler 函数的 runtime。

在本文中，我们将向你展示如何创建一个基于 Tensorflow 图像分类的 Rust 函数，将其编译为 WebAssembly，然后使用 YoMo 作为流数据 handler 运行它。我们使用 [WasmEdge](https://wasmedge.org/) 作为我们 WebAssembly runtime，因为与其他 WebAssembly runtime 相比，WasmEdge 提供了最高的性能与灵活性。是唯一能稳定支持 Tensorflow 的 WebAssembly 虚拟机。[YoMo 通过 WasmEdge 的 Go SDK](https://github.com/WasmEdge/WasmEdge/blob/master/docs/book/en/src/embed/go.md) 来管理 WasmEdge 虚拟机实例和 contained 容器化 WebAssembly 字节码应用。

> 源码: <https://github.com/yomorun/yomo-wasmedge-tensorflow>

详情可查看 [YoMo 中的 WasmEdge 图像分类函数实战](https://www.youtube.com/watch?v=E0ltsn6cLIU)

## 安装前提

显然，你需要 [安装 Go 语言](https://golang.org/doc/install)，但我觉得你肯定安装好了。

> 为了我们的示例代码能正常工作， Go 语言版本应不低于 1.15 。

你还要安装 YoMo CLI  应用程序，其负责数据流和 handler 函数调用的编排和协调。

```bash
$ go install github.com/yomorun/cli/yomo@latest
$ yomo version
YoMo CLI version: v0.1.3
```

接下来，请安装 WasmEdge 和其 Tensorflow 共享库。[WasmEdge](https://wasmedge.org/) 是由云原生基金会（CNCF）托管的行业领先的 WebAssembly runtime。我们将使用它来嵌入和运行来自 YoMo 的 WebAssembly 程序。

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash
```

最后，因为我们的 demo WebAssembly 函数是 Rust 编写的，因此你还需要一个  [Rust 编译器](https://www.rust-lang.org/tools/install)。

至于 demo 的其余部分，fork 并克隆 [源码库](https://github.com/yomorun/yomo-wasmedge-tensorflow) 就行。

```bash
git clone https://github.com/yomorun/yomo-wasmedge-tensorflow.git
```

## 图像分类函数

处理 YoMo 图像流的 [图像分类函数](https://github.com/yomorun/yomo-wasmedge-tensorflow/tree/main/flow/rust_mobilenet_food) 是 Rust 写的。它利用 WasmEdge Tensorflow API 来处理输入图像。

```rust
#[wasmedge_bindgen]
pub fn infer(image_data: Vec<u8>) -> Result<Vec<u8>, String> {
  let start = Instant::now();

  // 导入 TFLite 模型和其元数据（每个被识别对象编号的文本标签）
  let model_data: &[u8] = include_bytes!("lite-model_aiy_vision_classifier_food_V1_1.tflite");
  let labels = include_str!("aiy_food_V1_labelmap.txt");

  // 将图像进行预处理成该模型可以使用的规范模式
  let flat_img = wasmedge_tensorflow_interface::load_jpg_image_to_rgb8(&image_data[..], 192, 192);
  println!("RUST: Loaded image in ... {:?}", start.elapsed());

  // 使用 WasmEdge TensorFlow API 运行 TFLite 模型
  let mut session = wasmedge_tensorflow_interface::Session::new(&model_data, wasmedge_tensorflow_interface::ModelType::TensorFlowLite);
  session.add_input("input", &flat_img, &[1, 192, 192, 3])
         .run();
  let res_vec: Vec<u8> = session.get_output("MobilenetV1/Predictions/Softmax");

  // 在 res_vec 中找到概率最大的对象索引
  // 将概率转换为置信水平
  // 将模型元数据 food_name 中的对象索引转换为标签
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

你应该把 `wasm32-wasi` 目标添加到 rust，用以将此函数编译为 WebAssembly 字节码。

```bash
rustup target add wasm32-wasi

cd flow/rust_mobilenet_food
cargo build --target wasm32-wasi --release
# 编译生成的 WASM 字节码文件就是 target/wasm32-wasi/release/rust_mobilenet_food_lib.wasm

# 把 wasm 字节码文件拷贝到 flow/ 目录
cp target/wasm32-wasi/release/rust_mobilenet_food_lib.wasm ../
```

为了释放 WasmEdge 的最佳性能，你应该使用 `wasmedgec` 将 `.wasm` 文件编译为原生二进制文件来启用 AOT 模式。

```bash
wasmedgec rust_mobilenet_food_lib.wasm rust_mobilenet_food_lib.wasm
```

## 与 YoMo 集成

在 YoMo 方面，我们使用 WasmEdge Go 语言 API 来启动和运行 WasmEdge 虚拟机来实现图像分类功能。下面是源码项目中的 [app.go](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/flow/app.go) 文件。

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
  // 连接 Zipper 服务
  sfn := yomo.NewStreamFunction("image-recognition", yomo.WithZipperAddr("localhost:9900"))
  defer sfn.Close()

  // 设置监控数据
  sfn.SetObserveDataID(ImageDataKey)

  // 设置 handler
  sfn.SetHandler(Handler)

  // 启动
  err := sfn.Connect()
  if err != nil {
    log.Print("❌ Connect to zipper failure: ", err)
    os.Exit(1)
  }

  select {}
}

// Handler 处理流式数据
func Handler(img []byte) (byte, []byte) {
  // 初始化 WasmEdge 虚拟机
  vmConf, vm := initVM()
  bg := bindgen.Instantiate(vm)
  defer bg.Release()
  defer vm.Release()
  defer vmConf.Release()

  // 识别图像
  res, err := bg.Execute("infer", img)
  if err == nil {
    fmt.Println("GO: Run bindgen -- infer:", string(res))
  } else {
    fmt.Println("GO: Run bindgen -- infer FAILED")
  }

  // 打印 log 
  hash := genSha1(img)
  log.Printf("✅ received image-%d hash %v, img_size=%d \n", atomic.AddUint64(&counter, 1), hash, len(img))

  return 0x11, nil
}

// genSha1 生成图像的哈希值
func genSha1(buf []byte) string {
  h := sha1.New()
  h.Write(buf)
  return fmt.Sprintf("%x", h.Sum(nil))
}

// initVM 初始化 WasmEdge 虚拟机
func initVM() (*wasmedge.Configure, *wasmedge.VM) {
  wasmedge.SetLogErrorLevel()
  // 设置 Tensorflow 不要打印调试信息
  os.Setenv("TF_CPP_MIN_LOG_LEVEL", "3")
  os.Setenv("TF_CPP_MIN_VLOG_LEVEL", "3")

  // 创建配置信息
  vmConf := wasmedge.NewConfigure(wasmedge.WASI)

  // 根据配置信息创建虚拟机
  vm := wasmedge.NewVMWithConfig(vmConf)

  // 初始化 WASI
  var wasi = vm.GetImportObject(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],     // The args
    os.Environ(),    // The envs
    []string{".:."}, // The mapping directories
  )

  // 注册 WasmEdge-tensorflow 和 WasmEdge-image
  var tfobj = wasmedge.NewTensorflowImportObject()
  var tfliteobj = wasmedge.NewTensorflowLiteImportObject()
  vm.RegisterImport(tfobj)
  vm.RegisterImport(tfliteobj)
  var imgobj = wasmedge.NewImageImportObject()
  vm.RegisterImport(imgobj)

  // 实例化 wasm
  vm.LoadWasmFile("rust_mobilenet_food_lib.so")
  vm.Validate()

  return vmConf, vm
}
```

## 实战

最后，我们启动 YoMo 并可以看到整个数据处理流水线的运行情况。从项目文件夹中启动 YoMo CLI 应用程序。[yaml 文件](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/zipper/workflow.yaml) 定义了 YoMo 应该监听的端口和用来触发传入数据的工作流 handler。注意该 flow 名称 `image-recognition` 与上面提到的数据 handler 程序 [app.go](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/flow/app.go) 中一致。

```bash
yomo serve -c ./zipper/workflow.yaml
```

通过运行上面提到的 [app.go](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/flow/app.go) 程序来启动 handler 函数。

```bash
cd flow
go run --tags "tensorflow image" app.go
```

通过向 YoMo 发送视频来[启动模拟数据源](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/source/main.go)。视频都是一系列图像帧。[app.go](https://github.com/yomorun/yomo-wasmedge-tensorflow/blob/main/flow/app.go) 中的 WasmEdge 函数将针对视频中的图像逐帧调用。

```bash
# 下载视频文件
wget -P source 'https://github.com/yomorun/yomo-wasmedge-tensorflow/releases/download/v0.1.0/hot-dog.mp4'

# 将视频流传给 YoMo
go run ./source/main.go ./source/hot-dog.mp4
```

你可以在操作台中看到来自 WasmEdge handler 函数的输出。它打印了视频中每个图像帧检测到的物体名称。

## 更多内容

在本文中，我们了解了如何使用  WasmEdge Tensorflow API 和 YoMo 框架的 Go 语言 SDK 来近乎实时地处理图像流。

与 YoMo 合作，我们可以快速地在智能工厂的生产中部署 WasmEdge，以完成各种装配线任务。 WasmEdge 就是边缘计算软件 runtime 的事实标准！
