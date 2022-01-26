# 用 Docker 启动

我们提供了一个名为 `appdev` 的 Docker 镜像，它为开发者们提供了一个完整的 WasmEdge 应用开发环境。

参照下列文档来使用它:

### 在 x86_64 机器上使用

```bash
$ docker pull wasmedge/appdev_x86_64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_x86_64:0.9.0
(docker) #
```

这里提供了示例 [Dockerfile](https://github.com/WasmEdge/WasmEdge/blob/master/utils/docker/Dockerfile.appdev_x86_64) 和 [Docker Hub image](https://hub.docker.com/repository/docker/wasmedge/appdev_x86_64).

### 在 arm64 机器上使用

```bash
$ docker pull wasmedge/appdev_aarch64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_aarch64:0.9.0
(docker) #
```

这里提供了示例 [Dockerfile](https://github.com/WasmEdge/WasmEdge/blob/master/utils/docker/Dockerfile.appdev_aarch64) 和 [Docker Hub image](https://hub.docker.com/repository/docker/wasmedge/appdev_aarch64).

这个 `appdev` 镜像安装了以下组件:

- WasmEdge CLI 和 shared libraries
- WasmEdge with Tensorflow extension CLI and libraries (x86_64 only)
- Golang
- Rust
- Node.js with WasmEdge addons
- 在 `/root/examples/` 文件夹的一些示例

## 一些示例

Hello World. [点击此处查看更多示例](https://github.com/WasmEdge/WasmEdge/tree/master/tools/wasmedge/examples)

```bash
$ wasmedge hello.wasm world
hello
world
```

使用 AOT  来让它跑的 **更快**.

```bash
$ wasmedgec hello.wasm hello.wasm
$ wasmedge hello.wasm world
hello
world
```

这里也有一些 JavaScript 示例. [查看](https://github.com/WasmEdge/WasmEdge/tree/master/tools/wasmedge/examples/js)

```bash
$ wasmedge --dir .:. qjs.wasm hello.js 1 2 3
Hello 1 2 3

$ wasmedge-tensorflow-lite --dir .:. qjs_tf.wasm tf_image_classify.js
label: Hot dog
confidence: 0.8941176470588236
```

## 构建并发布 appdev 镜像

使用以下的命令来构建并发布 appdev 镜像

### 在一个 x86_64 机器上构建

```bash
docker build -t wasmedge/appdev_x86_64:0.9.0 -f Dockerfile.appdev_x86_64 ./
docker image push wasmedge/appdev_x86_64:0.9.0
```

### 在一个 ARM64 / aarch64 机器上构建

```bash
docker build -t wasmedge/appdev_aarch64:0.9.0 -f Dockerfile.appdev_aarch64 ./
docker image push wasmedge/appdev_aarch64:0.9.0
```
