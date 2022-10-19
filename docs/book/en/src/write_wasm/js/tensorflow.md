# TensorFlow

The interpreter supports the WasmEdge TensorFlow lite inference extension so that your JavaScript can run an ImageNet model for image classification. This article will show you how to use the TensorFlow Rust SDK for WasmEdge from your javascript program. You will first download the WasmEdge QuickJS Runtime with tensorflow support built-in.

```bash
curl -OL https://github.com/second-state/wasmedge-quickjs/releases/download/v0.4.0-alpha/wasmedge_quickjs_tf.wasm
```

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

To run the JavaScript in the WasmEdge runtime, you can do the following on the CLI. You should now see the name of the food item recognized by the TensorFlow lite ImageNet model.

```bash
$ wasmedge-tensorflow-lite --dir .:. /path/to/wasmedge_quickjs_tf.wasm example_js/tensorflow_lite_demo/main.js
label:
Hot dog
confidence:
0.8941176470588236
```

> The `wasmedge-tensorflow-lite` program is part of the WasmEdge package. It is the WasmEdge runtime with the Tensorflow extension built in.
