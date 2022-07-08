# Use Docker for WasmEdge app development

The `appdev` Docker images provide a complete WasmEdge application development environment. To use it, do the following.

## On x86_64 machines

```bash
$ docker pull wasmedge/appdev_x86_64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_x86_64:0.9.0
(docker) #
```

Here is the [Dockerfile](https://github.com/WasmEdge/WasmEdge/blob/master/utils/docker/Dockerfile.appdev_x86_64) and [Docker Hub image](https://hub.docker.com/repository/docker/wasmedge/appdev_x86_64).

## On arm64 machines

```bash
$ docker pull wasmedge/appdev_aarch64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_aarch64:0.9.0
(docker) #
```

Here is the [Dockerfile](https://github.com/WasmEdge/WasmEdge/blob/master/utils/docker/Dockerfile.appdev_aarch64) and [Docker Hub image](https://hub.docker.com/repository/docker/wasmedge/appdev_aarch64).

The WasmEdge application development Docker image installs the following components.

* WasmEdge CLI and shared libraries
* WasmEdge with Tensorflow extension CLI and libraries (x86_64 only)
* Golang
* Rust
* Node.js with WasmEdge addons
* Examples in the `/root/examples/` folder

## Examples

Hello World. [See more simple examples](https://github.com/WasmEdge/WasmEdge/tree/master/examples/wasm)

```bash
$ wasmedge hello.wasm world
hello
world
```

Use AOT to run it *much faster*.

```bash
$ wasmedgec hello.wasm hello.wasm
$ wasmedge hello.wasm world
hello
world
```

Here are some JavaScript examples. [See more](https://github.com/WasmEdge/WasmEdge/tree/master/examples/js)

```bash
$ wasmedge --dir .:. qjs.wasm hello.js 1 2 3
Hello 1 2 3

$ wasmedge-tensorflow-lite --dir .:. qjs_tf.wasm tf_image_classify.js
label: Hot dog
confidence: 0.8941176470588236
```

## Build and publish the appdev images

Run these commands to build and publish the `appdev` Docker images.

### Build on an x86_64 machine

```bash
docker build -t wasmedge/appdev_x86_64:0.9.0 -f Dockerfile.appdev_x86_64 ./ 
docker image push wasmedge/appdev_x86_64:0.9.0
```

### Build on an ARM64 / aarch64 machine

```bash
docker build -t wasmedge/appdev_aarch64:0.9.0 -f Dockerfile.appdev_aarch64 ./
docker image push wasmedge/appdev_aarch64:0.9.0
```

## Use WasmEdge release images

The `wasmedge/wasmedge:release-{version}` Docker images provide a slim WasmEdge images built with [DockerSlim](https://dockersl.im) every releases.

- Image `wasmedge/wasmedge:release-{version}` includes the following command line utilities:
  - `wasmedge`
  - `wasmedgec`
- Image `wasmedge/wasmedge:release-{version}-tensorflow` includes the following command line utilities:
  - `wasmedge`
  - `wasmedgec`
  - `wasmedge-tensorflow-lite`
  - `wasmedge-tensorflow`
  - `show-tflite-tensor`
- The working directory of the release docker image is `/app`.

### Examples

Use `wasmedgec` and `wasmedge` ([link](https://github.com/WasmEdge/WasmEdge/tree/master/examples/wasm)):

```bash
$ docker pull wasmedge/wasmedge:release-0.10.1

$ docker run -it --rm -v $PWD:/app wasmedge/wasmedge:release-0.10.1 wasmedgec hello.wasm hello.aot.wasm
[2022-07-07 08:15:49.154] [info] compile start
[2022-07-07 08:15:49.163] [info] verify start
[2022-07-07 08:15:49.169] [info] optimize start
[2022-07-07 08:15:49.808] [info] codegen start
[2022-07-07 08:15:50.419] [info] output start
[2022-07-07 08:15:50.421] [info] compile done
[2022-07-07 08:15:50.422] [info] output start

$ docker run -it --rm -v $PWD:/app wasmedge/wasmedge:release-0.10.1 wasmedge hello.aot.wasm world
hello
world
```

Use `wasmedge-tensorflow-lite` ([link](https://github.com/WasmEdge/WasmEdge/tree/master/examples/js)):

```bash
$ docker pull wasmedge/wasmedge:release-0.10.1-tensorflow
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/aiy_food_V1_labelmap.txt
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/food.jpg
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/lite-model_aiy_vision_classifier_food_V1_1.tflite
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/main.js

$ docker run -it --rm -v $PWD:/app wasmedge/wasmedge:release-0.10.1-tensorflow wasmedge-tensorflow-lite --dir .:. qjs_tf.wasm main.js
label:
Hot dog
confidence:
0.8941176470588236
```
