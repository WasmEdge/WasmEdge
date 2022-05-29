# 使用 Docker 開發 WasmEdge 應用程式

我們提供一系列 `appdev` 的 Docker 映像檔，他們提供了完整的 WasmEdge 應用程式開發環境。請參考以下文件來使用。

## 使用 x86_64 環境

```bash
$ docker pull wasmedge/appdev_x86_64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_x86_64:0.9.0
(docker) #
```

請參考 [Dockerfile](https://github.com/WasmEdge/WasmEdge/blob/master/utils/docker/Dockerfile.appdev_x86_64) 和 [Docker Hub image](https://hub.docker.com/repository/docker/wasmedge/appdev_x86_64).

## 使用 arm64 環境

```bash
$ docker pull wasmedge/appdev_aarch64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_aarch64:0.9.0
(docker) #
```

請參考 [Dockerfile](https://github.com/WasmEdge/WasmEdge/blob/master/utils/docker/Dockerfile.appdev_aarch64) 和 [Docker Hub image](https://hub.docker.com/repository/docker/wasmedge/appdev_aarch64).

這些 `appdev` 映像檔安裝了以下套件：

- WasmEdge CLI 和共享函式庫
- 包含 Tensorflow 擴充的 WasmEdge CLI 和函式庫（僅限於 x86_64 版本）
- Golang
- Rust
- Node.js with WasmEdge addons
- 在 `/root/examples/` 資料夾下的範例

## 範例

Hello World. [請在此參考更多範例](https://github.com/WasmEdge/WasmEdge/tree/master/examples/wasm) 。

```bash
$ wasmedge hello.wasm world
hello
world
```

使用 AOT 模式來**更快速**的執行。

```bash
$ wasmedgec hello.wasm hello.wasm
$ wasmedge hello.wasm world
hello
world
```

這裡也有一些 [JavaScript 範例](https://github.com/WasmEdge/WasmEdge/tree/master/examples/js)。

```bash
$ wasmedge --dir .:. qjs.wasm hello.js 1 2 3
Hello 1 2 3

$ wasmedge-tensorflow-lite --dir .:. qjs_tf.wasm tf_image_classify.js
label: Hot dog
confidence: 0.8941176470588236
```

## 組建並發布 appdev 映像檔

執行以下指令來組建並發布 `appdev` Docker 映像檔。

### 在 x86_64 機器組建

```bash
docker build -t wasmedge/appdev_x86_64:0.9.0 -f Dockerfile.appdev_x86_64 ./
docker image push wasmedge/appdev_x86_64:0.9.0
```

### 在 ARM64 / aarch64 機器組建

```bash
docker build -t wasmedge/appdev_aarch64:0.9.0 -f Dockerfile.appdev_aarch64 ./
docker image push wasmedge/appdev_aarch64:0.9.0
```
