# WasmEdge CLI tools for Android

In this section, we will show you how to use WasmEdge CLI tools on Android devices. We will showcase a full WasmEdge demo to perform image classification (Tensorflow-based AI inference) on an Android device.

## Install Android version of WasmEdge-TensorFlow-Tools

First, install WasmEdge-TensorFlow-Tools pre-release on your Android device. It works with the Android version of TensorFlow-Lite dynamic shared library.

### Preparation

#### Android developer options

Currently, WasmEdge only supports the arm64-v8a architecture on Android devices. You need an arm64-v8a Android simulator or a physical device with [developer options turned on](https://developer.android.com/studio/debug/dev-options). WasmEdge requires Android 6.0 and above.

#### Android development CLI

In Ubuntu Linux, you can use the `apt-get` command to install Android debugging and testing tool `adb`. Using the `adb shell` command on the Ubuntu dev machine, you can open a CLI shell to execute commands on the connected Android device.

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

### Install WasmEdge-TensorFlow-Tools packages

Use the following commands on your Ubuntu dev machine to download the WasmEdge-TensorFlow-Tools pre-release packages.

```bash
$ wget https://github.com/second-state/WasmEdge-tensorflow-tools/releases/download/{{ wasmedge_version }}/WasmEdge-tensorflow-tools-{{ wasmedge_version }}-android_aarch64.tar.gz
$ mkdir WasmEdge-tensorflow-tools && tar zxvf WasmEdge-tensorflow-tools-{{ wasmedge_version }}-android_aarch64.tar.gz -C WasmEdge-tensorflow-tools
show-tflite-tensor
wasmedge-tensorflow-lite
```

### Install Android version of the TensorFlow-Lite shared library

We provide an Android compatible version of TensorFlow-Lite dynamic shared library in the WasmEdge-Tensorflow-deps package. Download the package to your Ubuntu dev machine as follows.

```bash
$ wget https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/{{ wasmedge_version }}/WasmEdge-tensorflow-deps-TFLite-{{ wasmedge_version }}-android_aarch64.tar.gz
$ tar zxvf WasmEdge-tensorflow-deps-TFLite-{{ wasmedge_version }}-android_aarch64.tar.gz -C WasmEdge-tensorflow-tools
libtensorflowlite_c.so
```

Next use the `adb` tool to push the downloaded WasmEdge-TensorFlow packages onto a connected Android device.

```bash
adb push WasmEdge-tensorflow-tools /data/local/tmp
```

## Try it out

### Sample application

In this example, we will demonstrate a standard [WasmEdge Tensorflow-Lite sample application](https://github.com/second-state/wasm-learning/tree/master/rust/birds_v1). It can recognize and classify the bird type from a JPG or PNG picture of a bird. The explanation of the source code can be [found here](../../../write_wasm/rust/tensorflow.md).

```bash
git clone https://github.com/second-state/wasm-learning.git
cd wasm-learning/rust/birds_v1
```

Use the `cargo` command to build a Wasm bytecode file from the Rust source code. The Wasm file is located at `target/wasm32-wasi/release/birds_v1.wasm`.

```bash
rustup target add wasm32-wasi
cargo build --release --target=wasm32-wasi
```

Push the Wasm bytecode file, tensorflow lite model file, and the test bird picture file onto the Android device using `adb`.

```bash
adb push target/wasm32-wasi/release/birds_v1.wasm /data/local/tmp/WasmEdge-tensorflow-tools
adb push lite-model_aiy_vision_classifier_birds_V1_3.tflite /data/local/tmp/WasmEdge-tensorflow-tools
adb push bird.jpg /data/local/tmp/WasmEdge-tensorflow-tools
```

### Run the WasmEdge-TensorFlow-Tools

Type `adb shell` from the Ubuntu CLI to open a command shell for the connected Android device. Confirm that the tools, programs, and test image are all available on the Android device under the `/data/local/tmp/WasmEdge-tensorflow-tools` folder.

```bash
$ adb shell
sirius:/ $ cd /data/local/tmp/WasmEdge-tensorflow-tools
sirius:/data/local/tmp/WasmEdge-tensorflow-tools $ ls
bird.jpg               lite-model_aiy_vision_classifier_birds_V1_3.tflite
birds_v1.wasm          show-tflite-tensor
libtensorflowlite_c.so wasmedge-tensorflow-lite
```

Load the TensorFlow-Lite dynamic shared library, and use the `show-tflite-tensor` CLI tool to examine the Tensorflow Lite model file.

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

Use the extended WasmEdge Runtime in `wasmedge-tensorflow-lite` to execute the compiled Wasm program on the Android device. It loads the Tensorflow Lite model and bird image, and outputs the bird classification and its confidence.

```bash
sirius:/data/local/tmp/WasmEdge-tensorflow-tools $ chmod 777 wasmedge-tensorflow-lite
sirius:/data/local/tmp/WasmEdge-tensorflow-tools $ ./wasmedge-tensorflow-lite --dir .:. birds_v1.wasm lite-model_aiy_vision_classifier_birds_V1_3.tflite bird.jpg
INFO: Initialized TensorFlow Lite runtime.
166 : 0.84705883
```

The result shows that the bird type is in [line 166 of the label file](https://github.com/second-state/wasm-learning/blob/master/rust/birds_v1/aiy_birds_V1_labels.txt#L166) (Sicalis flaveola) and the confidence level is 84.7%.
