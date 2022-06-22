# TensorFlow

解释器支持 WasmEdge TensorFlow lite 的推断扩展，从而使你的 JavaScript 能够运行 ImageNet 模型来进行图像分类。本文会向你展示如何在你的 javascript 程序中使用基于 WasmEdge 的 TensorFlow Rust SDK。

下面是一个 JavaScript 的示例。全部代码可参考这里 [example_js/tensorflow_lite_demo/](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/tensorflow_lite_demo)

```javascript
import {Image} from 'image';
import * as std from 'std';
import {TensorflowLiteSession} from 'tensorflow_lite';

let img = new Image('food.jpg');
let img_rgb = img.to_rgb().resize(192, 192);
let rgb_pix = img_rgb.pixels();

let session = new TensorflowLiteSession(
    'lite-model_aiy_vision_classifier_food_V1_1.tflite');
session.add_input('input', rgb_pix);
session.run();
let output = session.get_output('MobilenetV1/Predictions/Softmax');
let output_view = new Uint8Array(output);
let max = 0;
let max_idx = 0;
for (var i in output_view) {
  let v = output_view[i];
  if (v > max) {
    max = v;
    max_idx = i;
  }
}
let label_file = std.open('aiy_food_V1_labelmap.txt', 'r');
let label = '';
for (var i = 0; i <= max_idx; i++) {
  label = label_file.getline();
}
label_file.close();

print('label:');
print(label);
print('confidence:');
print(max / 255);
```

为了在 WasmEdge runtime 中运行 JavaScript，你可以在 CLI 中执行如下操作，这样就可以重新构建一个包含 TensorFlow 的 QuickJS 引擎，然后就可以在 JavaScript 应用中调用 TensorFlow API 了。

```bash
$ cargo build --target wasm32-wasi --release --features=tensorflow
... ...
$ cd example_js/tensorflow_lite_demo
$ wasmedge-tensorflow-lite --dir .:. ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm main.js
label:
Hot dog
confidence:
0.8941176470588236
```

> 注意: 命令行中的 `--dir .:.` 是为了给 wasmedge 开启本地文件读取权限， 以便正常读取到 `main.js` 文件.

## 注意

* 编译器标志 `--features=tensorflow` 构建了一个包含 WasmEdge TensorFlow 扩展的 QuickJS 引擎。
* 程序 `wasmedge-tensorflow-lite` 是 WasmEdge 包的一部分。它是内含有 Tensorflow 扩展的 WasmEdge runtime。

你现在应该可以看到食物的名字了，它被 TensorFlow lite ImageNet 模型识别出来了。

## 使其更快

以上 Tensorflow 推断示例执行一次需要耗时 1-2 秒. 在 web 应用场景中虽然可以接受，但是仍然有改进空间。回想一下，WasmEdge 是如今最快的 WebAssembly runtime，这是由于它的提前编译（AOT, Ahead-of-time compiler）优化。WasmEdge 提供了一个 `wasmedgec` 实用程序去编译和添加原生机器码段，并放到 `wasm` 文件里，从而达到更快的性能。

以下示例采用了 `wasmedge` 和 `wasmedgec` 的扩展版本以支持 WasmEdge Tensorflow 扩展。

```bash
$ cd example_js/tensorflow_lite_demo
$ wasmedgec-tensorflow ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm wasmedge_quickjs.wasm
$ wasmedge-tensorflow-lite --dir .:. wasmedge_quickjs.wasm main.js
label:
Hot dog
confidence:
0.8941176470588236
```

你可以看到，图像识别任务在 0.1 秒内就完成了。它的性能至少提高了 10 倍。
