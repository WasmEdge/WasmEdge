# Use the appdev Docker images

The `appdev` Docker images provide a complete WasmEdge application development environment. To use it, do the following.

## On x86_64 machines

```bash
$ docker pull wasmedge/appdev_x86_64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_x86_64:0.9.0
(docker) #
```

## On arm64 machines

```bash
$ docker pull wasmedge/appdev_aarch64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_aarch64:0.9.0
(docker) #
```

It installs the following components.

* WasmEdge CLI and shared libraries
* WasmEdge with Tensorflow extension CLI and libraries (x86_64 only)
* Golang
* Rust
* Node.js with WasmEdge addons
* Examples in the `/root/examples/` folder

## Examples

Hello World. [See more simple examples](https://github.com/WasmEdge/WasmEdge/tree/master/tools/wasmedge/examples)

```bash
$ wasmedge hello.wasm world
hello
world
```

Use AOT to run it *much faster*.

```bash
$ wasmedgec hello.wasm hello.so
$ wasmedge hello.so world
hello
world
```

Here are some JavaScript examples. [See more](https://github.com/WasmEdge/WasmEdge/tree/master/tools/wasmedge/examples/js)

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
