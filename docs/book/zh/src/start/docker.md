# 使用 Docker 进行 WasmEdge 应用程序开发

`appdev` Docker 镜像提供了一个完整的 WasmEdge 应用程序开发环境。要想使用该镜像，请执行以下操作。

## 使用 x86_64 系统

```bash
$ docker pull wasmedge/appdev_x86_64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_x86_64:0.9.0
(docker) #
```

查看 `appdev` 的 [Dockerfile](https://github.com/WasmEdge/WasmEdge/blob/master/utils/docker/Dockerfile.appdev_x86_64) 和 [Docker Hub 镜像](https://hub.docker.com/repository/docker/wasmedge/appdev_x86_64)。

## 使用 arm64 系统

```bash
$ docker pull wasmedge/appdev_aarch64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_aarch64:0.9.0
(docker) #
```

查看 `appdev` 的 [Dockerfile](https://github.com/WasmEdge/WasmEdge/blob/master/utils/docker/Dockerfile.appdev_aarch64) 和 [Docker Hub 镜像](https://hub.docker.com/repository/docker/wasmedge/appdev_aarch64)。

这个 `appdev` 镜像安装了以下组件：

- WasmEdge CLI 和共享库；
- (仅限 x86_64 系统）包含 Tensorflow 扩展命令行和库的 WasmEdge；
- Golang；
- Rust；
- 包含 WasmEdge 插件的 Node.js；
- 在 `/root/examples/` 文件夹的一些示例。

## 一些示例

Hello World 示例。[查看更多。](https://github.com/WasmEdge/WasmEdge/tree/master/tools/wasmedge/examples)

```bash
$ wasmedge hello.wasm world
hello
world
```

使用 AOT 来**加快程序的运行速度**。

```bash
$ wasmedgec hello.wasm hello.wasm
$ wasmedge hello.wasm world
hello
world
```

以下是一些 JavaScript 示例。[查看更多。](https://github.com/WasmEdge/WasmEdge/tree/master/tools/wasmedge/examples/js)

```bash
$ wasmedge --dir .:. qjs.wasm hello.js 1 2 3
Hello 1 2 3

$ wasmedge-tensorflow-lite --dir .:. qjs_tf.wasm tf_image_classify.js
label: Hot dog
confidence: 0.8941176470588236
```

## 构建并发布 `appdev` 镜像

运行以下的命令来构建并发布 `appdev` 镜像：

### x86_64 系统

```bash
docker build -t wasmedge/appdev_x86_64:0.9.0 -f Dockerfile.appdev_x86_64 ./
docker image push wasmedge/appdev_x86_64:0.9.0
```

### ARM64 / aarch64 系统

```bash
docker build -t wasmedge/appdev_aarch64:0.9.0 -f Dockerfile.appdev_aarch64 ./
docker image push wasmedge/appdev_aarch64:0.9.0
```
