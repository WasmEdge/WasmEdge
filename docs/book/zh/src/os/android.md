# Android

在这一章，我们会演示怎么在 Android 上运行全套的 WasmEdge Runtime -- 包括 Tensorflow AI 推理，图像处理，等扩展。如果您需要自己为 Android 编译一个 WasmEdge Runtime，可以[参考这个文档](../extend/build_for_android.md)。

## 在 Android 环境下安装 WasmEdge-TensorFlow-Tools

本章节使用 WasmEdge-TensorFlow-Tools 的 pre-release 版本软件包，配合 Android 版本的 TensorFlow-Lite 动态链接库在 Android 平台上运行并测试。

### 环境准备

#### Android 

目前 WasmEdge 仅支持 Android 平台的 arm64-v8a 架构，开发者需要准备 arm64-v8a 架构的 Android 模拟器或一台已经 [启用开发者选项和 USB 调试](https://developer.android.com/studio/debug/dev-options) 的 Android 设备便于测试，最低系统版本为 Android 6.0。

#### Android 命令行工具

Ubuntu 环境下，开发者可以通过 `apt-get` 获取 Android 平台调试工具 `adb` 。通过 `adb shell` 指令，开发者可以进入设备并使用命令行操作 Android 操作系统。

```
$ sudo apt-get install adb
$ adb devices
* daemon not running; starting now at tcp:5037
* daemon started successfully
List of devices attached
c657c643	device
$ adb shell
sirius:/ $
```

### 获取 WasmEdge-TensorFlow-Tools 软件包

获取 WasmEdge-TensorFlow-Tools 的 pre-release 版本软件包。

```
$ wget https://github.com/second-state/WasmEdge-tensorflow-tools/releases/download/0.9.1-beta.2/WasmEdge-tensorflow-tools-0.9.1-beta.2-android_aarch64.tar.gz
$ mkdir WasmEdge-tensorflow-tools && tar zxvf WasmEdge-tensorflow-tools-0.9.1-beta.2-android_aarch64.tar.gz -C WasmEdge-tensorflow-tools
show-tflite-tensor
wasmedge-tensorflow-lite
```

### 获取 Android 版本的 TensorFlow-Lite 动态链接库

在 WasmEdge-TensorFlow-deps 中已经为用户提供了 Android 版本的 TensorFlow-Lite 动态链接库，用户可以直接下载并使用。

```
$ wget https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/0.9.1-beta.2/WasmEdge-tensorflow-deps-TFLite-0.9.1-beta.2-android_aarch64.tar.gz
$ tar zxvf WasmEdge-tensorflow-deps-TFLite-0.9.1-beta.2-android_aarch64.tar.gz -C WasmEdge-tensorflow-tools
._libtensorflowlite_c.so
libtensorflowlite_c.so
```

将 WasmEdge-TensorFlow 的工具及其依赖一起推送到 Android 设备上。

```
$ adb push WasmEdge-tensorflow-tools /data/local/tmp
```

## 测试

### 获取测试样例

[wasm-learning](https://github.com/second-state/wasm-learning.git) 中为用户提供了 WasmEdge-TensorFlow-Tools 的测试样例，在 `wasm-learning/rust/birds_v1` 中提供的例子是通过一张 jpg 图片来识别鸟的种类。

```
$ git clone https://github.com/second-state/wasm-learning.git
$ cd wasm-learning/rust/birds_v1
```

Cargo 构建 wasm 源文件，生成的 wasm 文件位于 `target/wasm32-wasi/release/birds_v1.wasm` 。

```
$ rustup target add wasm32-wasi
$ cargo build --release --target=wasm32-wasi
```

将测试需要的 wasm 源文件， tensorflow 模型以及 jpg 图片文件推送到 Android 设备上。

```
$ adb push target/wasm32-wasi/release/birds_v1.wasm /data/local/tmp/WasmEdge-tensorflow-tools
$ adb push lite-model_aiy_vision_classifier_birds_V1_3.tflite /data/local/tmp/WasmEdge-tensorflow-tools
$ adb push bird.jpg /data/local/tmp/WasmEdge-tensorflow-tools
```

### 运行 WasmEdge-TensorFlow-Tools

在命令行输入 `adb shell` 进入 Android 设备，查看 `/data/local/tmp/WasmEdge-tensorflow-tools` 文件夹下的工具及测试文件是否齐全。

```
$ adb shell
sirius:/ $ cd /data/local/tmp/WasmEdge-tensorflow-tools
sirius:/data/local/tmp/WasmEdge-tensorflow-tools $ ls
bird.jpg               lite-model_aiy_vision_classifier_birds_V1_3.tflite 
birds_v1.wasm          show-tflite-tensor                                 
libtensorflowlite_c.so wasmedge-tensorflow-lite
```

链接 TensorFlow-Lite 动态依赖库，并使用 show-tflite-tensor 检查 TensorFlow-Lite 的可用性。

```
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

运行 wasmedge-tensorflow-lite 工具，识别 jpg 图片中鸟的种类。

```
sirius:/data/local/tmp/WasmEdge-tensorflow-tools $ chmod 777 wasmedge-tensorflow-lite
sirius:/data/local/tmp/WasmEdge-tensorflow-tools $ ./wasmedge-tensorflow-lite --dir .:. birds_v1.wasm lite-model_aiy_vision_classifier_birds_V1_3.tflite bird.jpg
INFO: Initialized TensorFlow Lite runtime.
166 : 0.84705883
```

该输出结果表明：识别出的图片中鸟的种类[索引为 166](https://github.com/second-state/wasm-learning/blob/master/rust/birds_v1/aiy_birds_V1_labels.txt#L166)，准确率为 84%。
