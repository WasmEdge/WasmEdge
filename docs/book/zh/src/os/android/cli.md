# 适用于 Android 的 WasmEdge CLI 工具

在本节中，我们将向您展示如何在 Android 设备上使用 WasmEdge CLI 工具。 我们将展示一个完整的 WasmEdge 演示，以在 Android 设备上执行图像分类任务（基于 Tensorflow 的 AI 推理）。

## 在 Android 环境下安装 WasmEdge-TensorFlow-Tools

本章节使用 WasmEdge-TensorFlow-Tools 的 pre-release 版本软件包，配合 Android 版本的 TensorFlow-Lite 动态链接库在 Android 平台上运行并测试。

### 环境准备

#### Android 开发者选项

目前 WasmEdge 仅支持 Android 平台的 arm64-v8a 架构，开发者需要准备 arm64-v8a 架构的 Android 模拟器或一台已经 [启用开发者选项和 USB 调试](https://developer.android.com/studio/debug/dev-options) 的 Android 设备便于测试，最低系统版本为 Android 6.0。

#### Android 命令行工具

Ubuntu 环境下，开发者可以通过 `apt-get` 获取 Android 平台调试工具 `adb` 。通过 `adb shell` 指令，开发者可以进入设备并使用命令行操作 Android 操作系统。

```bash
$ sudo apt-get install adb
$ adb devices
* daemon not running; starting now at tcp:5037
* daemon started successfully
List of devices attached
c657c643 device
$ adb shell
sirius:/ $
```

### 获取 WasmEdge-TensorFlow-Tools 软件包

在 Ubuntu 宿主机上使用以下指令来获取 WasmEdge-TensorFlow-Tools 的 pre-release 版本软件包。

```bash
$ wget https://github.com/second-state/WasmEdge-tensorflow-tools/releases/download/0.11.0/WasmEdge-tensorflow-tools-0.11.0-android_aarch64.tar.gz
$ mkdir WasmEdge-tensorflow-tools && tar zxvf WasmEdge-tensorflow-tools-0.11.0-android_aarch64.tar.gz -C WasmEdge-tensorflow-tools
show-tflite-tensor
wasmedge-tensorflow-lite
```

### 获取 Android 版本的 TensorFlow-Lite 动态链接库

在 WasmEdge-TensorFlow-deps 中已经为用户提供了 Android 版本的 TensorFlow-Lite 动态链接库，用户可以直接下载并使用。

```bash
$ wget https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/0.11.0/WasmEdge-tensorflow-deps-TFLite-0.11.0-android_aarch64.tar.gz
$ tar zxvf WasmEdge-tensorflow-deps-TFLite-0.11.0-android_aarch64.tar.gz -C WasmEdge-tensorflow-tools
libtensorflowlite_c.so
```

然后使用 `adb` 工具将 WasmEdge-TensorFlow 软件包推送到 Android 设备上。

```bash
adb push WasmEdge-tensorflow-tools /data/local/tmp
```

## 测试

### 获取示例应用程序

在此示例中，我们将演示一个标准的 [WasmEdge Tensorflow-Lite 示例应用程序](https://github.com/second-state/wasm-learning/tree/master/rust/birds_v1)。它可以从鸟类的 JPG 或 PNG 图片中识别和分类鸟类类型。源代码的解释可以[在这里找到](https://wasmedge.org/book/en/dev/rust/tensorflow.html)。

```bash
git clone https://github.com/second-state/wasm-learning.git
cd wasm-learning/rust/birds_v1
```

使用 `cargo` 命令从 Rust 源代码构建 wasm 源文件，生成的 wasm 文件位于 `target/wasm32-wasi/release/birds_v1.wasm` 。

```bash
rustup target add wasm32-wasi
cargo build --release --target=wasm32-wasi
```

使用 `adb` 命令将测试需要的 wasm 源文件， tensorflow 模型以及 jpg 图片文件推送到 Android 设备上。

```bash
adb push target/wasm32-wasi/release/birds_v1.wasm /data/local/tmp/WasmEdge-tensorflow-tools
adb push lite-model_aiy_vision_classifier_birds_V1_3.tflite /data/local/tmp/WasmEdge-tensorflow-tools
adb push bird.jpg /data/local/tmp/WasmEdge-tensorflow-tools
```

### 运行 WasmEdge-TensorFlow-Tools

在 Ubuntu 命令行输入 `adb shell` 进入 Android 设备，查看 `/data/local/tmp/WasmEdge-tensorflow-tools` 文件夹下的工具及测试文件是否齐全。

```bash
$ adb shell
sirius:/ $ cd /data/local/tmp/WasmEdge-tensorflow-tools
sirius:/data/local/tmp/WasmEdge-tensorflow-tools $ ls
bird.jpg               lite-model_aiy_vision_classifier_birds_V1_3.tflite
birds_v1.wasm          show-tflite-tensor
libtensorflowlite_c.so wasmedge-tensorflow-lite
```

链接 TensorFlow-Lite 动态依赖库，并使用 `show-tflite-tensor` 检查 TensorFlow-Lite 的可用性。

```bash
sirius:/data/local/tmp/WasmEdge-tensorflow-tools $ export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
sirius:/data/local/tmp/WasmEdge-tensorflow-tools $ chmod 777 show-tflite-tensor
sirius:/data/local/tmp/WasmEdge-tensorflow-tools $ ./show-tflite-tensor lite-model_aiy_vision_classifier_birds_V1_3.tflite
INFO: Initialized TensorFlow Lite runtime.
Input tensor nums: 1
    Input tensor name: module/hub_input/images_uint8
        dimensions: [1 , 224 , 224 , 3]
        data type: UInt8
        tensor byte size: 150528
Output tensor nums: 1
    Output tensor name: module/prediction
        dimensions: [1 , 965]
        data type: UInt8
        tensor byte size: 965
```

使用 `wasmedge-tensorflow-lite` 中扩展的 WasmEdge Runtime 在 Android 设备上执行编译好的 Wasm 程序。 它加载 Tensorflow Lite 模型和鸟类图像，并输出鸟类分类及其置信度。

```bash
sirius:/data/local/tmp/WasmEdge-tensorflow-tools $ chmod 777 wasmedge-tensorflow-lite
sirius:/data/local/tmp/WasmEdge-tensorflow-tools $ ./wasmedge-tensorflow-lite --dir .:. birds_v1.wasm lite-model_aiy_vision_classifier_birds_V1_3.tflite bird.jpg
INFO: Initialized TensorFlow Lite runtime.
166 : 0.84705883
```

结果显示鸟的类型在 [标签文件的第166行](https://github.com/second-state/wasm-learning/blob/master/rust/birds_v1/aiy_birds_V1_labels.txt#L166)（Sicalis flaveola)，置信水平为 84%。
