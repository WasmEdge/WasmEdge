# TensorFlow

The interpreter supports the WasmEdge TensorFlow lite inference extension so that your JavaScript can run an ImageNet model for image classification. This article will show you how to use the TensorFlow Rust SDK for WasmEdge from your javascript program.

Here is an example of JavaScript. You could find the full code from [example_js/tensorflow_lite_demo/](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/tensorflow_lite_demo).

```javascript
import {Image} from 'image';
import * as std from 'std';
import {TensorflowLiteSession} from 'tensorflow_lite';

let img = new Image(__dirname + '/food.jpg');
let img_rgb = img.to_rgb().resize(192, 192);
let rgb_pix = img_rgb.pixels();

let session = new TensorflowLiteSession(
    __dirname + '/lite-model_aiy_vision_classifier_food_V1_1.tflite');
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
let label_file = std.open(__dirname + '/aiy_food_V1_labelmap.txt', 'r');
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

To run the JavaScript in the WasmEdge runtime, you can do the following on the CLI to re-build the QuickJS engine with TensorFlow and then run the JavaScript program with TensorFlow API.

```bash
$ cargo build --target wasm32-wasi --release --features=tensorflow
$ wasmedge-tensorflow-lite --dir .:. target/wasm32-wasi/release/wasmedge_quickjs.wasm example_js/tensorflow_lite_demo/main.js
label:
Hot dog
confidence:
0.8941176470588236
```

> Note: the `--dir .:.` on the command line is to give wasmedge permission to read the local directory in the file system for the `main.js` file.

## Note

* The `--features=tensorflow` compiler flag builds a version of the QuickJS engine with WasmEdge TensorFlow extensions.
* The `wasmedge-tensorflow-lite` program is part of the WasmEdge package. It is the WasmEdge runtime with the Tensorflow extension built in.

You should now see the name of the food item recognized by the TensorFlow lite ImageNet model.

## Make it faster

The above Tensorflow inference example takes 1–2 seconds to run. It is acceptable in web application scenarios but could be improved. Recall that WasmEdge is the fastest WebAssembly runtime today due to its AOT (Ahead-of-time compiler) optimization. WasmEdge provides a `wasmedgec` utility to compile and add a native machine code section to the `wasm` file for much faster performance.

The following example uses the extended versions to `wasmedge` and `wasmedgec` to support the WasmEdge Tensorflow extension.

```bash
$ wasmedgec-tensorflow target/wasm32-wasi/release/wasmedge_quickjs.wasm wasmedge_quickjs.wasm
$ wasmedge-tensorflow-lite --dir .:. wasmedge_quickjs.wasm example_js/tensorflow_lite_demo/main.js
label:
Hot dog
confidence:
0.8941176470588236
```

You can see that the image classification task can be completed within 0.1s. It is at least 10x improvement!
