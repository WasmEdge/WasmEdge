# Tensorflow

The interpreter supports the WasmEdge Tensorflow lite inference extension so that your JavaScript can run an ImageNet model for image classification. This article will show you how to use the TensorFlow Rust SDK for WasmEdge from your javascript program.


Here is an example of JavaScript. You could find the full code from [example_js/tensorflow_lite_demo/](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/tensorflow_lite_demo).

```
import {TensorflowLiteSession} from 'tensorflow_lite'
import {Image} from 'image'let img = new Image('./example_js/tensorflow_lite_demo/food.jpg')

let img_rgb = img.to_rgb().resize(192,192)
let rgb_pix = img_rgb.pixels()let session = new TensorflowLiteSession('./example_js/tensorflow_lite_demo/lite-model_aiy_vision_classifier_food_V1_1.tflite')

session.add_input('input',rgb_pix)
session.run()
let output = session.get_output('MobilenetV1/Predictions/Softmax');
let output_view = new Uint8Array(output)
let max = 0;
let max_idx = 0;
for (var i in output_view){
    let v = output_view[i]
    if(v>max){
        max = v;
        max_idx = i;
    }
}
print(max,max_idx)
```

To run the JavaScript in the WasmEdge runtime, you can do the following on the CLI to re-build the QuickJS engine with Tensorflow and then run the JavaScript program with Tensorflow API.

```
$ cargo build --target wasm32-wasi --release --features=tensorflow
... ...
$ cd example_js/tensorflow_lite_demo
$ wasmedge-tensorflow-lite --dir .:. ../../target/wasm32-wasi/release/wasmedge_quickjs.wasm main.js
label:
Hot dog
confidence:
0.8941176470588236
```
>  Note, the --dir .:. on the command line is to give wasmedge permission to read the local directory in the file system for the main.js file.


#### Note:

* The `--features=tensorflow` compiler flag builds a version of the QuickJS engine with WasmEdge Tensorflow extensions.
* The `wasmedge-tensorflow-lite` program is part of the WasmEdge package. It is the WasmEdge runtime with the Tensorflow extension built in.

You should now see the name of the food item recognized by the TensorFlow lite ImageNet model.
