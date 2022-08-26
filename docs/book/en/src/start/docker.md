# Use WasmEdge released Docker images

The `wasmedge/slim:{version}` Docker images provide a slim WasmEdge images built with [DockerSlim](https://dockersl.im) every releases.

* Image `wasmedge/slim-runtime:{version}` includes only WasmEdge runtime with `wasmedge` command.
* Image `wasmedge/slim:{version}` includes the following command line utilities:
  * `wasmedge`
  * `wasmedgec`
* Image `wasmedge/slim-tf:{version}` includes the following command line utilities:
  * `wasmedge`
  * `wasmedgec`
  * `wasmedge-tensorflow-lite`
  * `wasmedge-tensorflow`
  * `show-tflite-tensor`
* The working directory of the release docker image is `/app`.

## Examples

Use `wasmedgec` and `wasmedge` ([link](https://github.com/WasmEdge/WasmEdge/tree/master/examples/wasm)):

```bash
$ docker pull wasmedge/slim:{{ wasmedge_version }}

$ docker run -it --rm -v $PWD:/app wasmedge/slim:{{ wasmedge_version }} wasmedgec hello.wasm hello.aot.wasm
[2022-07-07 08:15:49.154] [info] compile start
[2022-07-07 08:15:49.163] [info] verify start
[2022-07-07 08:15:49.169] [info] optimize start
[2022-07-07 08:15:49.808] [info] codegen start
[2022-07-07 08:15:50.419] [info] output start
[2022-07-07 08:15:50.421] [info] compile done
[2022-07-07 08:15:50.422] [info] output start

$ docker run -it --rm -v $PWD:/app wasmedge/slim:{{ wasmedge_version }} wasmedge hello.aot.wasm world
hello
world
```

Use `wasmedge-tensorflow-lite` ([link](https://github.com/WasmEdge/WasmEdge/tree/master/examples/js)):

```bash
$ docker pull wasmedge/slim-tf:{{ wasmedge_version }}
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/aiy_food_V1_labelmap.txt
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/food.jpg
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/lite-model_aiy_vision_classifier_food_V1_1.tflite
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/main.js

$ docker run -it --rm -v $PWD:/app wasmedge/slim-tf:{{ wasmedge_version }} wasmedge-tensorflow-lite --dir .:. qjs_tf.wasm main.js
label:
Hot dog
confidence:
0.8941176470588236
```
